#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROCESS 10
// circular queue code section
#define TRUE	1
#define FALSE	0

#define QUE_LEN	100

typedef struct eval{
	float avg_turnaround;
	float avg_waiting;
} evaluation;

evaluation fcfs, sjf_np, sjf_p, priority_np, priority_p, rr, aging_priority_np, aging_priority_p, multilevel, multilevel_absolute, multilevel_feedback_absolute, hrrn, lrtf_np, lrtf_p;

//process description
typedef struct _process {
	int pid;
	int cpu_bt;
	int io_bt;
	int arrival_time;
	int priority;
	int progress;
	int io_start;
	int new_arrival_time;
	int remain_cpu_time;
	int remain_io_time;
	int time_quantum;
	int turnaround_time;
	int waiting_time;
	int aging_time;
	int original_priority;
}Process;


typedef Process* Data;

typedef struct _cQueue
{
	int front;
	int rear;
	int num_data;
	Data queArr[QUE_LEN];
} CQueue;

typedef CQueue Queue;

void QueueInit(Queue * pq)
{
	pq->front = 0;
	pq->rear = 0;
	pq->num_data = 0;
}

int QIsEmpty(Queue * pq)
{
	if (pq->front == pq->rear)
		return TRUE;
	else

		return FALSE;
}

int NextPosIdx(int pos)
{
	if (pos == QUE_LEN - 1)
		return 0;
	else
		return pos + 1;
}

void Enqueue(Queue * pq, Data data)
{
	if (NextPosIdx(pq->rear) == pq->front)
	{
		printf("Queue Memory Error!");
		exit(-1);
	}
	
	pq->rear = NextPosIdx(pq->rear);
	pq->queArr[pq->rear] = data;
	pq->num_data++;
}

Data Dequeue(Queue * pq)
{
	if (QIsEmpty(pq))
	{
		printf("Queue Memory Error!");
		exit(-1);
	}
	pq->num_data--;
	pq->front = NextPosIdx(pq->front);
	return pq->queArr[pq->front];
}

Data QPeek(Queue * pq)
{
	if (QIsEmpty(pq))
	{
		printf("Queue Memory Error!");
		exit(-1);
	}

	return pq->queArr[NextPosIdx(pq->front)];
}

Queue CopyQueue(Queue* pq)
{
	return *pq;
}


//Ready queue, running queue, process_info
Queue ready_q;
Queue running_q;
//PQueue ready_q;
//PQueue running_q;
Process * process[MAX_PROCESS];
int num_process;
int waiting_q[MAX_PROCESS];



void Create_Process() {
	printf("enter number of processes to make: ");
	scanf("%d", &num_process);
	printf("------------------------------------------------------------------------------------------\n");
	srand(time(NULL));
	
	

	for (int i = 0; i < num_process; i++)
	{
		

		Process  * process_p = (Process*)malloc(sizeof(Process));
		process_p->pid = i;
		process_p->cpu_bt = (int)(rand() % 8 + 2); 
		process_p->io_bt = (int)(rand() % 6 + 1);
		//process_p->cpu_bt = (int)(rand() % 12 + 2);
		//process_p->io_bt = (int)(rand() % 8 + 1);
		process_p->arrival_time = (int)(rand() % (num_process * 3));
		//process_p->arrival_time = 0;
		process_p->progress =0;
		process_p->aging_time = 0;
		//priority의 경우에는 중복을 허용해선 안된다. 중복을 허용하지 않고 1부터 10까지의 random priority를 각 process한테 부여한다.

		

		process_p->io_start = (int)(rand() % (process_p->cpu_bt-1)+1); // cpu 할당 받고 바로 i/o 연산하러 가지 않음
		process_p->new_arrival_time = process_p->arrival_time;
		process_p->remain_cpu_time = process_p->cpu_bt;
		process_p->remain_io_time = process_p->io_bt;
		process_p->time_quantum = 0;
		process_p->turnaround_time = 0;
		process_p->waiting_time = 0;
		process[i] = process_p;

		int ran_priority = (rand() % MAX_PROCESS) + 1; // 1~10 까지의 random priority 생성
		//int ran_priority = (rand() % 30) + 1;
		process_p->priority = ran_priority;
		process_p->original_priority = process_p->priority;
		
		/*for (int j = 0; j < i; j++)
		{
			if (ran_priority == process[j]->priority)
			{
				i--; // 다시 priority 설정을 위해
				break;
			}
		}
		*/

		
		
	}


	//적어도 하나의 process는 arrival time이 0이 되도록 설정
	int check_at = 0;
	for (int i = 0; i < num_process; i++)
	{
		if (process[i]->arrival_time == 0)
			check_at = 1;
	}

	if (check_at == 0)
	{
		int i = rand() % num_process;
		process[i]->arrival_time = 0;
	}

	printf("PID    CPU_BURST_TIME    IO_BURST_TIME    ARRIVAL_TIME   IO_START_TIME     PRIORITY\n");
	printf("------------------------------------------------------------------------------------------\n");
	for (int i = 0; i < num_process; i++)
	{
		printf("%d %10d        %10d        %10d        %10d      %10d\n", process[i]->pid, process[i]->cpu_bt, process[i]->io_bt, process[i]->arrival_time, process[i]->io_start, process[i]->priority);
	}

	printf("------------------------------------------------------------------------------------------\n");


}

void Config() {
	QueueInit(&ready_q);
	QueueInit(&running_q);

	for (int i = 0; i < num_process; i++)
	{
		waiting_q[i] = -1;
	}
}

void Initialize() { // 이걸 해야하는 이유는?
	for (int i = 0; i < num_process; i++)
	{
		waiting_q[i] = -1;
		process[i]->progress = 0;
		process[i]->remain_cpu_time = process[i]->cpu_bt;
		process[i]->remain_io_time = process[i]->io_bt;
		process[i]->time_quantum = 0;
		process[i]->turnaround_time = 0;
		process[i]->waiting_time = 0;
		process[i]->aging_time = 0;
		process[i]->priority = process[i]->original_priority;
	}
}

void Aging(Process * pq)
{
	
	if (pq->priority == 1)
	{
		pq->priority = 1;
	}
	else
		pq->priority--;
	
}





void FCFS() {
	printf("implementation of FCFS\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check=0;
	

	for(time = 0; check != num_process; time++)
	{ 
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				Enqueue(&ready_q, process[i]);
			}
			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				waiting_q[i]--;
				Enqueue(&ready_q, process[i]);
			}

		}
		if (QIsEmpty(&running_q) && !QIsEmpty(&ready_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
		{
			Process * run_process = Dequeue(&ready_q);
			Enqueue(&running_q, run_process);
			
			
		}

		if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
		{
			Process * run_process = QPeek(&running_q);
			printf("TIME[%d]~TIME[%d]  cpu : P[%d]\n", time, time+1, run_process->pid);
			run_process->progress++;
				
			
			if (run_process->progress == run_process->cpu_bt) // running process가 cpu_burst를 complete 한다면
			{
				Process * finish_p = Dequeue(&running_q);
				check++;
				finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
			}
			else if (run_process->progress == run_process->io_start)
			{
				waiting_q[run_process->pid] = run_process->io_bt;
				Dequeue(&running_q);

			}
		}
		else
		{
			printf("TIME[%d]~TIME[%d]  cpu : IDLE\n", time, time+1);
		}
		
	}

	printf("------------------------------------------------------------------------------------------\n");

	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	fcfs.avg_turnaround = avg_turnaround;
	fcfs.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");
}


void SJF_NP()
{
	printf("implementation of non preemptive sjf\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check = 0;
	

	for (time = 0; check != num_process; time++)
	{
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				if(QIsEmpty(&ready_q))
					Enqueue(&ready_q, process[i]);
				else {
					Enqueue(&ready_q, process[i]);

					//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k-1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->remain_cpu_time > temp_p[n + 1]->remain_cpu_time) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}

					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}



				}
			}
			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				waiting_q[i]--;
				if (QIsEmpty(&ready_q))
				{
					Enqueue(&ready_q, process[i]);
					process[i]->new_arrival_time = time;
				}
				else {
					Enqueue(&ready_q, process[i]);
					process[i]->new_arrival_time = time;
					//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k-1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->remain_cpu_time > temp_p[n + 1]->remain_cpu_time) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}
					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}
					
				}
			}

		}
		if (QIsEmpty(&running_q) && !QIsEmpty(&ready_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
		{
			Process * run_process = Dequeue(&ready_q);
			Enqueue(&running_q, run_process);


		}

		if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
		{
			Process * run_process = QPeek(&running_q);
			printf("TIME[%d]~TIME[%d]  cpu : P[%d]\n", time, time + 1, run_process->pid);
			
			

			run_process->progress++;
			run_process->remain_cpu_time--;


			if (run_process->progress == run_process->cpu_bt)
			{
				Process * finish_p = Dequeue(&running_q);
				finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
				check++;
			}
			else if (run_process->progress == run_process->io_start)
			{
				waiting_q[run_process->pid] = run_process->io_bt;
				Dequeue(&running_q);

			}
		}
		else
		{
			printf("TIME[%d]~TIME[%d]  cpu : IDLE\n", time, time + 1);
		}

	}

	printf("------------------------------------------------------------------------------------------\n");

	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	sjf_np.avg_turnaround = avg_turnaround;
	sjf_np.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");


}

