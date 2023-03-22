#include<cstdio>
#include<cmath>

int main(){
    int a=48;
    for(int i=0;i<6;i++){
        printf("%d ",a%2);
        a=a>>1;
    }
    printf("%f",atan2(-1,0));
}
//原材料格状态