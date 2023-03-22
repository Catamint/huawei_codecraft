#include <iostream>
#include<cstdio>
#include<cmath>
#include<vector>
#define PI 3.14

using namespace std;

/*跟踪参数*/
struct contralling{
    double theta;
    double distance;
    double theta_last;
    double distance_last;
} robot_control[3];

char line[1024];        //输入行
char mymap[100][100];   //地图 (1格 = 0.5m)
int frame_id;           //帧ID
int current_money;      //当前的金钱
int K;                  //crafting个数

double destination_1[3];    //测试,机器人1的目的地

double distance_square(double *pos_c, double *pos_d){
    return pow((pos_d[0]-pos_c[0]),2)+pow((pos_d[1]-pos_c[1]),2);
}
double inner_square(double *vec2){
    return pow((vec2[0]),2)+pow((vec2[1]),2);
}
double angle(double *pos_c, double *pos_d){
    double theta = atan2((pos_d[1]-pos_c[1]), (pos_d[0]-pos_c[0]));
    if(theta<0) return 2*M_PI+theta;
    return theta;
}
void controller(double &v, double &w, contralling *p){
    const double a1=1,c1=1, a3=2,c3=2;  //系数
    v = a1*p->distance + c1*(p->distance - p->distance_last);
    w = w + a3*p->theta + c3*(p->theta - p->theta_last);
}
void set_destination(double* pos_c, double* pos_d){
    destination_1[0]=pos_d[0];
    destination_1[1]=pos_d[1];
    destination_1[2]=angle(pos_c,pos_d)-pos_c[2];
}


int main() {

    double pose_c[3] = {0,0,M_PI};
    double pos_d[2]={0,-2};
    contralling *p = &robot_control[3];
    double w=0, 
           v=0;
    
    set_destination(pose_c, pos_d);
    p->theta = destination_1[2];
    printf("%f %f %f\n",destination_1[2],pose_c[2],p->theta);
    p->distance = sqrt(distance_square(pose_c, destination_1));
    controller(v,w,p);
    p->distance_last=p->distance;
    p->theta_last=p->theta;

    /*output*/
    printf("%d\n", frame_id);
    printf("forward %d %f\n", 3, 0);
    printf("rotate %d %f\n", 3, w);

    return 0;
}
