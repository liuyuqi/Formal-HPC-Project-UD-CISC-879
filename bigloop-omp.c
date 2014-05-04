#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>

#define NUM 100 /*任务节点个数*/
//#define MIU 1/NUM /*权值折算时的系数*/

float MIU;
int var;
#pragma omp threadprivate(MIU, var)

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

#pragma omp threadprivate (testnode, dag, dag_new, dag_dynamic, e_power, e_power_new)

typedef struct
{
    int cid;
    int curr_node;
    int last_node;
    int idle; 
    int remain;
} core_t;

/*处理器核心数目 Number of cores on a processor*/
#define NCORE 8

/*建立一个多核处理器 A multi-core processor */
core_t core[NCORE];

int task;
int ticks;
int need_schedule = 0;
int ready_list[NUM + 1];
int cpufree;

#pragma omp threadprivate(core, task, ticks, need_schedule, ready_list, cpufree)

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
#pragma omp threadprivate(ga)

/*处理器模型的初始化*/
/*Initialization of the processor model*/
void core_init ()
{
    int i;
    for (i = 0; i < NCORE; i++) {
        core[i].cid = i;
        core[i].curr_node = -1;
        core[i].last_node = -1;
        core[i].idle = 0;
        core[i].remain = 0;
    }     
}

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

/*将DAG补全成AOE网络*/
/*Make the DAG an Activity-on-Edge network*/
void dag_makeup()
{
    int i, j;
    for (i = 0; i < NUM; i++) {
        if (testnode[i].end_tag == 1)
            dag_new[i][NUM] = 1;
    }
    for (i = 0; i < NUM; i++) 
        for (j = 0; j < NUM; j++) {
            dag_new[i][j] = dag[i][j];
        }

    for (i = 0; i < NUM + 1; i++) {
        for (j = 0; j < NUM + 1; j++) {
            dag_dynamic[i][j] = dag_new[i][j];
        }
    }
}

/*DAG图权值的变换*/
/*Conversion of the powers on nodes to that on edges*/
void convert_dag ()
{
    int i, j;
    for (i = 0; i < NUM; i++) {
        for (j = 0; j < NUM; j++) {
            if (dag[i][j] != 0) {
                e_power_new[i][j] = e_power[i][j] * MIU + testnode[i].node_power;
            }
            else {
                e_power_new[i][j] = e_power[i][j];
            }
        }
    }
    for (i = 0; i <= NUM; i++) {
        if (testnode[i].end_tag == 1)
            e_power_new[i][NUM] = testnode[i].node_power;
    }
}

void dag_print() 
{
    int i, j;
    for (i = 0; i < NUM + 1; i++) {
        for (j = 0; j < NUM + 1; j++) {
            printf("%d\t", dag_new[i][j]);
        }
        printf("\n");
    }    
}

void createadjlist ()
{
    int i, j, k;
    edgenode *s;
    for (i = 0; i < NUM + 1; i++) {
        ga[i].vertex = testnode[i];
        ga[i].id = 0;
        ga[i].link = NULL;
    }

    for (i = 0; i < NUM + 1; i++) {
        for (j = 0; j < NUM + 1; j++) {
            if (dag_new[i][j] == 1) {
                ga[j].id++;
                testnode[j].id++;
                s = (edgenode*) malloc (sizeof (edgenode));
                s -> adjvex = j;
                s -> dur = e_power_new[i][j];
                s -> next = ga[i].link;
                ga[i].link = s;
            }
        }
    }
}

/*The algorithm of calculating the critical path of a DAG.*/
void critical_path ()
{
    long i, j, k, m;
    long front = -1, rear = -1;
    long tpord[NUM+1], vl[NUM+1], ve[NUM+1];
    long l[100000], e[100000];
    edgenode *p;
    for (i = 0; i < NUM+1; i++)
        ve[i] = 0;
    for (i = 0; i < NUM+1; i++) {
        if (ga[i].id == 0) {
            tpord[++rear] = i;
        }
    }
    m = 0;
    while (front != rear) {
        front ++;
        j = tpord[front];
        m++;
        p = ga[j].link;
        while (p) {
            k = p->adjvex;
            ga[k].id --;
            if (ve[j] + p->dur > ve[k])
                ve[k] = ve[j] + p->dur;
            if (ga[k].id == 0) {
                tpord[++rear] = k;
            }
            p = p -> next;
        }
    }

    if (m < NUM + 1) {
        printf("The AOE network has a cycle\n");
        return 0;
    }
    for (i = 0; i < NUM + 1; i++)
        vl[i] = ve[NUM];
    for (i = NUM - 1; i >= 0; i--) {
        j = tpord[i];
        p = ga[j].link;
        while (p) {
            k = p -> adjvex;
            if ( (vl[k] - p->dur) < vl[j])
                vl[j] = vl[k] - p->dur;
            p = p -> next;
        }
    }
    i = 0;
    for (j = 0; j < NUM + 1; j++) {
        p = ga[j].link;
        while (p) {
            k = p -> adjvex;
            e[++i] = ve[j];
            l[i] = vl[k] - p->dur;
            if (l[i] == e[i]) {
                testnode[j].critical = 1;
                testnode[j].priority = 2;
                testnode[k].critical = 1;
                testnode[k].priority = 2;
            }
            p = p -> next;
        }
    }
    return 1;
}