void SJF_P() {
	printf("implementation of preemptive sjf\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check = 0;
	

	for (time = 0; check != num_process; time++)
	{
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				if (QIsEmpty(&ready_q)) {
					Enqueue(&ready_q, process[i]);

					if (!QIsEmpty(&running_q)){ // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->remain_cpu_time < run_process->remain_cpu_time) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);

						}
					  }

				}


			
				else {
					Enqueue(&ready_q, process[i]);

					//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->remain_cpu_time > temp_p[n + 1]->remain_cpu_time) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}

					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}


					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->remain_cpu_time < run_process->remain_cpu_time) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);


							//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
							Process * temp_p[MAX_PROCESS];
							int k = 0;
							while (!QIsEmpty(&ready_q))
							{
								temp_p[k] = Dequeue(&ready_q);
								k++;
							}

							Process * temp;
							for (int m = k - 1; m > 0; m--) {
								for (int n = 0; n < m; n++) {
									if (temp_p[n]->remain_cpu_time > temp_p[n + 1]->remain_cpu_time) {
										temp = temp_p[n];
										temp_p[n] = temp_p[n + 1];
										temp_p[n + 1] = temp;
									}
								}
							}

							for (int m = 0; m < k; m++)
							{
								Enqueue(&ready_q, temp_p[m]);
							}
								
							}

						}
					}
				}
			
			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				waiting_q[i]--;
				if (QIsEmpty(&ready_q)){
					Enqueue(&ready_q, process[i]);


					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->remain_cpu_time < run_process->remain_cpu_time) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);


						}

					}



				}
				else {
					Enqueue(&ready_q, process[i]);
					

					//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->remain_cpu_time > temp_p[n + 1]->remain_cpu_time) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}

					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}
					

					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->remain_cpu_time < run_process->remain_cpu_time) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);

							//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
							Process * temp_p_2[MAX_PROCESS];
							int k = 0;
							while (!QIsEmpty(&ready_q))
							{
								temp_p_2[k] = Dequeue(&ready_q);
								k++;
							}

							Process * temp;
							for (int m = k - 1; m > 0; m--) {
								for (int n = 0; n < m; n++) {
									if (temp_p_2[n]->remain_cpu_time > temp_p_2[n + 1]->remain_cpu_time) {
										temp = temp_p_2[n];
										temp_p_2[n] = temp_p_2[n + 1];
										temp_p_2[n + 1] = temp;
									}
								}
							}

							for (int m = 0; m < k; m++)
							{
								Enqueue(&ready_q, temp_p_2[m]);
							}


						}

					}
				}
			}

		}
		if (QIsEmpty(&running_q) && !QIsEmpty(&ready_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
		{
			Process * run_process = Dequeue(&ready_q);
			Enqueue(&running_q, run_process);


		}

		if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
		{
			Process * run_process = QPeek(&running_q);
			printf("TIME[%d]~TIME[%d]  cpu : P[%d]\n", time, time + 1, run_process->pid);
			run_process->progress++;
			run_process->remain_cpu_time--;


			if (run_process->progress == run_process->cpu_bt)
			{
				Process * finish_p = Dequeue(&running_q);
				finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
				check++;
			}
			else if (run_process->progress == run_process->io_start)
			{
				waiting_q[run_process->pid] = run_process->io_bt;
				Dequeue(&running_q);

			}
		}
		else
		{
			printf("TIME[%d]~TIME[%d]  cpu: IDLE\n", time, time + 1);
		}


		

	}

	printf("------------------------------------------------------------------------------------------\n");

	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	sjf_p.avg_turnaround = avg_turnaround;
	sjf_p.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");
}


void PRIORITY_NP()
{
	printf("implementation of non preemptive priority\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check = 0;
	

	for (time = 0; check != num_process; time++)
	{
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				if (QIsEmpty(&ready_q))
					Enqueue(&ready_q, process[i]);
				else {
					Enqueue(&ready_q, process[i]);

					//ready_q에 process를 enqueue 할때 마다 priority를 기준으로 sorting하기 , priority는 숫자가 작으면 우선권 있는거
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->priority > temp_p[n + 1]->priority) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}

					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}



				}
			}
			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				waiting_q[i]--;
				if (QIsEmpty(&ready_q))
				{
					Enqueue(&ready_q, process[i]);
					process[i]->new_arrival_time = time;
				}
				else {
					Enqueue(&ready_q, process[i]);
					process[i]->new_arrival_time = time;
					//ready_q에 process를 enqueue 할때 priority를 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->priority > temp_p[n + 1]->priority) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}
					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}

				}
			}

		}
		if (QIsEmpty(&running_q) && !QIsEmpty(&ready_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
		{
			Process * run_process = Dequeue(&ready_q);
			Enqueue(&running_q, run_process);


		}

		if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
		{
			Process * run_process = QPeek(&running_q);
			printf("TIME[%d]~TIME[%d]  cpu : P[%d]\n", time, time + 1, run_process->pid);

			

			run_process->progress++;
			run_process->remain_cpu_time--;


			if (run_process->progress == run_process->cpu_bt)
			{
				Process * finish_p = Dequeue(&running_q);
				finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
				check++;
			}
			else if (run_process->progress == run_process->io_start)
			{
				waiting_q[run_process->pid] = run_process->io_bt;
				Dequeue(&running_q);

			}
		}
		else
		{
			printf("TIME[%d]~TIME[%d]  cpu: IDLE\n", time, time + 1);
		}

	}

	printf("------------------------------------------------------------------------------------------\n");



	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	priority_np.avg_turnaround = avg_turnaround;
	priority_np.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");

}


void PRIORITY_P() {
	printf("implementation of preemptive priority\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check = 0;
	

	for (time = 0; check != num_process; time++)
	{
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				if (QIsEmpty(&ready_q)) {
					Enqueue(&ready_q, process[i]);

					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->priority < run_process->priority) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);

						}
					}

				}



				else {
					Enqueue(&ready_q, process[i]);

					//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->priority > temp_p[n + 1]->priority) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}

					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}


					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->priority < run_process->priority) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);


							//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
							Process * temp_p[MAX_PROCESS];
							int k = 0;
							while (!QIsEmpty(&ready_q))
							{
								temp_p[k] = Dequeue(&ready_q);
								k++;
							}

							Process * temp;
							for (int m = k - 1; m > 0; m--) {
								for (int n = 0; n < m; n++) {
									if (temp_p[n]->priority > temp_p[n + 1]->priority) {
										temp = temp_p[n];
										temp_p[n] = temp_p[n + 1];
										temp_p[n + 1] = temp;
									}
								}
							}

							for (int m = 0; m < k; m++)
							{
								Enqueue(&ready_q, temp_p[m]);
							}

						}

					}
				}
			}

			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				waiting_q[i]--;
				if (QIsEmpty(&ready_q)) {
					Enqueue(&ready_q, process[i]);


					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->priority < run_process->priority) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);


						}

					}



				}
				else {
					Enqueue(&ready_q, process[i]);


					//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->priority > temp_p[n + 1]->priority) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}

					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}


					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->priority < run_process->priority) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);

							//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
							Process * temp_p_2[MAX_PROCESS];
							int k = 0;
							while (!QIsEmpty(&ready_q))
							{
								temp_p_2[k] = Dequeue(&ready_q);
								k++;
							}

							Process * temp;
							for (int m = k - 1; m > 0; m--) {
								for (int n = 0; n < m; n++) {
									if (temp_p_2[n]->priority > temp_p_2[n + 1]->priority) {
										temp = temp_p_2[n];
										temp_p_2[n] = temp_p_2[n + 1];
										temp_p_2[n + 1] = temp;
									}
								}
							}

							for (int m = 0; m < k; m++)
							{
								Enqueue(&ready_q, temp_p_2[m]);
							}


						}

					}
				}
			}

		}
		if (QIsEmpty(&running_q) && !QIsEmpty(&ready_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
		{
			Process * run_process = Dequeue(&ready_q);
			Enqueue(&running_q, run_process);


		}

		if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
		{
			Process * run_process = QPeek(&running_q);
			printf("TIME[%d]~TIME[%d]  cpu: P[%d]\n", time, time + 1, run_process->pid);

		
			run_process->progress++;
			run_process->remain_cpu_time--;


			if (run_process->progress == run_process->cpu_bt)
			{
				Process * finish_p = Dequeue(&running_q);
				finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
				check++;
			}
			else if (run_process->progress == run_process->io_start)
			{
				waiting_q[run_process->pid] = run_process->io_bt;
				Dequeue(&running_q);

			}
		}
		else
		{
			printf("TIME[%d]~TIME[%d]  cpu: IDLE\n", time, time + 1);
		}

	}

	printf("------------------------------------------------------------------------------------------\n");
	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	priority_p.avg_turnaround = avg_turnaround;
	priority_p.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");


}


