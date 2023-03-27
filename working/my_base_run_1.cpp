#include <iostream>
#include<cstdio>
#include<cmath>
#include<vector>
#include<algorithm>
using namespace std;

/*工作台*/
struct table{
    int id;                      //id
    int kind;                    //种类 (1-9)
    double pos[2];               //位置向量 { 0:x  1:y } (m)
    int prd_cd;                  //商品剩余cd: (帧)
    int mtr_bag[9];              //已有原料: [1,8] (二进制位描述: 48=110000={4,5})
    int mtr_bag_offline[9];      //已有原料(线下)
    int prd_bag;                 //已有商品: {y:1, n:0}
    int prd_bag_offline;
    vector<table*> cra_means;   //临近的工作台
    vector<table*> cra_down;    //下一流程的工作台
    double temp_distance;       //temp
} crafting[55];     //[0,K-1]
vector<table*> crafts_by_kind[10];      //按种类分

/*机器人*/
struct rob{
    int craft_id;           //当前所处工作台: (NO crafting in 0.4m ? -1 : [0,K-1])
    table *destination;     //下一次去往的工作台 {no destination ? -1 : [0,K-1]}
    table *destination_sell;//下一次去出售的工作台 {no destination ? -1 : [0,K-1]}
    int action;             //动作 {0:buy, 1:sell}
    int carring;            //携带的物品编号: [0,7]
    double time_factor;    //时间系数: [0.8,1]
    double crash_factor;   //碰撞系数: [0.8,1]
    double w;            //角速度: rad/s
    double v[2];         //{ 0:v_x, 1:v_y } (m/s)
    double pose[3];      //{ 0:x, 1:y, 2:theta_rad_[-PI,PI] }
} robot[5];
double destination_1[5][3];    //机器人的目的地, (考虑加入robot)

/*跟踪参数*/
struct contralling{
    double theta;     //角度
    double distance;  //距离
    double theta_last;  //上一个角度
    double distance_last;   //上一个距离
} robot_control[5];

char line[1024];        //输入行
char mymap[100][100];   //地图 (1格 = 0.5m)
int frame_id;           //帧ID
int current_money;      //当前的金钱
int K;                  //crafting个数
FILE *test_file;        //测试文件

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
    int next_kind[10][4]={{0,0,0,0},{3,4,5,0},{3,4,6,0},{3,5,6,0},
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

/*针对不同地图优化的craft_down排序*/
void set_downstream_map1(){
    int next_kind[10][4]={{0,0,0,0},{3,4,5,0},{3,4,6,0},{3,5,6,0},
                                    {2,7,0,0},{2,7,0,0},{2,7,0,0},
                                    {2,8,9,0},{0,0,0,0},{0,0,0,0}};
    table *crafting_p=crafting;
    for(int i=0; i<K; i++){
        vector<table*> &cra_down = crafting_p->cra_down;
        int *next_p=next_kind[crafting_p->kind];
        vector<table*>::iterator t[3];
        vector<table*>::iterator e[3];
        for(int j=1;j<next_p[0]+1;j++){
            t[j]=crafts_by_kind[next_p[j]].begin();
            e[j]=crafts_by_kind[next_p[j]].end();
        }
        while (1){
            int flag=0;
            if(t[1]!=e[1]){ cra_down.push_back(*t[1]++);    flag=1;}
            if(t[2]!=e[2]){ cra_down.push_back(*t[2]++);    flag=1;}
            if(t[3]!=e[3]){ cra_down.push_back(*t[3]++);    flag=1;}
            if(flag==0) break;
        }
        crafting_p++;
    }
}

/*根据距离对下一流程工作台排序, 确定下一次卖出的地点*/
void set_downstream_map4(){
    int next_kind[10][4]={{0,0,0,0},{3,4,5,0},{3,4,6,0},{3,5,6,0},
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
        if(crafting_p->id==2 || crafting_p->id==6){
            cra_down.insert(cra_down.begin(), &crafting[17]);
        }
        crafting_p++;
    }
}

void set_downstream_map2(){
    int next_kind[10][4]={{0,0,0,0},{3,4,5,0},{3,4,6,0},{3,5,6,0},
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
        if(crafting_p->id==5 || crafting_p->id==7){
            cra_down.insert(cra_down.begin(), &crafting[0]);
        }
        if(crafting_p->id==17 || crafting_p->id==19){
            cra_down.insert(cra_down.begin(), &crafting[24]);
        }
        crafting_p++;
    }
}

/*根据距离对邻近工作台排序, 确定下一次购买的地点*/
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
    const double a1=10,c1=6,ax=0.5, a3=20,c3=20;  //系数
    w = a3*p->theta + c3*(p->theta - p->theta_last);
    v = ax/(abs(w)+ax) * (a1*p->distance + c1*(p->distance - p->distance_last));
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
void initialize(){
    for(int i=0;i<K;i++){
        crafting[i].prd_bag_offline=1;
    }
}
int search_path(rob *c){
    table* current_craft = &crafting[c->craft_id];
    for(int i=0;i<K;i++){
        if(current_craft->cra_means[i]->prd_bag &&
           current_craft->cra_means[i]->prd_bag_offline)
        {
            table* temp_desti=current_craft->cra_means[i];
            for(int j=0;j<temp_desti->cra_down.size();j++){
                if(temp_desti->cra_down[j]->mtr_bag[temp_desti->kind]==0 &&
                   temp_desti->cra_down[j]->mtr_bag_offline[temp_desti->kind]==0){
                        c->destination=temp_desti;
                        c->destination_sell=temp_desti->cra_down[j];
                        return 1;
                   }
            }
        }
    }
    return -1;
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
    if(K==43){
        set_downstream_map1();
    }else if(K==25){
        set_downstream_map2();
    }else if(K==18){
        set_downstream_map4();
    }else{
        set_downstream();
    }
    
    set_means();
    initialize();
    puts("OK");
    fflush(stdout);

    /*帧区间*/
    while (scanf("%d", &frame_id) != EOF) {

        /*input*/
        scanf("%d", &current_money);
        scanf("%d", &K);
        for(int i=0;i<K;i++){  //注意, 若不事先读第一帧, 从0开始
            readCrafting(i); // 读入工作台 (*K)
        }
        for(int i=0;i<4;i++){
            readRobot(i);   //读入机器人 (*4)
        }
        if(readOK()==false) {
            printf("input error");  //检测输入
            return -1;
        }

        /*action*/
        printf("%d\n", frame_id);
        
        for (int robot_id=0;robot_id<4;robot_id++)  //test
        {
            rob *c = &robot[robot_id];
            contralling *p = &robot_control[robot_id];
            table *current_craft = &crafting[c->craft_id];
            double w=c->w, 
                v=sqrt(inner_square(c->v));
            
            if(frame_id == 1){
                c->action=1;
                c->destination=&crafting[robot_id*K/4];
            }   //后续添加到初始化函数

            if(current_craft->id == c->destination->id){
                //sell
                if(c->action==1){
                    if(c->carring) printf("sell %d\n", robot_id);
                    current_craft->mtr_bag_offline[c->carring]=0;
                    current_craft->mtr_bag[c->carring]=1;

                    if(frame_id<8750/*最后5秒不买商品*/ && search_path(c)==1){
                        c->destination->prd_bag_offline=0;
                        c->destination_sell->mtr_bag_offline[c->destination->kind]=1;
                        c->action=0; //frush action
                    }

                }else if(c->action==0){
                    printf("buy %d\n", robot_id);
                    c->action=1; //frush action

                    current_craft->prd_bag_offline=1;
                    current_craft->prd_bag=0;

                    c->destination=c->destination_sell;
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
