#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
    if(argc<2)
	return 1;

    int bill = 0;
    int usage = atoi(argv[1]);

    if(usage>0){
	bill = 40;
    }

    if(usage>100){
	if(usage <= 200){
	    bill += (usage - 100)*0.5; 
	}else{
	    bill += bill + 50 + (usage - 200)*0.1;
	    if(bill >= 100){
		    bill *= 0.9;
	    }
	}
    }
    return 0;
}
