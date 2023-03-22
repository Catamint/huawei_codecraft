#include<cstdio>
#include<cmath>

int main(){
    int a=48;
    for(int i=0;i<9;i++){
        printf("%d ",(a>>i)%2);
    }
    printf("%f",atan2(-1,0));
}
//原材料格状态