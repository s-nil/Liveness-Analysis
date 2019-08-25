#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"

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

	// class {};

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
		errs() << F.getName() << '\n';

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
			FlowResult fr;
			DenseMap<BasicBlock*, FlowResultofBB> initValues;
			DenseMap<StringRef,int> valuetoIdx;	//TODO

			for(int i = 0; i<domain.size(); ++i){
				valuetoIdx[domain[i]->getName()] = i;
			}

			BasicBlock* firstBB = &F.front();

			for(auto bbit = F.begin(); bbit != F.end(); ++bbit){
				if(&*bbit == &*firstBB){
					FlowResultofBB tmp(boundaryCondition, boundaryCondition);
					initValues[&*bbit] = tmp;
				}else{
					FlowResultofBB tmp(InitBlockCond, InitBlockCond);
					initValues[&*bbit] = tmp;
				}
			}

			DenseMap<BasicBlock*,BitVector> def;

			for(auto bbit = F.begin(); bbit != F.end(); ++bbit){
				BitVector tmpdef(domain.size(),false);
				for(auto &i : *bbit){
					if(&*bbit == &*firstBB){
						for(auto arg_it=F.arg_begin(); arg_it!=F.arg_end(); ++arg_it){
							tmpdef.set(valuetoIdx[(*arg_it).getName()]);
						}
					}
					if(i.hasName())
						tmpdef.set(valuetoIdx[i.getName()]);
				}
				def[&*bbit] = tmpdef;
			}

			DenseMap<BasicBlock*,BitVector> use;

			for(auto bbit = F.begin(); bbit!=F.end(); ++bbit){
				BitVector tmpuse(domain.size(),false);
				for(auto &i : *bbit){
					for(auto j=0; j<i.getNumOperands(); ++j){
						if(i.getOperand(j)->hasName() && valuetoIdx.find(i.getOperand(j)->getName())!=valuetoIdx.end()){
							tmpuse.set(valuetoIdx[i.getOperand(j)->getName()]);
						}
					}
				}
				use[&*bbit] = tmpuse;
			}

			return fr;
		}

    };
}

char LA::ID = 0;
static RegisterPass<LA> X("LA","Liveness Analysis");
