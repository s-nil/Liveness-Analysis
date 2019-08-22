#include <stdio.h>
#include <stdlib.h>

void inner(void){
    puts("inner()");
}

void myfun(void){
    puts("enter myfun()");
    for(int i = 0; i < 10; ++i){
	printf("something %d\n",i);
	inner();
    }
    puts("leave myfun");
}

int main(int argc, char **argv){
    puts("in main");
    myfun();
    puts("after myfun()");
    return 0;
}