void RR() {
	printf("implementation of ROUND ROBIN\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check = 0;
	
	int current_time_quantum;
	printf("how would you like the time quantum to be? : ");
	while (1) {
		scanf("%d", &current_time_quantum);
		if (current_time_quantum > 5 || current_time_quantum < 1) printf("[ERROR] Time Quantum must be 1 to 4. Please enter time quantum again: ");
		else break;
	}


	for (time = 0; check != num_process; time++)
	{
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				Enqueue(&ready_q, process[i]);
			}
			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				waiting_q[i]--;
				Enqueue(&ready_q, process[i]);
			}

		}
		if (QIsEmpty(&running_q) && !QIsEmpty(&ready_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
		{
			Process * run_process = Dequeue(&ready_q);
			Enqueue(&running_q, run_process);


		}

		if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
		{
			Process * run_process = QPeek(&running_q);
			printf("TIME[%d]~TIME[%d]  cpu: P[%d]\n", time, time + 1, run_process->pid);
			run_process->progress++;
			run_process->time_quantum++;
			if(QIsEmpty(&ready_q))
			{
				printf("ready_1는 아무것도 없지렁!\n\n");
			}
			if (run_process->progress == run_process->cpu_bt)
			{
				Process * finish_p = Dequeue(&running_q);
				finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
				check++;
			}
			else if (run_process->progress == run_process->io_start)
			{
				waiting_q[run_process->pid] = run_process->io_bt;
				run_process->time_quantum = 0;
				Dequeue(&running_q);

			}
			else if (run_process->time_quantum == current_time_quantum)
			{
				run_process->time_quantum = 0;
				Enqueue(&ready_q, (Dequeue(&running_q)));
			}
		}
		else // when running_Q is empty & ready_Q is empty too
		{
			printf("TIME[%d]~TIME[%d]  cpu: IDLE\n", time, time + 1);
		}

	}

	printf("------------------------------------------------------------------------------------------\n");
	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	rr.avg_turnaround = avg_turnaround;
	rr.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");
}

void AGING_PRIORITY_NP()
{
	printf("implementation of non preemptive priority\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check = 0;


	for (time = 0; check != num_process; time++)
	{
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				printf("%d has entered ready_Q at time %d \n",process[i]->pid, time);
				if (QIsEmpty(&ready_q))
					Enqueue(&ready_q, process[i]);
				else {
					Enqueue(&ready_q, process[i]);

					//ready_q에 process를 enqueue 할때 마다 priority를 기준으로 sorting하기 , priority는 숫자가 작으면 우선권 있는거
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->priority > temp_p[n + 1]->priority) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}

					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}



				}
			}
			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				//printf("%d IO COMPLETION!! \n", process[i]->pid);
				waiting_q[i]--;
				if (QIsEmpty(&ready_q))
				{
					Enqueue(&ready_q, process[i]);
					process[i]->new_arrival_time = time;
				}
				else {
					Enqueue(&ready_q, process[i]);
					process[i]->new_arrival_time = time;

					//ready_q에 process를 enqueue 할때 priority를 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->priority > temp_p[n + 1]->priority) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}
					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}

				}
			}

		}
		
		
		if (QIsEmpty(&running_q) && !QIsEmpty(&ready_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
		{
			Process * run_process = Dequeue(&ready_q);
			Enqueue(&running_q, run_process);
			run_process->aging_time = 0; // aging 시간을 초기화
			printf("process has just started running\n");

		}

		if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
		{
			Process * run_process = QPeek(&running_q);
			printf("TIME[%d]~TIME[%d]  cpu : P[%d]\n", time, time + 1, run_process->pid);



			run_process->progress++;
			run_process->remain_cpu_time--;


			if (run_process->progress == run_process->cpu_bt)
			{
				Process * finish_p = Dequeue(&running_q);
				finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
				check++;
				printf("process has been completed\n");
			}
			else if (run_process->progress == run_process->io_start)
			{
				waiting_q[run_process->pid] = run_process->io_bt;
				Dequeue(&running_q);
				printf("IO START!!\n");

			}
		}
		else
		{
			printf("TIME[%d]~TIME[%d]  cpu: IDLE\n", time, time + 1);
		}

		if (!QIsEmpty(&ready_q)) // aging 관련 처리
		{

			Process * temp_age[MAX_PROCESS];
			int age_num = 0;
			while (!QIsEmpty(&ready_q))
			{
				temp_age[age_num] = Dequeue(&ready_q);
				age_num++;
			}

			for (int a = 0; a < age_num; a++)//현재 시간에 대해 ready_q에 있는 애들의 aging_time이 max time에 도달하면 priority를 수정
			{
				temp_age[a]->aging_time++;
			}

			for (int a = 0; a < age_num; a++)  
			{
				if (temp_age[a]->aging_time == 5) // aging을 하기 위해 ready_Q에 잇어야 하는 시간
				{
					int past = temp_age[a]->priority;
					temp_age[a]->aging_time = 0;
					Aging(temp_age[a]);
					//printf("내 나이가 어때서~ P[%d], PRIORITY %d -> %d \n",temp_age[a]->pid, past, temp_age[a]->priority );
				}
				else
				{
					//printf("현재 내가 ready_q에 있는 시간은 P[%d], AGING TIME = %d\n", temp_age[a]->pid, temp_age[a]->aging_time);
				}
			}
			//printf("\n");
			Process * temp_a;
			for (int m = age_num - 1; m > 0; m--) {
				for (int n = 0; n < m; n++) {
					if (temp_age[n]->priority > temp_age[n + 1]->priority) {//동일한 priority를 가지고 있으면 ready_q에 먼저 들어간 애부터 집어넣어준다. 이때 ready_q에 먼저 들어가는 process는 먼저 생성된 process이다.(PID 순)
						temp_a = temp_age[n];
						temp_age[n] = temp_age[n + 1];
						temp_age[n + 1] = temp_a;
					}
				}
			}


			for (int m = 0; m < age_num; m++)
			{
				Enqueue(&ready_q, temp_age[m]);
			}

		}
		else {
			printf("ready_Q is empty\n");

			printf("\n");
		}


	}

	printf("------------------------------------------------------------------------------------------\n");



	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	aging_priority_np.avg_turnaround = avg_turnaround;
	aging_priority_np.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");

}


