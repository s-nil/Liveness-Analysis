#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/CFG.h"

#include <vector>
#include <set>

using namespace llvm;
using namespace std;

namespace{
	void printBitvector(BitVector b){
		for(int i = 0; i < b.size(); ++i){
			if(b[i])
				errs() << "1";
			else
				errs() << "0";
		}
		errs() << '\n';
	}

	class FlowResultofBB{
		public:
			BitVector in;
			BitVector out;

			FlowResultofBB(){}
			FlowResultofBB(BitVector i, BitVector o){
				in = i;
				out = o;
			}
	};

	class FlowResult{
		public:
			DenseMap<BasicBlock*, FlowResultofBB> fr;
			FlowResult(){}
			FlowResult(DenseMap<BasicBlock*, FlowResultofBB> f){
				fr = f;
			}
	};

    class LA : public FunctionPass{
	public:	
	    static char ID;
		vector<Value*> domain;
	    BitVector boundaryCondition;
		BitVector InitBlockCond;
		LA(): FunctionPass(ID){}

	    virtual bool runOnFunction(Function &F){
		domain.clear();
//		errs() << F.getName() << '\n';

		for (auto arg_it = F.arg_begin(); arg_it != F.arg_end(); ++arg_it){
			domain.push_back(arg_it);
		}
		
		for (inst_iterator iit = inst_begin(F); iit!=inst_end(F); ++iit){
			Value* v = &*iit;

			if((*iit).hasName())
				domain.push_back(v);
		}

		int setSize = domain.size();

		BitVector bc(setSize, false);

		int i = 0;
		for(auto x : domain){
			if(isa<Argument>(x)){
				bc[i] = 1;
			}
			++i;
		}

		boundaryCondition = bc;

		BitVector ibc(setSize, false);
		InitBlockCond = ibc;

		run(F);

		return false;
	    }

		FlowResult run(Function &F){
			DenseMap<BasicBlock*, FlowResultofBB> initValues;
			DenseMap<StringRef,int> valuetoIdx;	//TODO
			DenseMap<int,StringRef> idxtoValue;	//TODO

			for(int i = 0; i<domain.size(); ++i){
				valuetoIdx[domain[i]->getName()] = i;
				idxtoValue[i] = domain[i]->getName();
			}

			BasicBlock* firstBB = &F.front();

			for(auto bbit = F.begin(); bbit != F.end(); ++bbit){
				if(&*bbit == &*firstBB){
					FlowResultofBB tmp(boundaryCondition, InitBlockCond);
					initValues[&*bbit] = tmp;
				}else{
					FlowResultofBB tmp(InitBlockCond, InitBlockCond);
					initValues[&*bbit] = tmp;
				}
			}

			/**	Use-Def of a BB using forward scanning of a BB
			 * ------------------------------------------------
			 * Assume x = y op z
			 *
			 * for i = 1 to numOfInstructions in BB do
			 * 		if(y is not in def)
			 * 			use <-- y
			 *  	if(z is not in def)
			 * 			use <-- z
			 * 		def <-- x
			 * ------------------------------------------------
			 */

			DenseMap<BasicBlock*,BitVector> def;
			DenseMap<BasicBlock*,BitVector> use;

			for(auto bbit = F.begin(); bbit != F.end(); ++bbit){
				BitVector tmpdef(domain.size(),false);
				BitVector tmpuse(domain.size(),false);
				for(auto &i : *bbit){
					if(&*bbit == &*firstBB){
						for(auto arg_it=F.arg_begin(); arg_it!=F.arg_end(); ++arg_it){
							tmpdef.set(valuetoIdx[(*arg_it).getName()]);
						}
					}
					for(auto j=0; j<i.getNumOperands(); ++j){
						if(i.getOperand(j)->hasName() && valuetoIdx.find(i.getOperand(j)->getName())!=valuetoIdx.end()){
							if(!tmpdef[valuetoIdx[i.getOperand(j)->getName()]])
								tmpuse.set(valuetoIdx[i.getOperand(j)->getName()]);
						}
					}
					if(i.hasName())
						tmpdef.set(valuetoIdx[i.getName()]);
				}
				use[&*bbit] = tmpuse;
				def[&*bbit] = tmpdef;
			}

			bool converge = false;

			int i = 0;
			while(!converge){
				i++;
				converge = true;

				auto &bblist = F.getBasicBlockList();
				for(auto bbit=bblist.rbegin(); bbit!=bblist.rend(); ++bbit){
					BitVector i(domain.size(),false);
					BitVector o(domain.size(),false);

					i = initValues[&*bbit].in;
					o = initValues[&*bbit].out;

					BitVector tmpout(domain.size(),false);
					for(auto it = succ_begin(&*bbit); it!=succ_end(&*bbit); ++it){

						BitVector tin = initValues[&**it].in;
						for(auto ins=(*it)->begin(); ins!=(*it)->end(); ++ins){

							if((*ins).getOpcode() == Instruction::PHI){
								for(int opr=0; opr<(*ins).getNumOperands(); ++opr){

									if((*ins).getOperand(opr)->hasName()){
										auto phi = dyn_cast<PHINode>(ins);

										if((*bbit).getName().compare(phi->getIncomingBlock(opr)->getName())){
											tin[valuetoIdx[(*ins).getOperand(opr)->getName()]] = 0;
										}
									}
								}
							}
						}
						tmpout |= tin;
					}

					BitVector a(def[&*bbit]);
					a.flip();
					a &= tmpout;
					a |= use[&*bbit];
					BitVector tmpin(a);

					initValues[&*bbit].in	= tmpin;
					initValues[&*bbit].out	= tmpout;
					if(converge && (i != tmpin || o != tmpout))
						converge = false;
				}
			}

			FlowResult fr(initValues);

			for(auto i = initValues.begin(); i!=initValues.end(); ++i){
				errs() << (*(*i).first).getName() << "\t|\t";
				BitVector tmp = (*i).second.out;
				for(int i=0; i<domain.size(); ++i){
					if(tmp[i]){
						errs() << idxtoValue[i] << ',';
					}
				}
				errs() <<"\b \n";
			}

			return fr;
		}

    };
}

char LA::ID = 0;
static RegisterPass<LA> X("LA","Liveness Analysis");
