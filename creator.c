#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM 100 /*任务节点个数*/
//#define MIU 1/NUM /*权值折算时的系数*/

float MIU;
int var;

typedef struct
{
    int node_number;
    int node_power;
    int end_tag;
    int id;
    int priority;
    int critical;
    int to_schedule;
    int scheduled;
    int dest_core;
    int start_time;
    int cost;
} node_t; /*任务节点 The data structure for task nodes.*/

node_t testnode [NUM + 1]; /*测试节点 A list of all the tasks.*/

/*DAG图的邻接矩阵  The adjacent matrix of the DAG*/
int dag [NUM][NUM]; 
/*转换后的邻接矩阵  The adjacent matrix of the DAG after conversion*/
int dag_new [NUM+1][NUM+1]; 
/*用于任务可执行性检测的邻接矩阵 
 * The adjacent matrix of the DAG used for checking the eligibility of execution of tasks.*/
int dag_dynamic [NUM+1][NUM+1]; 
/*DAG图中边的权值矩阵  The matrix of power on edges in the DAG*/
int e_power [NUM+1][NUM+1]; 
/*转换后的边权值矩阵  The matrix of power on edges after transfer */
int e_power_new [NUM+1][NUM+1]; 

typedef struct
{
    int cid;
    int curr_node;
    int last_node;
    int idle; 
    int remain;
} core_t;

/*邻接表定义区*/
/*Definitions of adjacent table*/
typedef node_t vextype;
typedef struct node {
    int adjvex;
    int dur;
    struct node *next;
}edgenode;
typedef struct {
    vextype vertex;
    int id;
    edgenode *link;
}vexnode;
vexnode ga[NUM+1]; 

/*节点的初始化*/
/*Initialization of nodes (or vertices)*/
void node_init (node_t* p_node)
{
    int i = 0;
    for (i = 0; i < NUM + 1; i++) {
        p_node -> node_number = i;
        p_node -> node_power = -1;
        p_node -> end_tag = 0;
        p_node -> id = 0;
        p_node -> priority = 0;
        p_node -> critical = 0;
        p_node -> to_schedule = 0;
        p_node -> scheduled = 0;
        p_node -> dest_core = -1;
        p_node -> start_time = -1;
        p_node -> cost = 0;
        ++ p_node;
    }
    -- p_node;
}

/*DAG图邻接矩阵的初始化*/ 
/*Initialization of the adjacent matrix of DAG*/
void dag_init () {
    int i;
    int* p_dag = dag; 
    int* p_dag_new = dag_new; 
    int* p_dag_dynamic = dag_dynamic;  
    for (i = 0; i < NUM*NUM; i++) {
        *(p_dag++) = 0;
        *(p_dag_new++) = 0;
        *(p_dag_dynamic++) = 0;
    }
    for (; i< (NUM+1)*(NUM+1); i++) {
        *(p_dag_new++) = 0;
        *(p_dag_dynamic++) = 0;
    }
}

/*DAG图边权值矩阵的初始化*/
/*Initialization of the matrix of the edge's power of DAG*/
void epower_init () {
    int i;
    int* p_epower = e_power;
    int* p_epower_new = e_power_new;
    for (i = 0; i < (NUM+1)*(NUM+1); i++) {
        *(p_epower++) = 0;    
        *(p_epower_new++) = 0;
    }     
}

/*随机生成节点权值*/
/*Randomly produces the power values of vertices*/
void empower_node (node_t* p_node) {
    srand (time (NULL));
    int i = 0;
    for (i = 0; i < NUM; i++) {
A:      p_node -> node_power = rand() % 100;
        if (p_node -> node_power == 0)
            goto A;
        ++ p_node;
    }     
}

/*DAG图邻接矩阵的随机生成*/ 
/*Randomly produces the adjacent matrix of DAG*/
void dag_create () {
    int i = 0, j = 0;
    int temp_sum = 0, flag = 0; 
    dag[0][1] = 1; /*一个必须满足的条件，第一节点必须有边*/
    /*The first node must have an edge setting off from it.*/
    dag[NUM - 2][NUM - 1] = 1;
    srand (time (NULL));

    while (1) {
        for (i = 0; i < NUM - 1; i++) {
            for (j = i + 1; j < NUM; j++) {
                if (i == 0 && j == 1 ) 
                    continue;
                dag[i][j] = rand () % 2;
            }    
        }

        /*终边约束条件判断*/ 
        /*Judges whether the final edge is legal*/
        flag = 0;
        for (j = 2; j < NUM; j++) {
            temp_sum = 0; /*计算准备*/
            for (i = 0; i < j; i++)
                temp_sum += dag[i][j]; /*循环累加*/
            if (temp_sum == 0) { /*非法条件检测*/
                flag = 1;
                break;
            } 
        }
        if (flag == 0) {
            epower_init ();
            break;
        }
    }
    /* 判断终点（不再发出边的点）并标记 */
    /*Judges and marks the end edges.*/
    for (i = 0; i < NUM; i++) {
        temp_sum = 0;
        for (j = 0; j < NUM; j++) {
            temp_sum += dag[i][j];
        }
        if (temp_sum == 0) {
            testnode[i].end_tag = 1;
        }
    }
}

/*DAG图边权值的随机生成*/
/*Randomly produces the edges' power values of the DAG.*/
void empower_epower ()
{
    srand (time (NULL));

    int i, j;
    for (i = 0; i < NUM; i++) { 
        for (j = 0; j < NUM; j++) {
            if (dag[i][j] != 0) 
                e_power[i][j] = rand() % 100 + 16;
        }
    } 
}

int main (void)
{
    int test, result_idx;
    int result[101];
    node_t *p_node = NULL;
    long result_total[101];
    for (result_idx = 0; result_idx <= 100; result_idx++) {
        result_total[result_idx] = 0;
    }
    for (test = 0; test < 50; test++) {
        for (result_idx = 0; result_idx <= 100; result_idx++) {
            result[result_idx] = 0;
        }
        printf("test %d started\n", test);
        p_node = testnode; /*节点指针 pointer to a vertex (node)*/
        /*初始化阶段*/
        /*Initialization stage*/
        node_init (p_node); /*节点的初始化 Initialization on nodes*/
        dag_init (); /*DAG图邻接矩阵的初始化 init on adjacent matrix of DAG*/ 
        epower_init (); /*DAG图边权值矩阵的初始化 init on edges' power matrix of DAG*/
        core_init ();	/*处理器模型的初始化 init on the processor model */

        /*随机数生成阶段*/
        /*Stage of random number production*/
        p_node = testnode;
        empower_node (p_node);/*随机生成节点权值 Randomly produces the power of nodes*/
        dag_create ();/*DAG图邻接矩阵的随机生成 Randomly produces the adjacent matrix of the DAG.*/ 
        empower_epower ();/*DAG图边权值的随机生成 Randomly produces the power of edges.*/
        dag_makeup (); /*将DAG补全成AOE网络 Convert the DAG to Activity-on-Edge network*/
        //dag_print ();
    }
    return 0;
}