void AGING_PRIORITY_P() {
	printf("implementation of preemptive priority\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check = 0;


	for (time = 0; check != num_process; time++)
	{
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				if (QIsEmpty(&ready_q)) {
					Enqueue(&ready_q, process[i]);

					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->priority < run_process->priority) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);

						}
					}

				}



				else {
					Enqueue(&ready_q, process[i]);

					//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->priority > temp_p[n + 1]->priority) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}

					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}


					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->priority < run_process->priority) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);


							//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
							Process * temp_p[MAX_PROCESS];
							int k = 0;
							while (!QIsEmpty(&ready_q))
							{
								temp_p[k] = Dequeue(&ready_q);
								k++;
							}

							Process * temp;
							for (int m = k - 1; m > 0; m--) {
								for (int n = 0; n < m; n++) {
									if (temp_p[n]->priority > temp_p[n + 1]->priority) {
										temp = temp_p[n];
										temp_p[n] = temp_p[n + 1];
										temp_p[n + 1] = temp;
									}
								}
							}

							for (int m = 0; m < k; m++)
							{
								Enqueue(&ready_q, temp_p[m]);
							}

						}

					}
				}
			}

			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				waiting_q[i]--;
				if (QIsEmpty(&ready_q)) {
					Enqueue(&ready_q, process[i]);


					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->priority < run_process->priority) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);


						}

					}



				}
				else {
					Enqueue(&ready_q, process[i]);


					//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->priority > temp_p[n + 1]->priority) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}

					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}


					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->priority < run_process->priority) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);

							//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
							Process * temp_p_2[MAX_PROCESS];
							int k = 0;
							while (!QIsEmpty(&ready_q))
							{
								temp_p_2[k] = Dequeue(&ready_q);
								k++;
							}

							Process * temp;
							for (int m = k - 1; m > 0; m--) {
								for (int n = 0; n < m; n++) {
									if (temp_p_2[n]->priority > temp_p_2[n + 1]->priority) {
										temp = temp_p_2[n];
										temp_p_2[n] = temp_p_2[n + 1];
										temp_p_2[n + 1] = temp;
									}
								}
							}

							for (int m = 0; m < k; m++)
							{
								Enqueue(&ready_q, temp_p_2[m]);
							}


						}

					}
				}
			}

		}
		if (QIsEmpty(&running_q) && !QIsEmpty(&ready_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
		{
			Process * run_process = Dequeue(&ready_q);
			Enqueue(&running_q, run_process);
			run_process->aging_time = 0; // aging 시간을 초기화

		}

		if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
		{
			Process * run_process = QPeek(&running_q);
			printf("TIME[%d]~TIME[%d]  cpu: P[%d]\n", time, time + 1, run_process->pid);


			run_process->progress++;
			run_process->remain_cpu_time--;


			if (run_process->progress == run_process->cpu_bt)
			{
				Process * finish_p = Dequeue(&running_q);
				finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
				check++;
			}
			else if (run_process->progress == run_process->io_start)
			{
				waiting_q[run_process->pid] = run_process->io_bt;
				Dequeue(&running_q);

			}
		}
		else
		{
			printf("TIME[%d]~TIME[%d]  cpu: IDLE\n", time, time + 1);
		}

		if (!QIsEmpty(&ready_q)) // aging 관련 처리
		{

			Process * temp_age[MAX_PROCESS];
			int age_num = 0;
			while (!QIsEmpty(&ready_q))
			{
				temp_age[age_num] = Dequeue(&ready_q);
				age_num++;
			}

			for (int a = 0; a < age_num; a++)//현재 시간에 대해 ready_q에 있는 애들의 aging_time이 max time에 도달하면 priority를 수정
			{
				temp_age[a]->aging_time++;
			}

			for (int a = 0; a < age_num; a++)
			{
				if (temp_age[a]->aging_time == 4) // aging을 하기 위해 ready_Q에 잇어야 하는 시간
				{
					temp_age[a]->aging_time = 0;
					Aging(temp_age[a]);
					printf("내 나이가 어때서~ P[%d], PRIORITY %d -> %d \n", temp_age[a]->pid, (temp_age[a]->priority) + 1, temp_age[a]->priority);
				}
				else
				{
					printf("현재 내가 ready_q에 있는 시간은 P[%d], AGING TIME = %d\n", temp_age[a]->pid, temp_age[a]->aging_time);
				}
			}
			printf("\n");
			Process * temp_a;
			for (int m = age_num - 1; m > 0; m--) {
				for (int n = 0; n < m; n++) {
					if (temp_age[n]->priority > temp_age[n + 1]->priority) {//동일한 priority를 가지고 있으면 ready_q에 먼저 들어간 애부터 집어넣어준다. 이때 ready_q에 먼저 들어가는 process는 먼저 생성된 process이다.
						temp_a = temp_age[n];
						temp_age[n] = temp_age[n + 1];
						temp_age[n + 1] = temp_a;
					}
				}
			}
			for (int m = 0; m < age_num; m++)
			{
				Enqueue(&ready_q, temp_age[m]);
			}

		}
		else {
			printf("ready_Q is empty\n");

			printf("\n");
		}

	}

	printf("------------------------------------------------------------------------------------------\n");
	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	aging_priority_p.avg_turnaround = avg_turnaround;
	aging_priority_p.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");


}

