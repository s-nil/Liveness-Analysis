#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include <vector>

using namespace llvm;
using namespace std;

namespace{
    class LA : public FunctionPass{
	public:	
	    static char ID;
		vector<Value*> domain;
	    LA(): FunctionPass(ID){}

	    virtual bool runOnFunction(Function &F){
		
		errs() << F.getName() << '\n';

		for (auto arg_it = F.arg_begin(); arg_it != F.arg_end(); ++arg_it){
			domain.push_back(arg_it);
		}
		

		for (inst_iterator iit = inst_begin(F); iit!=inst_end(F); ++iit){
			Value* v = &*iit;
			errs() << *v << '\n';
			// errs() << *iit << '\n';
			
		}
		

		return false;
	    }
    };
}

char LA::ID = 0;
static RegisterPass<LA> X("LA","Liveness Analysis");
