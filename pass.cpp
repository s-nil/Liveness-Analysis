#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"

using namespace llvm;
namespace{
    struct LA : public FunctionPass{
	static char ID;
	LA(): FunctionPass(ID){}

	virtual bool runOnFunction(Function &F){
	    errs() << "Hello Function Pass\n";
	    return false;
	}
    };
}

char LA::ID = 0;
static RegisterPass<LA> X("LA","Liveness Analysis");