void MULTILEVEL_QUEUE() {

	printf("implementation of MULTILEVEL_QUEUE\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check = 0;

	int current_time_quantum;
	printf("how would you like the time quantum to be? : ");
	while (1) {
		scanf("%d", &current_time_quantum);
		if (current_time_quantum > 5 || current_time_quantum < 1) printf("[ERROR] Time Quantum must be 1 to 4. Please enter time quantum again: ");
		else break;
	}
	Queue rr_q;
	Queue fcfs_q;
	// ready_q, runnin_q, waiting_q initialization
	QueueInit(&rr_q);
	QueueInit(&fcfs_q);
	QueueInit(&running_q);

	for (int i = 0; i < num_process; i++)
	{
		waiting_q[i] = -1;
	}


	for (time = 0; check != num_process; time++)
	{
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				if (process[i]->priority < 6) // priority 높은 애들은 rr
					Enqueue(&rr_q, process[i]);
				else
					Enqueue(&fcfs_q, process[i]);
			}
			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				waiting_q[i]--;
				if (process[i]->priority < 6) // priority 높은 애들은 rr
					Enqueue(&rr_q, process[i]);
				else
					Enqueue(&fcfs_q, process[i]);
			}

		}
		int rr_empty = 0;
		int fcfs_empty = 0;
		if (time % 10 < 8)
		{
			if (QIsEmpty(&running_q) && !QIsEmpty(&rr_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
			{
				Process * run_process = Dequeue(&rr_q);
				Enqueue(&running_q, run_process);

			}
			else if (QIsEmpty(&running_q) && !QIsEmpty(&fcfs_q)) // 만약 rr은 empty이고 fcfs은 empty가 아니라면
			{
				Process * run_process = Dequeue(&fcfs_q);
				Enqueue(&running_q, run_process);

			}


			if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
			{
				Process * run_process = QPeek(&running_q);
				printf("TIME[%d]~TIME[%d]  cpu: P[%d], rr status: %d,   fcfs status: %d   ", time, time + 1, run_process->pid, !QIsEmpty(&rr_q), !QIsEmpty(&fcfs_q));
				run_process->progress++;
				run_process->time_quantum++;
				if (run_process->priority < 6)
				{
					printf(" round robin process\n");
				}
				else
					printf(" fcfs process\n");

				if (run_process->progress == run_process->cpu_bt)
				{
					Process * finish_p = Dequeue(&running_q);
					finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
					check++;
				}
				else if (run_process->progress == run_process->io_start)
				{
					waiting_q[run_process->pid] = run_process->io_bt;
					run_process->time_quantum = 0;
					Dequeue(&running_q);

				}
				else if (run_process->priority < 6)
				{
					if (run_process->time_quantum == current_time_quantum)
					{
						run_process->time_quantum = 0;
						Enqueue(&rr_q, (Dequeue(&running_q)));
					}
				}

			}
			else
			{
				printf("TIME[%d]~TIME[%d]  cpu : IDLE\n", time, time + 1);
			}
		}
		else {
			if (QIsEmpty(&running_q) && !QIsEmpty(&fcfs_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
			{
				Process * run_process = Dequeue(&fcfs_q);
				Enqueue(&running_q, run_process);

			}
			else if (QIsEmpty(&running_q) && !QIsEmpty(&fcfs_q)) // 만약 fcfs은 empty이고 rr은 empty가 아니라면
			{
				Process * run_process = Dequeue(&rr_q);
				Enqueue(&running_q, run_process);

			}


			if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
			{
				Process * run_process = QPeek(&running_q);
				printf("TIME[%d]~TIME[%d]  cpu: P[%d], rr status: %d,   fcfs status: %d   ", time, time + 1, run_process->pid, !QIsEmpty(&rr_q), !QIsEmpty(&fcfs_q));
				if (run_process->priority < 6)
				{
					printf(" round robin process\n");
				}
				else
					printf(" fcfs process\n");
				run_process->progress++;
				run_process->time_quantum++;


				if (run_process->progress == run_process->cpu_bt)
				{
					Process * finish_p = Dequeue(&running_q);
					finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
					check++;
				}
				else if (run_process->progress == run_process->io_start)
				{
					waiting_q[run_process->pid] = run_process->io_bt;
					run_process->time_quantum = 0;
					Dequeue(&running_q);

				}
				else if (run_process->priority < 6)
				{
					if (run_process->time_quantum == current_time_quantum)
					{
						run_process->time_quantum = 0;
						Enqueue(&rr_q, (Dequeue(&running_q)));
					}
				}

			}
			else
			{
				printf("TIME[%d]~TIME[%d]  cpu : IDLE\n", time, time + 1);
			}
		}





	}

	printf("------------------------------------------------------------------------------------------\n");

	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	multilevel.avg_turnaround = avg_turnaround;
	multilevel.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");


}



void MULTILEVEL_QUEUE_ABSOLUTE() {

	printf("implementation of MULTILEVEL_QUEUE_ABSOLUTE\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check = 0;

	int current_time_quantum;
	printf("how would you like the time quantum to be? : ");
	while (1) {
		scanf("%d", &current_time_quantum);
		if (current_time_quantum > 5 || current_time_quantum < 1) printf("[ERROR] Time Quantum must be 1 to 4. Please enter time quantum again: ");
		else break;
	}
	Queue rr_q;
	Queue fcfs_q;
	// ready_q, runnin_q, waiting_q initialization
	QueueInit(&rr_q);
	QueueInit(&fcfs_q);
	QueueInit(&running_q);

	for (int i = 0; i < num_process; i++)
	{
		waiting_q[i] = -1;
	}


	for (time = 0; check != num_process; time++)
	{
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				if (process[i]->priority < 6) // priority 높은 애들은 rr
					Enqueue(&rr_q, process[i]);
				else
					Enqueue(&fcfs_q, process[i]);
			}
			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				waiting_q[i]--;
				if (process[i]->priority < 6) // priority 높은 애들은 rr
					Enqueue(&rr_q, process[i]);
				else
					Enqueue(&fcfs_q, process[i]);
			}

		}
		if (!QIsEmpty(&rr_q))
		{
			if (QIsEmpty(&running_q) && !QIsEmpty(&rr_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
			{
				Process * run_process = Dequeue(&rr_q);
				Enqueue(&running_q, run_process);

			}
			else if (QIsEmpty(&running_q) && !QIsEmpty(&fcfs_q)) // 만약 rr은 empty이고 fcfs은 empty가 아니라면
			{
				Process * run_process = Dequeue(&fcfs_q);
				Enqueue(&running_q, run_process);

			}


			if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
			{
				Process * run_process = QPeek(&running_q);
				printf("TIME[%d]~TIME[%d]  cpu: P[%d], rr status: %d,   fcfs status: %d   ", time, time + 1, run_process->pid, !QIsEmpty(&rr_q), !QIsEmpty(&fcfs_q));
				run_process->progress++;
				run_process->time_quantum++;
				if (run_process->priority < 6)
				{
					printf(" round robin process\n");
				}
				else
					printf(" fcfs process\n");

				if (run_process->progress == run_process->cpu_bt)
				{
					Process * finish_p = Dequeue(&running_q);
					finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
					check++;
				}
				else if (run_process->progress == run_process->io_start)
				{
					waiting_q[run_process->pid] = run_process->io_bt;
					run_process->time_quantum = 0;
					Dequeue(&running_q);

				}
				else if (run_process->priority < 6)
				{
					if (run_process->time_quantum == current_time_quantum)
					{
						run_process->time_quantum = 0;
						Enqueue(&rr_q, (Dequeue(&running_q)));
					}
				}

			}
			else
			{
				printf("TIME[%d]~TIME[%d]  cpu : IDLE\n", time, time + 1);
			}
		}


		else {
			if (QIsEmpty(&running_q) && !QIsEmpty(&fcfs_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
			{
				Process * run_process = Dequeue(&fcfs_q);
				Enqueue(&running_q, run_process);

			}
			else if (QIsEmpty(&running_q) && !QIsEmpty(&fcfs_q)) // 만약 fcfs은 empty이고 rr은 empty가 아니라면
			{
				Process * run_process = Dequeue(&rr_q);
				Enqueue(&running_q, run_process);

			}


			if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
			{
				Process * run_process = QPeek(&running_q);
				printf("TIME[%d]~TIME[%d]  cpu: P[%d], rr status: %d,   fcfs status: %d   ", time, time + 1, run_process->pid, !QIsEmpty(&rr_q), !QIsEmpty(&fcfs_q));
				if (run_process->priority < 6)
				{
					printf(" round robin process\n");
				}
				else
					printf(" fcfs process\n");
				run_process->progress++;
				run_process->time_quantum++;


				if (run_process->progress == run_process->cpu_bt)
				{
					Process * finish_p = Dequeue(&running_q);
					finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
					check++;
				}
				else if (run_process->progress == run_process->io_start)
				{
					waiting_q[run_process->pid] = run_process->io_bt;
					run_process->time_quantum = 0;
					Dequeue(&running_q);

				}
				else if (run_process->priority < 6)
				{
					if (run_process->time_quantum == current_time_quantum)
					{
						run_process->time_quantum = 0;
						Enqueue(&rr_q, (Dequeue(&running_q)));
					}
				}

			}
			else
			{
				printf("TIME[%d]~TIME[%d]  cpu : IDLE\n", time, time + 1);
			}
		}





	}

	printf("------------------------------------------------------------------------------------------\n");

	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	multilevel_absolute.avg_turnaround = avg_turnaround;
	multilevel_absolute.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");


}


void MULTILEVEL_FEEDBACK_QUEUE_ABSOLUTE() {

	printf("implementation of MULTILEVEL_FEEDBACK_QUEUE_ABSOLUTE\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check = 0;

	int current_time_quantum;
	int promotion_time;
	while (1) {
		printf("how would you like the promotion time to be? : ");
		scanf("%d", &promotion_time);
		printf("how would you like the time quantum to be? : ");
		scanf("%d", &current_time_quantum);
		if (current_time_quantum > 5 || current_time_quantum < 1) printf("[ERROR] Time Quantum must be 1 to 4. Please enter time quantum again: ");
		else break;


	}
	Queue rr_q;
	Queue fcfs_q;
	// ready_q, runnin_q, waiting_q initialization
	QueueInit(&rr_q);
	QueueInit(&fcfs_q);
	QueueInit(&running_q);

	for (int i = 0; i < num_process; i++)
	{
		waiting_q[i] = -1;
	}


	for (time = 0; check != num_process; time++)
	{
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				if (process[i]->priority < 6) // priority 높은 애들은 rr
					Enqueue(&rr_q, process[i]);
				else
					Enqueue(&fcfs_q, process[i]);
			}
			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				waiting_q[i]--;
				if (process[i]->priority < 6) // priority 높은 애들은 rr
					Enqueue(&rr_q, process[i]);
				else
					Enqueue(&fcfs_q, process[i]);
			}

		}
		if (!QIsEmpty(&rr_q)) //rr을 우선으로 실행시켜주기
		{
			if (QIsEmpty(&running_q) && !QIsEmpty(&rr_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
			{
				Process * run_process = Dequeue(&rr_q);
				Enqueue(&running_q, run_process);

			}
			else if (QIsEmpty(&running_q) && !QIsEmpty(&fcfs_q)) // 만약 rr은 empty이고 fcfs은 empty가 아니라면--> 실제로 실행되는 코드는 아님
			{
				Process * run_process = Dequeue(&fcfs_q);
				Enqueue(&running_q, run_process);
				run_process->aging_time = 0; // aging 시간을 초기화

			}


			if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
			{
				Process * run_process = QPeek(&running_q);
				printf("TIME[%d]~TIME[%d]  cpu: P[%d], rr status: %d,   fcfs status: %d   ", time, time + 1, run_process->pid, !QIsEmpty(&rr_q), !QIsEmpty(&fcfs_q));
				run_process->progress++;
				run_process->time_quantum++;
				if (run_process->priority < 6)
				{
					printf(" round robin process\n");
				}
				else
					printf(" fcfs process\n");

				if (run_process->progress == run_process->cpu_bt)
				{
					Process * finish_p = Dequeue(&running_q);
					finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
					check++;
				}
				else if (run_process->progress == run_process->io_start)
				{
					waiting_q[run_process->pid] = run_process->io_bt;
					run_process->time_quantum = 0;
					Dequeue(&running_q);

				}
				else if (run_process->priority < 6)
				{
					if (run_process->time_quantum == current_time_quantum)
					{
						run_process->time_quantum = 0;
						Enqueue(&rr_q, (Dequeue(&running_q)));
					}
				}

			}
			else
			{
				printf("TIME[%d]~TIME[%d]  cpu : IDLE\n", time, time + 1);
			}



			if (!QIsEmpty(&fcfs_q)) // aging 관련 처리
			{

				Process * temp_age[MAX_PROCESS];
				int promote[MAX_PROCESS] = { 0, };
				int age_num = 0;
				while (!QIsEmpty(&fcfs_q))
				{
					temp_age[age_num] = Dequeue(&fcfs_q);
					age_num++;
				}

				for (int a = 0; a < age_num; a++)//현재 시간에 대해 ready_q에 있는 애들의 aging_time이 max time에 도달하면 priority를 수정
				{
					temp_age[a]->aging_time++;
				}

				for (int a = 0; a < age_num; a++)
				{
					if (temp_age[a]->aging_time == promotion_time) // aging을 하기 위해 ready_Q에 잇어야 하는 시간
					{
						temp_age[a]->priority = 5; // 승격시킨 후 승격의 자격을 유지하기 위해 priority 수정
						Enqueue(&rr_q, temp_age[a]); // aging을 완료했기에 상위 큐인 rr_Q로 승격
						printf("promotion of P[%d] has occured\n", temp_age[a]->pid);
						temp_age[a]->aging_time = 0;
						promote[a] = 1;//승격시켜주기
									   //printf("내 나이가 어때서~ P[%d], PRIORITY %d -> %d \n",temp_age[a]->pid, past, temp_age[a]->priority );
					}
					else
					{
						//printf("현재 내가 ready_q에 있는 시간은 P[%d], AGING TIME = %d\n", temp_age[a]->pid, temp_age[a]->aging_time);
					}
				}
				//printf("\n");
				int age_cnt = 0;
				for (int a = 0; a < age_num; a++)
				{
					if (promote[a] == 1)
					{

						age_cnt++; // 승격된 process의 갯수 세기
						for (int t = a; t < age_num; t++)
						{
							temp_age[t] = temp_age[t + 1];
						}
						for (int l = a; l < age_num; l++)
						{
							promote[l] = promote[l + 1];
						}
						a--;
					}
				}
				age_num = age_num - age_cnt;

				for (int m = 0; m < age_num; m++)
				{
					Enqueue(&fcfs_q, temp_age[m]);
				}

			}

		}
		else //IF(!QIsEmpty(fcfs_q) ==> RR_Q에 아무것도 없으면 FCFS에 있는 PROCESS를 실행하고자 함
		{
			if (QIsEmpty(&running_q) && !QIsEmpty(&fcfs_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
			{
				Process * run_process = Dequeue(&fcfs_q);
				Enqueue(&running_q, run_process);
				run_process->aging_time = 0;
			}
			else if (QIsEmpty(&running_q) && !QIsEmpty(&rr_q)) // 만약 fcfs은 empty이고 rr은 empty가 아니라면
			{
				Process * run_process = Dequeue(&rr_q);
				Enqueue(&running_q, run_process);

			}


			if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
			{
				Process * run_process = QPeek(&running_q);
				printf("TIME[%d]~TIME[%d]  cpu: P[%d], rr status: %d,   fcfs status: %d   ", time, time + 1, run_process->pid, !QIsEmpty(&rr_q), !QIsEmpty(&fcfs_q));
				if (run_process->priority < 6)
				{
					printf(" round robin process\n");
				}
				else
					printf(" fcfs process\n");
				run_process->progress++;
				run_process->time_quantum++;


				if (run_process->progress == run_process->cpu_bt)
				{
					Process * finish_p = Dequeue(&running_q);
					finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
					check++;
				}
				else if (run_process->progress == run_process->io_start)
				{
					waiting_q[run_process->pid] = run_process->io_bt;
					run_process->time_quantum = 0;
					Dequeue(&running_q);

				}
				else if (run_process->priority < 6)
				{
					if (run_process->time_quantum == current_time_quantum)
					{
						run_process->time_quantum = 0;
						Enqueue(&rr_q, (Dequeue(&running_q)));
					}
				}

			}
			else
			{
				printf("TIME[%d]~TIME[%d]  cpu : IDLE\n", time, time + 1);
			}


			if (!QIsEmpty(&fcfs_q)) // aging 관련 처리
			{

				Process * temp_age[MAX_PROCESS];
				int promote[MAX_PROCESS] = { 0, };
				int age_num = 0;
				while (!QIsEmpty(&fcfs_q))
				{
					temp_age[age_num] = Dequeue(&fcfs_q);
					age_num++;
				}

				for (int a = 0; a < age_num; a++)//현재 시간에 대해 ready_q에 있는 애들의 aging_time이 max time에 도달하면 priority를 수정
				{
					temp_age[a]->aging_time++;
				}

				for (int a = 0; a < age_num; a++)
				{
					if (temp_age[a]->aging_time == promotion_time) // aging을 하기 위해 ready_Q에 잇어야 하는 시간
					{
						Enqueue(&rr_q, temp_age[a]);// aging을 완료했기에 상위 큐인 rr_Q로 승격
						printf("promotion of P[%d] has occured\n", temp_age[a]->pid);
						temp_age[a]->aging_time = 0;
						promote[a] = 1;//승격시켜주기
									   //printf("내 나이가 어때서~ P[%d], PRIORITY %d -> %d \n",temp_age[a]->pid, past, temp_age[a]->priority );
					}
					else
					{
						//printf("현재 내가 ready_q에 있는 시간은 P[%d], AGING TIME = %d\n", temp_age[a]->pid, temp_age[a]->aging_time);
					}
				}
				//printf("\n");
				int age_cnt = 0;
				for (int a = 0; a < age_num; a++)
				{
					if (promote[a] == 1)
					{

						age_cnt++;
						for (int t = a; t < age_num; t++)
						{
							temp_age[t] = temp_age[t + 1];
						}
						for (int l = a; l < age_num; l++)
						{
							promote[l] = promote[l + 1];
						}
						a--;
					}
				}
				age_num = age_num - age_cnt;

				for (int m = 0; m < age_num; m++)
				{
					Enqueue(&fcfs_q, temp_age[m]);
				}

			}

		}





	}

	printf("------------------------------------------------------------------------------------------\n");

	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	multilevel_feedback_absolute.avg_turnaround = avg_turnaround;
	multilevel_feedback_absolute.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");


}

void HRRN() //ready_q에 있을 때마다 waiting time 올려주기
{
	printf("implementation of highest response ration next\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check = 0;



	for (time = 0; check != num_process; time++)
	{
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				if (QIsEmpty(&ready_q))
					Enqueue(&ready_q, process[i]);
				else {
					Enqueue(&ready_q, process[i]);

					//ready_q에 process를 enqueue 할때 마다 (남은 cpu 시간) + (waiting time) / (남은 cpu 시간) 을 기준으로 큰애가 맨앞으로 오게 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (((float)((temp_p[n]->remain_cpu_time) + (temp_p[n]->waiting_time)) / (float)(temp_p[n]->remain_cpu_time)) < ((float)((temp_p[n + 1]->remain_cpu_time) + (temp_p[n + 1]->waiting_time)) / (float)(temp_p[n + 1]->remain_cpu_time))) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}

					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}
				}
			}
			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				waiting_q[i]--;
				if (QIsEmpty(&ready_q))
				{
					Enqueue(&ready_q, process[i]);
					process[i]->new_arrival_time = time;
				}
				else {
					Enqueue(&ready_q, process[i]);
					process[i]->new_arrival_time = time;
					//ready_q에 process를 enqueue 할때 마다  (남은 cpu 시간) + (waiting time) / (남은 cpu 시간)을 기준으로 큰 값이 맨 앞으로 오게 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (((float)((temp_p[n]->remain_cpu_time) + (temp_p[n]->waiting_time)) / (float)(temp_p[n]->remain_cpu_time)) < ((float)((temp_p[n + 1]->remain_cpu_time) + (temp_p[n + 1]->waiting_time)) / (float)(temp_p[n + 1]->remain_cpu_time))) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}
					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}

				}
			}

		}
		if (QIsEmpty(&running_q) && !QIsEmpty(&ready_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
		{
			Process * run_process = Dequeue(&ready_q);
			Enqueue(&running_q, run_process);
		}

		if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
		{
			Process * run_process = QPeek(&running_q);
			printf("TIME[%d]~TIME[%d]  cpu : P[%d]\n", time, time + 1, run_process->pid);
			run_process->progress++;
			run_process->remain_cpu_time--;


			if (run_process->progress == run_process->cpu_bt)
			{
				Process * finish_p = Dequeue(&running_q);
				finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
				check++;
			}
			else if (run_process->progress == run_process->io_start)
			{
				waiting_q[run_process->pid] = run_process->io_bt;
				Dequeue(&running_q);

			}
		}
		else
		{
			printf("TIME[%d]~TIME[%d]  cpu : IDLE\n", time, time + 1);
		}

		if (!QIsEmpty(&ready_q))//waiting 시간 늘려주기!
		{
			Process * temp_p[MAX_PROCESS];
			int k = 0;
			while (!QIsEmpty(&ready_q))
			{
				temp_p[k] = Dequeue(&ready_q);
				temp_p[k]->waiting_time++;
				k++;
			}

			Process * temp;
			for (int m = k - 1; m > 0; m--) {
				for (int n = 0; n < m; n++) {
					if (((float)((temp_p[n]->remain_cpu_time) + (temp_p[n]->waiting_time)) / (float)(temp_p[n]->remain_cpu_time)) < ((float)((temp_p[n + 1]->remain_cpu_time) + (temp_p[n + 1]->waiting_time)) / (float)(temp_p[n + 1]->remain_cpu_time))) {
						temp = temp_p[n];
						temp_p[n] = temp_p[n + 1];
						temp_p[n + 1] = temp;
					}
				}
			}
			for (int m = 0; m < k; m++)
			{
				Enqueue(&ready_q, temp_p[m]);
			}
		}

	}

	printf("------------------------------------------------------------------------------------------\n");

	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	hrrn.avg_turnaround = avg_turnaround;
	hrrn.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");


}

void LRTF_NP()
{
	printf("implementation of non preemptive longest remaining time first\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check = 0;


	for (time = 0; check != num_process; time++)
	{
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				if (QIsEmpty(&ready_q))
					Enqueue(&ready_q, process[i]);
				else {
					Enqueue(&ready_q, process[i]);

					//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->remain_cpu_time < temp_p[n + 1]->remain_cpu_time) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}

					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}



				}
			}
			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				waiting_q[i]--;
				if (QIsEmpty(&ready_q))
				{
					Enqueue(&ready_q, process[i]);
					process[i]->new_arrival_time = time;
				}
				else {
					Enqueue(&ready_q, process[i]);
					process[i]->new_arrival_time = time;
					//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->remain_cpu_time < temp_p[n + 1]->remain_cpu_time) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}
					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}

				}
			}

		}
		if (QIsEmpty(&running_q) && !QIsEmpty(&ready_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
		{
			Process * run_process = Dequeue(&ready_q);
			Enqueue(&running_q, run_process);


		}

		if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
		{
			Process * run_process = QPeek(&running_q);
			printf("TIME[%d]~TIME[%d]  cpu : P[%d]\n", time, time + 1, run_process->pid);



			run_process->progress++;
			run_process->remain_cpu_time--;


			if (run_process->progress == run_process->cpu_bt)
			{
				Process * finish_p = Dequeue(&running_q);
				finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
				check++;
			}
			else if (run_process->progress == run_process->io_start)
			{
				waiting_q[run_process->pid] = run_process->io_bt;
				Dequeue(&running_q);

			}
		}
		else
		{
			printf("TIME[%d]~TIME[%d]  cpu : IDLE\n", time, time + 1);
		}

	}

	printf("------------------------------------------------------------------------------------------\n");

	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	lrtf_np.avg_turnaround = avg_turnaround;
	lrtf_np.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");


}


void LRTF_P() {
	printf("implementation of preemptive longest remaining time first\n");
	printf("------------------------------------------------------------------------------------------\n");
	int time;
	int check = 0;


	for (time = 0; check != num_process; time++)
	{
		for (int i = 0; i < num_process; i++)
		{
			if (time == process[i]->arrival_time) //arrival time 도달 시 ready_q에 해당 process enqueue
			{
				if (QIsEmpty(&ready_q)) {
					Enqueue(&ready_q, process[i]);

					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->remain_cpu_time > run_process->remain_cpu_time) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);

						}
					}

				}



				else {
					Enqueue(&ready_q, process[i]);

					//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->remain_cpu_time < temp_p[n + 1]->remain_cpu_time) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}

					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}


					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->remain_cpu_time > run_process->remain_cpu_time) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);


							//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
							Process * temp_p[MAX_PROCESS];
							int k = 0;
							while (!QIsEmpty(&ready_q))
							{
								temp_p[k] = Dequeue(&ready_q);
								k++;
							}

							Process * temp;
							for (int m = k - 1; m > 0; m--) {
								for (int n = 0; n < m; n++) {
									if (temp_p[n]->remain_cpu_time < temp_p[n + 1]->remain_cpu_time) {
										temp = temp_p[n];
										temp_p[n] = temp_p[n + 1];
										temp_p[n + 1] = temp;
									}
								}
							}

							for (int m = 0; m < k; m++)
							{
								Enqueue(&ready_q, temp_p[m]);
							}

						}

					}
				}
			}

			if (waiting_q[i] > 0)  // 만약 해당 process가 waiting_q에 있다면 시간 줄이기
			{
				waiting_q[i]--;
			}
			else if (waiting_q[i] == 0)
			{
				waiting_q[i]--;
				if (QIsEmpty(&ready_q)) {
					Enqueue(&ready_q, process[i]);


					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->remain_cpu_time > run_process->remain_cpu_time) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);


						}

					}



				}
				else {
					Enqueue(&ready_q, process[i]);


					//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
					Process * temp_p[MAX_PROCESS];
					int k = 0;
					while (!QIsEmpty(&ready_q))
					{
						temp_p[k] = Dequeue(&ready_q);
						k++;
					}

					Process * temp;
					for (int m = k - 1; m > 0; m--) {
						for (int n = 0; n < m; n++) {
							if (temp_p[n]->remain_cpu_time < temp_p[n + 1]->remain_cpu_time) {
								temp = temp_p[n];
								temp_p[n] = temp_p[n + 1];
								temp_p[n + 1] = temp;
							}
						}
					}

					for (int m = 0; m < k; m++)
					{
						Enqueue(&ready_q, temp_p[m]);
					}


					if (!QIsEmpty(&running_q)) { // 새로 들어온 process의 remain cpu time이 기존의 running process의 remain cpu time보다 작을 경우 preemption이 이루어지도록 한다.
						Process * run_process = QPeek(&running_q);
						Process * ready_process = QPeek(&ready_q);
						if (ready_process->remain_cpu_time > run_process->remain_cpu_time) {
							Process * preempt_ready_process = Dequeue(&running_q);
							Process * preempt_run_process = Dequeue(&ready_q);

							Enqueue(&running_q, preempt_run_process);
							Enqueue(&ready_q, preempt_ready_process);

							//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
							Process * temp_p_2[MAX_PROCESS];
							int k = 0;
							while (!QIsEmpty(&ready_q))
							{
								temp_p_2[k] = Dequeue(&ready_q);
								k++;
							}

							Process * temp;
							for (int m = k - 1; m > 0; m--) {
								for (int n = 0; n < m; n++) {
									if (temp_p_2[n]->remain_cpu_time < temp_p_2[n + 1]->remain_cpu_time) {
										temp = temp_p_2[n];
										temp_p_2[n] = temp_p_2[n + 1];
										temp_p_2[n + 1] = temp;
									}
								}
							}

							for (int m = 0; m < k; m++)
							{
								Enqueue(&ready_q, temp_p_2[m]);
							}


						}

					}
				}
			}

		}
		if (QIsEmpty(&running_q) && !QIsEmpty(&ready_q)) // 만약 cpu가 idle하고 ready_q에 process가 있으면 process run 시켜주기
		{
			Process * run_process = Dequeue(&ready_q);
			Enqueue(&running_q, run_process);


		}

		if (!QIsEmpty(&running_q)) // 만약 cpu가 idle하지 않다면
		{
			Process * run_process = QPeek(&running_q);
			printf("TIME[%d]~TIME[%d]  cpu : P[%d]\n", time, time + 1, run_process->pid);
			run_process->progress++;
			run_process->remain_cpu_time--;


			if (run_process->progress == run_process->cpu_bt)
			{
				Process * finish_p = Dequeue(&running_q);
				finish_p->turnaround_time = (time + 1) - finish_p->arrival_time;
				check++;
			}
			else if (run_process->progress == run_process->io_start)
			{
				waiting_q[run_process->pid] = run_process->io_bt;
				Dequeue(&running_q);

			}
			else if(!QIsEmpty(&ready_q)) {//running 동안 preemption 가능하도록( 실행되고 잇는애가 ready_q의 애들보다 작아진다면 preemption
				
					Process * ready_process = QPeek(&ready_q);
					if (ready_process->remain_cpu_time > run_process->remain_cpu_time) {



						Process * preempt_ready_process = Dequeue(&running_q);
						Process * preempt_run_process = Dequeue(&ready_q);

						Enqueue(&running_q, preempt_run_process);
						Enqueue(&ready_q, preempt_ready_process);

						//ready_q에 process를 enqueue 할때 마다 남은 cpu 시간을 기준으로 sorting하기 
						Process * temp_p[MAX_PROCESS];
						int k = 0;
						while (!QIsEmpty(&ready_q))
						{
							temp_p[k] = Dequeue(&ready_q);
							k++;
						}

						Process * temp;
						for (int m = k - 1; m > 0; m--) {
							for (int n = 0; n < m; n++) {
								if (temp_p[n]->remain_cpu_time < temp_p[n + 1]->remain_cpu_time) {
									temp = temp_p[n];
									temp_p[n] = temp_p[n + 1];
									temp_p[n + 1] = temp;
								}
							}
						}

						for (int m = 0; m < k; m++)
						{
							Enqueue(&ready_q, temp_p[m]);
						}

					}
			}
		}
		else
		{
			printf("TIME[%d]~TIME[%d]  cpu: IDLE\n", time, time + 1);
		}
	}
	printf("------------------------------------------------------------------------------------------\n");

	int turnaround_sum = 0;
	int waiting_sum = 0;

	for (int b = 0; b < num_process; b++)
	{
		turnaround_sum += process[b]->turnaround_time;
		waiting_sum += ((process[b]->turnaround_time) - (process[b]->cpu_bt) - (process[b]->io_bt));
	}
	float avg_turnaround = (float)turnaround_sum / (float)num_process;
	float avg_waiting = (float)waiting_sum / (float)num_process;

	printf("\n* Average Waiting Time = %.4f", avg_waiting);
	printf("\n* Average Turnaround Time = %.4f\n", avg_turnaround);
	lrtf_p.avg_turnaround = avg_turnaround;
	lrtf_p.avg_waiting = avg_waiting;
	printf("------------------------------------------------------------------------------------------\n");
}


