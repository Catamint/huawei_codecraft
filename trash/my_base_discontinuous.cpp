#include <iostream>
#include<cstdio>
#include<cmath>
#include<vector>
#define PI 3.14

using namespace std;

/*工作台*/
struct table{
    int kind;           //种类 (1-9)
    double r_x;         //位置向量 x: (m)
    double r_y;         //位置向量 y: (m)
    int prd_cd;         //商品剩余cd: (帧)
    int mtr_bag[8];     //已有原料: [1,8] (二进制位描述: 48=110000={4,5})
    int prd_bag;        //已有商品: {y:1, n:0}
} crafting[50];
vector<table*> crafts[10];  //按种类分

/*机器人*/
struct {
    int craft_id;       //当前所处工作台: (NO crafting in 0.4m ? -1 : [0,K-1])
    int carring;            //携带的物品编号: [0,7]
    double time_factor;    //时间系数: [0.8,1]
    double crash_factor;   //碰撞系数: [0.8,1]
    double w;    //角速度: rad/s
    double v_x;         //速度 x: (m/s)
    double v_y;         //速度 y: (m/s)
    double angle;       //弧度: [-PI,PI] (rad)
    double r_x;         //位置向量: x
    double r_y;         //位置向量: y
} robot[4];

char line[1024];        //输入行
char mymap[100][100];   //地图 (1格 = 0.5m)
int frame_id;           //帧ID
int current_money;      //当前的金钱
int K;                  //crafting个数

FILE *test_file;        //测试文件

bool readUntilOK() {
    while (fgets(line, sizeof line, stdin)) {
        if (line[0] == 'O' && line[1] == 'K') {
            return true;
        }
    }
    return false;
}
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
    scanf("%lf%lf", &crafting[i].r_x, &crafting[i].r_y);
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
    printf("%f%f ", crafting[i].r_x, crafting[i].r_y);
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
    scanf("%lf%lf", &robot[i].v_x, &robot[i].v_y);
    scanf("%lf", &robot[i].angle);
    scanf("%lf%lf", &robot[i].r_x, &robot[i].r_y);
}
void printRobot__(int i){
    printf("%d ", robot[i].craft_id);
    printf("%d ", robot[i].carring);
    printf("%f ", robot[i].time_factor);
    printf("%f ", robot[i].crash_factor);
    printf("%f ", robot[i].w);
    printf("%f %f ", robot[i].v_x, robot[i].v_y);
    printf("%f ", robot[i].angle);
    printf("%f %f ", robot[i].r_x, robot[i].r_y);
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
    
}

/*第一帧读入(temp)*/
bool read_first_frame(){
    scanf("%d", &frame_id);
    scanf("%d", &current_money);
    scanf("%d", &K);
    for(int i=0;i<K;i++)
        readCrafting(i); // 读入工作台 (*K)
    for(int i=0;i<4;i++)
        readRobot(i);   //读入机器人 (*4)
    if(readOK()==false) {
        printf("input error");  //检测输入
        return -1;
    }
    linkCrafting();
    printf("%d\n", frame_id);
    printf("OK\n", frame_id);
    fflush(stdout);
    return true;
}

double distance_square(double x_c, double y_c, double x_d, double y_d){
    return pow((x_c+x_d),2)+pow((y_c+y_d),2);
}
double angle(double x_c, double y_c, double x_d, double y_d){
    return atan((y_d-y_c)/(x_d-x_c));
}

int main() {
    /*重定向stdin, 从文件输入方便调试, 判题时记得注释*/
    // if(freopen("input_map_1.txt", "r", stdin) == NULL){
    //     printf("undefined file");
    //     return -1;
    // }

    /*初始化*/
    if(!readMap()) return -1; //读入地图
    // printMap__();
    puts("OK");
    fflush(stdout);

    /*第一帧读入(temp)*/
    // read_first_frame();

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

        /*compute*/
        int robot_id=3, crafting_id=21;
        double angleSpeed=0, lineSpeed=0;
        if(frame_id<300) lineSpeed=4;
        else lineSpeed=0;

        /*output*/
        printf("%d\n", frame_id);
        printf("forward %d %f\n", 3, lineSpeed);
        printf("rotate %d %f\n", 3, angleSpeed);
        printf("OK\n", frame_id);
        fflush(stdout);
    }
    return 0;
}
