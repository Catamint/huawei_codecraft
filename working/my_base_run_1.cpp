#include <iostream>
#include<cstdio>
#include<cmath>
#include<vector>
using namespace std;

/*工作台*/
struct table{
    int kind;           //种类 (1-9)
    double pos[2];      //位置向量 { 0:x  1:y } (m)
    int prd_cd;         //商品剩余cd: (帧)
    int mtr_bag[8];     //已有原料: [1,8] (二进制位描述: 48=110000={4,5})
    int prd_bag;        //已有商品: {y:1, n:0}
} crafting[50];
vector<table*> crafts[10];  //按种类分

/*机器人*/
struct rob{
    int craft_id;       //当前所处工作台: (NO crafting in 0.4m ? -1 : [0,K-1])
    int destination;    //去往的工作台id {no destination ? -1 : [0,K-1]}
    int action;         //动作 {0:buy, 1:sell}
    int carring;            //携带的物品编号: [0,7]
    double time_factor;    //时间系数: [0.8,1]
    double crash_factor;   //碰撞系数: [0.8,1]
    double w;            //角速度: rad/s
    double v[2];         //{ 0:v_x, 1:v_y } (m/s)
    double pose[3];      //{ 0:x, 1:y, 2:theta_rad_[-PI,PI] }
} robot[4];

/*跟踪参数*/
struct contralling{
    double theta;     //角度
    double distance;  //距离
    double theta_last;  //上一个角度
    double distance_last;   //上一个距离
} robot_control[3];

char line[1024];        //输入行
char mymap[100][100];   //地图 (1格 = 0.5m)
int frame_id;           //帧ID
int current_money;      //当前的金钱
int K;                  //crafting个数

double destination_1[3];    //测试,机器人1的目的地

FILE *test_file;        //测试文件


//void 初始化: robot[0:3].destination=-1;


bool readOK() {
    // fgets(line, sizeof line, stdin);
    scanf("%s", line);
    if(line[0]=='\n') return readOK();
    if (line[0] == 'O' && line[1] == 'K') return true;
    return false;
}

void readCrafting(int i){
    int mtr_bag_b;
    scanf("%d", &crafting[i].kind);
    scanf("%lf%lf", &crafting[i].pos[0], &crafting[i].pos[1]);
    scanf("%d", &crafting[i].prd_cd);
    scanf("%d", &mtr_bag_b);                        //二进制转列表(未验证)
    for(int j=0;j<9;j++){
        crafting[i].mtr_bag[j] = mtr_bag_b % 2;
        mtr_bag_b=mtr_bag_b>>1;
    }
    scanf("%d", &crafting[i].prd_bag);
}
void linkCrafting(){
    for(int i=0;i<K;i++){
        crafts[crafting[i].kind].push_back(&crafting[i]);
    }
}
void printCrafting__(int i){
    printf("%d ", crafting[i].kind);
    printf("%f%f ", crafting[i].pos[0], crafting[i].pos[1]);
    printf("%d ", crafting[i].prd_cd);
    printf("%d ", crafting[i].mtr_bag);
    printf("%d ", crafting[i].prd_bag);
    printf("\n");
}

void readRobot(int i){
    scanf("%d", &robot[i].craft_id);
    scanf("%d", &robot[i].carring);
    scanf("%lf", &robot[i].time_factor);
    scanf("%lf", &robot[i].crash_factor);
    scanf("%lf", &robot[i].w);
    scanf("%lf%lf", &robot[i].v[0], &robot[i].v[1]);
    scanf("%lf", &robot[i].pose[2]);
    if(robot[i].pose[2]<0) robot[i].pose[2]=2*M_PI+robot[i].pose[2];
    scanf("%lf%lf", &robot[i].pose[0], &robot[i].pose[1]);
}
void printRobot__(int i){
    printf("%d ", robot[i].craft_id);
    printf("%d ", robot[i].carring);
    printf("%f ", robot[i].time_factor);
    printf("%f ", robot[i].crash_factor);
    printf("%f ", robot[i].w);
    printf("%f %f ", robot[i].v[0], robot[i].v[1]);
    printf("%f ", robot[i].pose[2]);
    printf("%f %f ", robot[i].pose[0], robot[i].pose[1]);
    printf("\n");
}