int Print_Menu() {//만들게 aging priority, sjf_io, ljf
	int selection = 0;

	printf("|--------------SCHEDULER MENU--------------|\n");
	printf("|..........................................|\n");
	printf("|               1.  FCFS                   |\n");
	printf("|        2.  nonpreemptive SJF             |\n");
	printf("|         3.  preemptive SJF               |\n");
	printf("|       4.  nonpreemptive Priority         |\n");
	printf("|        5.  preemptive Priority           |\n");
	printf("|                6.  RR                    |\n");
	printf("|    7.  nonpreemptive Aging Priority      |\n");
	printf("|       8. preemptive Aging Priority       |\n");
	printf("|    9.  Multilevel Queue(Time Slice)      |\n");
	printf("|       10.  Multilevel Queue(Absolute)    |\n");
	printf("|    11. Multilevel Feedback Scheduling    |\n");
	printf("|    12.  Highest Response Ratio Next      |\n");
	printf("|        13.  nonpreemptive LRTF           |\n");
	printf("|         14.  preemptive LRTF             |\n");
	printf("|                OTHERS:  EXIT             |\n");
	printf("|--------------SCHEDULER MENU--------------|\n");

	printf("\nSCHEDULING 방법을 고르세요 \n(EXIT을 위해선 1-6이 아닌 수를 입력하세요) : ");
	scanf("%d", &selection);
	printf("\n");
	return selection;
}