/*ready_list的初始化*/
/*The initialization of ready list*/
void list_init ()
{
    int i;
    for (i = 0; i < NUM; i++) 
        ready_list[i] = 0;
}

/*判断是否所有任务都已执行完毕*/
/*Judge whether all tasks have been scheduled.*/
int task_not_empty ()
{
    int i;
    for (i = 0; i < NUM + 1; i++) {
        if (testnode[i].scheduled == 0)
            return 1;
    }
    return 0;
}

/*检查ready_list中是否有给定优先级的任务*/
/*Check whether there still exist tasks with the given priority value in the ready list.*/
int exist_pri (int pri)
{
    int i;
    for (i = 0; i < NUM + 1; i++) {
        if (testnode[i].scheduled == 0 && ready_list[i] == 1 && testnode[i].priority == pri)
            return 1;
    }
    return 0;
}

/*从ready_list中取出一个给定优先级的任务*/
/*Pick a task from the ready list with the given priority value.*/
int pick_pri (int pri)
{
    int i;
    for (i = 0; i < NUM + 1; i++) {
        if (testnode[i].scheduled == 0 && ready_list[i] == 1 && testnode[i].priority == pri)
            return i;
    }
}

/*调度算法主体*/
/*The main body of the scheduling algorithm*/
int algorithm () {
    int i, coreid;
    int des_core;
    for (ticks = 0; task_not_empty(); ticks++) {
        for (coreid = 0; coreid < NCORE; coreid++) {
            if (core[coreid].idle == 0 && core[coreid].remain > 0)
                core[coreid].remain--;
            if (core[coreid].remain == 0 && core[coreid].idle == 0) {
                core[coreid].idle = 1;
                core[coreid].last_node = core[coreid].curr_node;
                core[coreid].curr_node = -1;
                /*如果之前有任务执行，则将之前任务发出的有向边去掉*/
                /*If there was some tasks executing on the core, then eliminate the directed edge 
                 * from the previous task (vertex).*/
                if (core[coreid].last_node != -1) {
                    for (i = 0; i < NUM + 1; i++) {
                        if (dag_new[core[coreid].last_node][i] == 1) {
                            dag_dynamic [core[coreid].last_node][i] = 0; 
                            testnode[i].id--;
                        } 
                    }
                } 
            }
        }

        need_schedule = 0;
        need_schedule = find_next_node ();
        if (need_schedule) {
            des_core = -1;
            while ( check_free_core () && exist_pri (3)) {
                task = pick_pri (3);
                des_core = -1;
                des_core = search_core_satisfy (task);
                if (des_core > -1)  schedule_to (task, des_core);
                else  schedule (task);
            }
            while ( check_free_core () && exist_pri (2) ) {
                task = pick_pri (2);
                schedule (task);
            }
            while ( check_free_core () && exist_pri (1) ) {
                task = pick_pri (1);
                des_core = -1;
                des_core = search_core_satisfy (task);
                if (des_core > -1)  schedule_to (task, des_core);
                else  schedule (task);
            }
            while ( check_free_core () && exist_pri (0) ) {
                task = pick_pri (0);
                schedule (task);
            }	
        }
    }
    int large = 0;
    return ticks;
}

/*检查是否有空闲的处理器核*/
/*Check whether there are some cores available*/
int check_free_core ()
{
    int i;
    for (i = 0; i < NCORE; i++) {
        if (core[i].idle == 1)  {
            return 1;
        }
    }
    return 0;
}

/*寻找刚执行完毕任务的子任务，没有返回0，有返回1*/
/*Seek the child tasks of the task that just finished execution.
 * On found return 1, otherwise return 0.*/
