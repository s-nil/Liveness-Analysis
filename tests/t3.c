int g;

int erk(int a, int b){
    int i;
    int x = g;
    int ret = 0;
    for(i = 0; i < b; ++i){
	ret = ret*a;
    }
    return ret+x;
}
