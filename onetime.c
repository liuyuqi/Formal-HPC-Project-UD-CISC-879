#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM 100 /*����ڵ����*/
//#define MIU 1/NUM /*Ȩֵ����ʱ��ϵ��*/

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
} node_t; /*����ڵ�*/
node_t testnode [NUM + 1]; /*���Խڵ�*/

int dag [NUM][NUM]; /*DAGͼ���ڽӾ���*/
int dag_new [NUM+1][NUM+1]; /*ת������ڽӾ���*/
int dag_dynamic [NUM+1][NUM+1]; /*���������ִ���Լ����ڽӾ���*/
int e_power [NUM+1][NUM+1]; /*DAGͼ�бߵ�Ȩֵ����*/
int e_power_new [NUM+1][NUM+1]; /*ת����ı�Ȩֵ����*/

typedef struct
{
    int cid;
    int curr_node;
    int last_node;
    int idle; 
    int remain;
} core_t;
#define NCORE 8/*������������Ŀ*/
core_t core[NCORE];/*����һ����˴�����*/


int task;
int ticks;
int need_schedule = 0;
int ready_list[NUM + 1];
int cpufree;

/********�ڽӱ�����*********/
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

/*������ģ�͵ĳ�ʼ��*/
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

/*�ڵ�ĳ�ʼ��*/
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

/*DAGͼ�ڽӾ���ĳ�ʼ��*/ 
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

/*DAGͼ��Ȩֵ����ĳ�ʼ��*/
void epower_init () {
    int i;
    int* p_epower = e_power;
    int* p_epower_new = e_power_new;
    for (i = 0; i < (NUM+1)*(NUM+1); i++) {
        *(p_epower++) = 0;    
        *(p_epower_new++) = 0;
    }     
}

/*������ɽڵ�Ȩֵ*/
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

/*DAGͼ�ڽӾ�����������*/ 
void dag_create () {
    int i = 0, j = 0;
    int temp_sum = 0, flag = 0; 
    dag[0][1] = 1; /*һ�������������������һ�ڵ�����б�*/
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
        /*�ձ�Լ�������ж�*/ 
        
        flag = 0;
        for (j = 2; j < NUM; j++) {
        	temp_sum = 0; /*����׼��*/
        	for (i = 0; i < j; i++)
        		temp_sum += dag[i][j]; /*ѭ���ۼ�*/
        	
        	if (temp_sum == 0) { /*�Ƿ��������*/
        		flag = 1;
                break;
        	} 
        }
        
        if (flag == 0) {
        	epower_init ();
        	break;
        }
    }
    /*   �ж��յ㣨���ٷ����ߵĵ㣩����� */
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

/*DAGͼ��Ȩֵ���������*/
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

/*��DAG��ȫ��AOE����*/
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

/*DAGͼȨֵ�ı任*/
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

/*ready_list�ĳ�ʼ��*/
void list_init ()
{
    int i;
	for (i = 0; i < NUM; i++) 
	    ready_list[i] = 0;
}

/*�ж��Ƿ�����������ִ�����*/
int task_not_empty ()
{
    int i;
	for (i = 0; i < NUM + 1; i++) {
	    if (testnode[i].scheduled == 0)
		    return 1;
	}
	return 0;
}

/*���ready_list���Ƿ��и������ȼ�������*/
int exist_pri (int pri)
{
    int i;
	for (i = 0; i < NUM + 1; i++) {
	    if (testnode[i].scheduled == 0 && ready_list[i] == 1 && testnode[i].priority == pri)
		    return 1;
	}
	return 0;
}

/*��ready_list��ȡ��һ���������ȼ�������*/
int pick_pri (int pri)
{
    int i;
	for (i = 0; i < NUM + 1; i++) {
	    if (testnode[i].scheduled == 0 && ready_list[i] == 1 && testnode[i].priority == pri)
		    return i;
	}
}

/*�����㷨����*/
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
                /*���֮ǰ������ִ�У���֮ǰ���񷢳��������ȥ��*/
                if ( core[coreid].last_node != -1) {
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

/*����Ƿ��п��еĴ�������*/
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

/*Ѱ�Ҹ�ִ����������������û�з���0���з���1*/
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
            //printf("Task %d scheduled to core %d!\n", task, testnode[task].dest_core);
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
    //printf("Task %d scheduled to core %d!\n", task, testnode[task].dest_core);
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
    int test;
    long result_total[101];
    for (test = 0; test < 101; test ++) {
        result_total[test] = 0;
    }
    for (test = 0; test < 1; test ++) {
    printf("test %d started\n", test);
    node_t* p_node = testnode; /*�ڵ�ָ��*/
    /*��ʼ���׶�*/
    node_init (p_node); /*�ڵ�ĳ�ʼ��*/
    dag_init (); /*DAGͼ�ڽӾ���ĳ�ʼ��*/ 
    epower_init (); /*DAGͼ��Ȩֵ����ĳ�ʼ��*/
    core_init ();	/*������ģ�͵ĳ�ʼ��*/
    
    /*��������ɽ׶�*/
    p_node = testnode;
    empower_node (p_node);/*������ɽڵ�Ȩֵ*/
    dag_create ();/*DAGͼ�ڽӾ�����������*/ 
    empower_epower ();/*DAGͼ��Ȩֵ���������*/
    dag_makeup (); /*��DAG��ȫ��AOE����*/
    //dag_print ();
    /*�����㷨ʵʩ�׶�*/
    
    int result[101];
    for (var = 0; var <= 0; var++) {
        MIU = (float)var / 100.0;
        core_init ();	/*������ģ�͵ĳ�ʼ��*/
        node_recover (); /*�ڵ�ĳ�ʼ��*/
        list_init ();
        convert_dag (); /*DAGͼȨֵ�ı任*/
        createadjlist ();
        critical_path ();  /*AOE�ؼ�·�������*/
        result[var] = algorithm ();
        result_total[var] += result[var];
        //printf("Trial %d' total time is %d\n", var, result[var]);
        //printf("algorithm completed\n");
        calculate ();
    }
    }
    for (test = 0; test <= 1; test ++) {
        printf("MIU %d' total time is %d\n", test, result_total[test]);
    }
    
    return 0;
}

