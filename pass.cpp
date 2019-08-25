#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include <vector>

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

			if(isa<StoreInst>(&*iit)){
				Value *vv = (((StoreInst*)v)->getPointerOperand());
				if(find(domain.begin(),domain.end(),vv) == domain.end()){
					domain.push_back(vv);
				}
			}
			else if(isa<Instruction>(&*iit)){
				string str;
				raw_string_ostream rso(str);
				rso << *iit;

				if(str.size()>2 && str[2]=='%'){
					if(find(domain.begin(),domain.end(),v) == domain.end()){
						domain.push_back(v);
					}
				}
			}
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

		return false;
	    }

    };
}

char LA::ID = 0;
static RegisterPass<LA> X("LA","Liveness Analysis");