void Evaluation() {
	printf("[Evaluation of each scheduling algorithms]\n");
	printf("-----------------------------------------------------------------------------\n");
	printf("1. FCFS Scheduling\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n", fcfs.avg_waiting, fcfs.avg_turnaround);
	printf("2. SJF_NP Scheduling\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n", sjf_np.avg_waiting, sjf_np.avg_turnaround);
	printf("3. SJF_P Scheduling\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n", sjf_p.avg_waiting, sjf_p.avg_turnaround);
	printf("4. PRIORITY_NP Scheduling\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n", priority_np.avg_waiting, priority_np.avg_turnaround);
	printf("5. PRIORITY_P Scheduling\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n", priority_p.avg_waiting, priority_p.avg_turnaround);
	printf("6. ROUND ROBIN Scheduling\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n", rr.avg_waiting, rr.avg_turnaround);
	printf("7. AGING_PRIORITY_NP Scheduling\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n", aging_priority_np.avg_waiting, aging_priority_np.avg_turnaround);
	printf("8. AGING_PRIORITY_P Scheduling\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n", aging_priority_p.avg_waiting, aging_priority_p.avg_turnaround);
	printf("9. MULTILEVEL Scheduling(Time Slice)\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n", multilevel.avg_waiting, multilevel.avg_turnaround);
	printf("10. MULTILEVEL Scheduling(Absolute)\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n", multilevel_absolute.avg_waiting, multilevel_absolute.avg_turnaround);
	printf("11. MULTILEVEL FEEDBACK Scheduling(Absolute)\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n", multilevel_feedback_absolute.avg_waiting, multilevel_feedback_absolute.avg_turnaround);
	printf("12. HIGHEST RESPONSE RATIO NEXT Scheduling\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n",hrrn.avg_waiting, hrrn.avg_turnaround);
	printf("13. LONGEST REMAINING TIME FIRST_NP Scheduling\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n", lrtf_np.avg_waiting, lrtf_np.avg_turnaround);
	printf("14. LONGEST REMAINING TIME FIRST_P Scheduling\n");
	printf("Average Waiting Time = %.4f, Average Turnaround Time = %.4f\n", lrtf_p.avg_waiting, lrtf_p.avg_turnaround);
	
}

