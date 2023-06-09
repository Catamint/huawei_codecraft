#include <iostream>
#include<cstdio>
#include<cmath>
#include<vector>
#include<set>
#include<algorithm>
using namespace std;

/*工作台*/
struct table{
    int id;                      //id
    int kind;                    //种类 (1-9)
    double pos[2];               //位置向量 { 0:x  1:y } (m)
    int prd_cd;                  //商品剩余cd: (帧)
    int mtr_bag;              //已有原料: [1,8] (二进制位描述: 48=110000={4,5})
    int mtr_bag_offline[8];      //已有原料(线下)
    int prd_bag;                 //已有商品: {y:1, n:0}
    vector<table*> cra_means;   //临近的工作台
    vector<table*> cra_down;    //下一流程的工作台
    double temp_distance;       //temp
} crafting[50];     //[0,K-1]

vector<table*> crafts_by_kind[10];      //按种类分

// vector<table*> crafts_with_good;        //当前有商品的工作台(后续改成等待时间小于到达时间的)

/*机器人*/
struct rob{
    int craft_id;           //当前所处工作台: (NO crafting in 0.4m ? -1 : [0,K-1])
    table *destination;     //去往的工作台 {no destination ? -1 : [0,K-1]}
    int action;             //动作 {0:buy, 1:sell}
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

double destination_1[4][3];    //测试,机器人的目的地

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
    scanf("%d", &crafting[i].kind);
    scanf("%lf%lf", &crafting[i].pos[0], &crafting[i].pos[1]);
    scanf("%d", &crafting[i].prd_cd);
    scanf("%d", &crafting[i].mtr_bag);                        //二进制转列表(未验证)
    scanf("%d", &crafting[i].prd_bag);
}
void printCrafting__(int i){
    printf("%d ", crafting[i].kind);
    printf("%f %f ", crafting[i].pos[0], crafting[i].pos[1]);
    printf("%d [", crafting[i].prd_cd);
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
    int cra_id=0;
    table* cra_p=crafting;

    for(int i=0;i<100;i++){
        for(int j=0;j<100;j++){
            p=mymap[i][j];
            if(p=='.') continue;
            if(p=='A'){
                rob_p->craft_id=-1;
                rob_p->destination=NULL;
                rob_p->pose[0]=99.75-0.5*i;
                rob_p->pose[1]=0.25+0.5*j;
                rob_p++;
            }else{
                cra_p->id=cra_id;
                cra_p->kind=(int)(p-'0');
                cra_p->pos[0]=99.75-0.5*i;
                cra_p->pos[1]=0.25+0.5*j;
                crafts_by_kind[(int)(p-'0')].push_back(cra_p);
                cra_p++; cra_id++;
            }
        }
    }
    K=cra_id;
}

bool sorting(const table *p, const table *q){
    return p->temp_distance < q->temp_distance;
}

double distance_square(double *pos_c, double *pos_d){
    return pow((pos_d[0]-pos_c[0]),2)+pow((pos_d[1]-pos_c[1]),2);
}

void set_downstream(){
    int next_kind[10][4]={{0,0,0,0},{3,4,5,9},{3,4,6,9},{3,5,6,9},
                                    {2,7,9,0},{2,7,9,0},{2,7,9,0},
                                    {2,8,9,0},{0,0,0,0},{0,0,0,0}};
    table *crafting_p=crafting;
    for(int i=0; i<K; i++){
        vector<table*> &cra_down = crafting_p->cra_down;
        int *next_p=next_kind[crafting_p->kind];
        for(int j=1;j<next_p[0]+1;j++){
            cra_down.insert(cra_down.end(),
                            crafts_by_kind[next_p[j]].begin(),
                            crafts_by_kind[next_p[j]].end());
        }
        for(int j=0;j<cra_down.size();j++){
            cra_down[j]->temp_distance=distance_square(crafting_p->pos,cra_down[j]->pos);
        }
        sort(cra_down.begin(), cra_down.end(), sorting);
        crafting_p++;
    }
}

void set_means(){
    table *crafting_i=crafting;
    for(int i=0;i<K;i++){
        table *crafting_j=crafting;
        for(int j=0;j<K;j++){
            crafting_i->cra_means.push_back(crafting_j);
            crafting_j->temp_distance=distance_square(crafting_i->pos,crafting_j->pos);
            crafting_j++;
        }
        sort(crafting_i->cra_means.begin(), crafting_i->cra_means.end(), sorting);
        crafting_i++;
    }
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
    const double a1=3,c1=3,ax=-3, a3=10,c3=10;  //系数
    w = a3*p->theta + c3*(p->theta - p->theta_last);
    v = a1*p->distance + c1*(p->distance - p->distance_last);
}
void set_destination(double* pos_c, double* pos_d, int rid){
    destination_1[rid][0]=pos_d[0];
    destination_1[rid][1]=pos_d[1];
    destination_1[rid][2]=angle(pos_c,pos_d);
}
void set_loss(double *pose_c, contralling *p, int rid){
    p->distance = sqrt(distance_square(pose_c, destination_1[rid]));    
    p->theta = destination_1[rid][2] - pose_c[2];
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
    set_downstream();
    set_means();
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

        int robot_id=0;
        // for (;robot_id<4;robot_id++)  //test
        {
            rob *c = &robot[robot_id];
            contralling *p = &robot_control[robot_id];
            table *current_craft = &crafting[c->craft_id];
            double w=c->w, 
                v=sqrt(inner_square(c->v));
            
            if(frame_id == 1){
                c->action=0;
                c->destination=crafts_by_kind[robot_id+1][3];
            }   //后续添加到初始化函数

            if(current_craft->id == c->destination->id){
                //sell
                if(c->action==1){
                    printf("sell %d\n", robot_id);
                    c->action=0; //frush action
                    for(int i=0;i<K;i++){
                        if(current_craft->cra_means[i]->prd_bag){
                            c->destination = current_craft->cra_means[i];
                            //flag
                            break;
                        }
                    }
                    //if !flag action=-1;

                }else if(c->action==0){
                    printf("buy %d\n", robot_id);
                    c->action=1; //frush action
                    for(int i=0;i<current_craft->cra_down.size();i++){
                        if(((current_craft->cra_down[i]->mtr_bag)>>(c->destination->kind))%2==0){
                            c->destination = current_craft->cra_down[i];
                            //flag
                            break;
                        }
                    }
                    //if !flag action=-1;
                }
                //set destination
            }
            /*后续加碰撞检测*/

            /*pid*/
            set_destination(c->pose, c->destination->pos, robot_id);
            set_loss(c->pose, p, robot_id);
            controller(v,w,p);
            p->distance_last=p->distance;
            p->theta_last=p->theta;

            printf("forward %d %f\n", robot_id, v);
            printf("rotate %d %f\n", robot_id, w);
        }

        printf("OK\n", frame_id);
        fflush(stdout);
    }
    return 0;
}
