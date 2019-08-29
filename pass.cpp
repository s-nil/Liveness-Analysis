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

		DenseMap<StringRef,int> valuetoIdx;	// map[instruction name --> index in bitvector]
		DenseMap<int,StringRef> idxtoValue;	// map[index in bitvector --> instruction name]

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

			for(int i = 0; i<domain.size(); ++i){
				valuetoIdx[domain[i]->getName()] = i;
				idxtoValue[i] = domain[i]->getName();
			}

			FlowResult result = run(F);

			errs() << "Live values out of each Basic Block :\n";
			errs() << "--------------------------------------\n";
			errs() << "\tBasic Block \t:\t Live Values\n";
			errs() << "\t----------------------------------------------------\n";
			for(auto i = result.fr.begin(); i!=result.fr.end(); ++i){
				errs() << "\t" << (*(*i).first).getName() << "\t\t:\t";
				BitVector tmp = (*i).second.out;
				for(int i=0; i<domain.size(); ++i){
					if(tmp[i]){
						errs() << idxtoValue[i] << ',';
					}
				}
				errs() <<"\b \n";
			}

			/**
			 * 	computing for second output
			 */
			DenseMap<BasicBlock*, vector<pair<int,set<StringRef>>>> liveatPP;
			for(auto i = result.fr.begin(); i!=result.fr.end(); ++i){

				int program_points = (*i).first->size() + 1;

				set<StringRef> tmplive;
				BitVector tmp = (*i).second.out;
				for(int ii=0; ii<domain.size(); ++ii){
					if(tmp[ii]){
						tmplive.insert(idxtoValue[ii]);
					}
				}

				program_points--;
				pair<int,set<StringRef>> p(program_points,tmplive);
				liveatPP[(*i).first].push_back(p);
				for(auto ins_it=(*i).first->rbegin(); ins_it!=(*i).first->rend(); ++ins_it){

					if((*ins_it).getOpcode() != Instruction::PHI){

					if((*ins_it).hasName() &&
									tmplive.find((*ins_it).getName())!=tmplive.end())
						tmplive.erase((*ins_it).getName());

					for(int opr=0; opr<(*ins_it).getNumOperands(); ++opr){
						if((*ins_it).getOperand(opr)->hasName() &&
									valuetoIdx.find((*ins_it).getOperand(opr)->getName())!=valuetoIdx.end()){
							tmplive.insert((*ins_it).getOperand(opr)->getName());
						}
					}

					program_points--;
					p.first = program_points;
					p.second = tmplive;
					liveatPP[(*i).first].push_back(p);
					}else{
						program_points--;
						p.first = program_points;
						set<StringRef> s;
						p.second = s;
						liveatPP[(*i).first].push_back(p);
					}
				}
			}

			errs() << "------------------------------------------------------------------\n";
			errs() << "Live values at each program point in each Basic Block : \n";
			map<int,int> histogram;
			for(auto it=liveatPP.begin(); it!=liveatPP.end(); ++it){
				errs() << "Basic Block\n";
				errs() << "-----------------\n";
				errs() << (*it).first->getName() << " :\n";
				errs() << "\tprogram_point : live values\n";
				errs() << "\t----------------------------\n";
				for(auto x : (*it).second){
					errs() << "\t" << x.first << " : ";
					histogram[x.second.size()]++;
					for(auto y : x.second){
						errs() << y << ',';
					}
					errs() << "\b \n";
				}
			}

			errs() << "\n------------------------------------------------------------------\n";
			errs() << "Histogram : \n";
			errs() << "------------\n";
			errs() << "#live_values\t: #program_points\n";
			errs() << "---------------------------------\n";
			for(auto it=histogram.begin(); it!=histogram.end(); ++it){
				errs() << "\t" << (*it).first << "\t:\t" << (*it).second << '\n';
			}

			return false;
	    }

		FlowResult run(Function &F){
			DenseMap<BasicBlock*, FlowResultofBB> initValues;
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
					//	entry bb of function will have function arguments as def
					if(&*bbit == &*firstBB){
						for(auto arg_it=F.arg_begin(); arg_it!=F.arg_end(); ++arg_it){
							tmpdef.set(valuetoIdx[(*arg_it).getName()]);
						}
					}
					//	read the above algo
					for(auto j=0; j<i.getNumOperands(); ++j){
						if(i.getOperand(j)->hasName() &&
											valuetoIdx.find(i.getOperand(j)->getName())!=valuetoIdx.end()){

							if(!tmpdef[valuetoIdx[i.getOperand(j)->getName()]])
								tmpuse.set(valuetoIdx[i.getOperand(j)->getName()]);
						}
					}
					//	read the above algo
					if(i.hasName())
						tmpdef.set(valuetoIdx[i.getName()]);
				}
				use[&*bbit] = tmpuse;
				def[&*bbit] = tmpdef;
			}

			/**
			 * for each Basic Block in CFG in reverse top sort order
			 * 		in'[b]	=	in[b]
			 * 		out'[b]	=	out[b]
			 * 		out[b]	=	union of in[s], where s belongs to SUCC[b] 		//(deal with phi instructions also)
			 * 		in[b]	=	use[n] union (out[b] - def[b])
			 * 	untill in'[b] = in[b] and out'[b] = out[b] for all basic blocks
			 *
			 */

			bool converge = false;

			int j = 0;
			while(!converge){
				j++;
				converge = true;

				auto &bblist = F.getBasicBlockList();
				for(auto bbit=bblist.rbegin(); bbit!=bblist.rend(); ++bbit){
					BitVector i(domain.size(),false);
					BitVector o(domain.size(),false);

					i = initValues[&*bbit].in;
					o = initValues[&*bbit].out;

					BitVector tmpout(domain.size(),false);
					for(auto it = succ_begin(&*bbit); it!=succ_end(&*bbit); ++it){

						/**
						 * Each operand of a phi instruction is only live along the edge from the corresponding predecessor block
						 */
						BitVector tin = initValues[&**it].in;
						for(auto ins=(*it)->begin(); ins!=(*it)->end(); ++ins){

							if((*ins).getOpcode() == Instruction::PHI){
								for(int opr=0; opr<(*ins).getNumOperands(); ++opr){

									if((*ins).getOperand(opr)->hasName()){
										auto phi = dyn_cast<PHINode>(ins);

										// compare basic block name and change in the bitvector accordingly
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
					a.flip();				// ~def
					a &= tmpout;			//	out & ~def
					a |= use[&*bbit];		//	use | (out & ~def)
					BitVector tmpin(a);		// in = use | (out & ~def)

					initValues[&*bbit].in	= tmpin;
					initValues[&*bbit].out	= tmpout;
					if(converge && (i != tmpin || o != tmpout))
						converge = false;
				}
			}

			FlowResult fr(initValues);
			return fr;
		}
    };
}

char LA::ID = 0;
static RegisterPass<LA> X("LA","Liveness Analysis");