void Main_Menu() {
	int selection_menu = 0;
	while (1) {
		printf("|--------------MAIN MENU--------------|\n");
		printf("|.....................................|\n");
		printf("|         0.  CREATE PROCESS          |\n");
		printf("|   1. SELECT SCHEDULING_ALGORITHM    |\n");
		printf("| 2. EXECUTE ALL SCHEDULING ALGORITHMS|\n");
		printf("|            3. EVALUATION            |\n");
		printf("|           4. EXIT PROGRAM           |\n");
		printf("|--------------MAIN MENU--------------|\n");

		printf("\n 실행할 ACTION을 고르세요: ");
		scanf("%d", &selection_menu);
		printf("\n");

		if (selection_menu == 0)
		{
			Create_Process();
		}
		else if (selection_menu == 1)
		{
			int algorithm = Print_Menu();
			Config();
			Initialize();
			switch (algorithm) {

			case 1: FCFS();
				break;
			case 2: SJF_NP();
				break;
			case 3: SJF_P();
				break;
			case 4: PRIORITY_NP();
				break;
			case 5: PRIORITY_P();
				break;
			case 6 : RR();
				break;
			case 7: AGING_PRIORITY_NP();
				break;
			case 8: AGING_PRIORITY_P();
				break;
			case 9: MULTILEVEL_QUEUE();
				break;
			case 10: MULTILEVEL_QUEUE_ABSOLUTE();
				break;
			case 11: MULTILEVEL_FEEDBACK_QUEUE_ABSOLUTE();
				break;
			case 12: HRRN();
				break;
			case 13: LRTF_NP();
				break;
			case 14: LRTF_P();
				break;
			default: printf("잘못된 메뉴를 선택하셨습니다.\n");
				exit(-1);

			}
		}
		else if (selection_menu == 2) {
			Config();
			Initialize();
			FCFS();
			Config();
			Initialize();
			SJF_NP();
			Config();
			Initialize();
			SJF_P();
			Config();
			Initialize();
			PRIORITY_NP();
			Config();
			Initialize();
			PRIORITY_P();
			Config();
			Initialize();
			RR();
			Config();
			Initialize();
			AGING_PRIORITY_NP();
			Config();
			Initialize();
			AGING_PRIORITY_P();
			Config();
			Initialize();
			MULTILEVEL_QUEUE();
			Config();
			Initialize();
			MULTILEVEL_QUEUE_ABSOLUTE();
			Config();
			Initialize();
			MULTILEVEL_FEEDBACK_QUEUE_ABSOLUTE();
			Config();
			Initialize();
			HRRN(); 
			Config();
			Initialize();
			Config();
			LRTF_NP();
			Config();
			Initialize();
			LRTF_P();
		}
		else if (selection_menu == 3)
		{
			Evaluation();
		}
		else if (selection_menu == 4)
		{
			printf(" PROGRAM WILL BE EXITED!\n");
			exit(-1);
		}
		else
		{
			printf(" SELECTION ERROR HAS OCCURED \n");
			exit(-1);
		}
	}
}


int main() {
	
	Main_Menu();

	return 0;
}