int find_next_node ()
{
    int i, j, k, flagtotal;
    int last = -1;
    flagtotal = 0;
    for (i = 0; i < NCORE; i++) {
        if (core[i].idle == 1) {
            last = core[i].last_node;
        }
        else continue;

        if (ticks != 0) {
            for (j = 0; j < NUM + 1; j++) {
                if ( testnode[j].id == 0 && testnode[j].scheduled == 0) {
                    flagtotal = 1;
                    if (testnode[j].to_schedule == 0 && dag_new[last][j] == 1) {
                        if (testnode[j].priority == 2) 
                            testnode[j].priority = 3;
                        else if (testnode[j].priority == 0) 
                            testnode[j].priority = 1;
                    }
                    ready_list[j] = 1;
                    testnode[j].to_schedule = 1;
                }
            }
        }
        else {
            for (i = 0; i < NUM + 1; i++) {
                if (testnode[i].id == 0) {
                    ready_list[i] = 1;
                    testnode[i].to_schedule = 1;
                    flagtotal = 1;
                }
            }
        }
    }
    return flagtotal;
}

int search_core_satisfy (int task)
{
    int i, prev_core = -1;
    for (i = 0; i < NUM + 1; i++) {
        if (dag_new[i][task] != dag_dynamic[i][task]) {
            prev_core = testnode[i].dest_core;
            if (core[prev_core].idle == 1) return prev_core;
        }   
    }
    return -1;
}

void schedule (int task)
{
    int i, j, large;
    for (i = 0; i < NCORE; i++) {
        if (core[i].idle == 1) {
            testnode[task].dest_core = i;

            testnode[task].scheduled = 1;
            testnode[task].to_schedule = 0;
            core[i].idle = 0;
            core[i].curr_node = task;
            core[i].remain = testnode[task].node_power;
            if (dag_new[core[i].last_node][task] == 0 ) {
                large = 0;
                for (j = 0; j < NUM + 1; j++) {
                    if (dag_new[j][task] == 1 && large < e_power[j][task])
                        large = e_power[j][task];
                }
                core[i].remain += large;
            }
            if (task == NUM)
                large = 0;
            testnode[task].start_time = ticks + large;
            testnode[task].cost = large;
            ready_list[task] = 0;
            break;
        }
    }
}

void schedule_to (int task, int to_core)
{
    int large, j;
    testnode[task].dest_core = to_core;

    testnode[task].scheduled = 1;
    testnode[task].to_schedule = 0;
    core[to_core].idle = 0;
    core[to_core].curr_node = task;
    core[to_core].remain = testnode[task].node_power;
    if (dag_new[core[to_core].last_node][task] == 0 ) {
        large = 0;
        for (j = 0; j < NUM + 1; j++) {
            if (dag_new[j][task] == 1 && large < e_power[j][task])
                large = e_power[j][task];
        }
        core[to_core].remain += large;
    }
    if (task == NUM)
        large = 0;
    testnode[task].start_time = ticks + large;
    testnode[task].cost = large;
    ready_list[task] = 0;
}

void calculate ()
{
    printf("Task\tCore\tStart\tStop\tPri\n");
    int i;
    for (i = 0; i < NUM + 1; i++) {
        printf("%d\t%d\t%d\t%d\t%d\n", testnode[i].node_number, testnode[i].dest_core,
                testnode[i].start_time+testnode[i].cost, 
                testnode[i].start_time+testnode[i].node_power+testnode[i].cost,
                testnode[i].priority);
    }  
}

void node_recover () 
{
    int i = 0;
    node_t *p_node = testnode;
    for (i = 0; i < NUM + 1; i++) {
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

int main (void)
{
    int test, result_idx;
    int result[101];
    long result_total[101];
    for (result_idx = 0; result_idx <= 100; result_idx++) {
        result_total[result_idx] = 0;
    }
    for (test = 0; test < 4; test++) {
        for (result_idx = 0; result_idx <= 100; result_idx++) {
            result[result_idx] = 0;
        }
        printf("test %d started...\n", test);
        node_t* p_node = testnode; /*节点指针 pointer to a vertex (node)*/
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

        /*调度算法实施阶段*/
        /*Stage of implementing scheduling algorithm.*/
        
/*BEGINNING OF PARALLEL REGION*/
#pragma omp parallel for default (none)\
		firstprivate (result)\
		shared (result_total)
        for (var = 0; var <= 100; var++) {
            MIU = (float)var / 100.0;
            core_init ();
            node_recover ();
            list_init ();
            convert_dag (); /*DAG图权值的变换 Transfer the power from vertices to edges*/
            createadjlist ();
            critical_path ();  /*AOE关键路径的求解 Solving the critical path.*/
            result[var] = algorithm ();
            result_total[var] += result[var];
        } /*END OF PARALLEL REGION*/

		printf("================================\n");
		printf("The running result of test No.%d is :\n", test);
		printf("--------------------------------\n");
        for (result_idx = 0; result_idx <= 100; result_idx++) {
            printf("When MIU = %.2f, total time is %d\n", result_idx/100.0, result_total[result_idx]);
        }
	}   
    return 0;
}