bool readMap(){
    int i=0;
    while (fgets(line, sizeof line, stdin)) {
        if (line[0] == 'O' && line[1] == 'K') return true;
        for(int j=0; j<100; j++) mymap[i][j]=line[j];
        i++;
    }
    printf("map illegal");
    return false;
}
void printMap__(){
    for(char i=0;i<100;i++){
        for(char j=0;j<100;j++){
            printf("%c ", mymap[i][j]);
        }
        printf("\n");
    }
}

void map_analyze(){
    char p;
    rob* rob_p=robot;
    table* cra_p=crafting;

    for(int i=0;i<100;i++){
        for(int j=0;j<100;j++){
            p=mymap[i][j];
            if(p=='.') continue;
            if(p=='A'){
                rob_p->craft_id=-1;
                rob_p->destination=-1;
                rob_p->pose[0]=99.75-0.5*i;
                rob_p->pose[1]=0.25+0.5*j;
                rob_p++;
            }else{
                cra_p->kind=p;
                cra_p->pos[0]=99.75-0.5*i;
                cra_p->pos[1]=0.25+0.5*j;
                crafts[p].push_back(cra_p);
                cra_p++;
            }
        }
    }
}

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
    const double a1=6,c1=6, a3=10,c3=10;  //系数
    w = a3*p->theta + c3*(p->theta - p->theta_last);
    v = a1*p->distance + c1*(p->distance - p->distance_last);
}
void set_destination(double* pos_c, double* pos_d){
    destination_1[0]=pos_d[0];
    destination_1[1]=pos_d[1];
    destination_1[2]=angle(pos_c,pos_d);
}
void set_loss(double *pose_c, contralling *p){
    p->distance = sqrt(distance_square(pose_c, destination_1));    
    p->theta = destination_1[2] - pose_c[2];
        if(p->theta > M_PI) p->theta=2*M_PI - p->theta;
        else if(p->theta<-M_PI) p->theta=2*M_PI + p->theta;
}

int main() {
    /*重定向stdin, 从文件输入方便调试, 判题时记得注释*/
    // if(freopen("input_map_1.txt", "r", stdin) == NULL){
    //     printf("undefined file");
    //     return -1;
    // }

    /*初始化*/
    if(!readMap()) return -1; //读入地图
    map_analyze();
    puts("OK");
    fflush(stdout);

    /*帧区间*/
    while (scanf("%d", &frame_id) != EOF) {

        /*input*/
        scanf("%d", &current_money);
        scanf("%d", &K);
        for(int i=0;i<K;i++){  //注意, 若不事先读第一帧, 从0开始
            readCrafting(i); // 读入工作台 (*K)
            // printCrafting__(i);
        }
        for(int i=0;i<4;i++){
            readRobot(i);   //读入机器人 (*4)
            // printRobot__(i);
        }
        if(readOK()==false) {
            printf("input error");  //检测输入
            return -1;
        }

        /*action*/
        printf("%d\n", frame_id);

        //for c in robot:
        int robot_id=3;
        rob *c=&robot[robot_id];
        contralling *p=&robot_control[robot_id];
        table *d;
        double w=c->w, 
               v=sqrt(inner_square(c->v));
        
        if(frame_id == 1){
            c->action=0;
            c->destination=30;
        }   //后续添加到初始化函数

        if(c->craft_id == c->destination){
            //sell
            if(c->action==1){
                printf("sell %d\n", robot_id);
                c->action=0; //frush action
            }else if(c->action==0){
                printf("buy %d\n", robot_id);
                c->action=1; //frush action
            }
            //set destination
            c->destination=c->destination==20 ? 30:20;
        }
        /*后续加碰撞检测*/

        /*pid*/
        d = &crafting[c->destination];
        set_destination(c->pose, d->pos);
        set_loss(c->pose, p);
        controller(v,w,p);
        p->distance_last=p->distance;
        p->theta_last=p->theta;

        printf("forward %d %f\n", 3, v);
        printf("rotate %d %f\n", 3, w);

        printf("OK\n", frame_id);
        fflush(stdout);
    }
    return 0;
}
