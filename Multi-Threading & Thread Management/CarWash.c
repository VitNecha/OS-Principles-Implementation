
#define BILLION 1000000000L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <signal.h>

static struct timespec clock_start;

typedef union _semun
{
  int val;
  struct semid_ds *buf;
  unsigned short *array;
}semun;

int stop_flag = 0;

//Signal handler
void signal_handler(int sig) {
	key_t queue_key = ftok("CarWash.c",'a');
	key_t system_data_key = ftok("CarWash.c",'b');
	int queue_id = shmget(queue_key, 1024, IPC_CREAT|0666);
	int system_data_id = shmget(system_data_key, 256, IPC_CREAT|0666);
	int * queue = (int *) shmat(queue_id,0,0);
	int * system_data = (int *) shmat(system_data_id,0,0);
	kill(queue[system_data[1]],SIGCONT);
}

//User clicked ctrl + z
void stop_signal_handler(){
  	stop_flag = 1;
  	key_t queue_key = ftok("CarWash.c",'a');
	key_t system_data_key = ftok("CarWash.c",'b');
	int queue_id = shmget(queue_key, 1024, IPC_CREAT|0666);
	int system_data_id = shmget(system_data_key, 256, IPC_CREAT|0666);
	int * queue = (int *) shmat(queue_id,0,0);
	int * system_data = (int *) shmat(system_data_id,0,0);
	printf("\n");
  	for(int i = system_data[1]; i < system_data[0]; i++){
    	kill(queue[i], SIGKILL);
  	}
}

int DOWN(int semid);
int UP(int semid);
int initsem(key_t semkey);
double timer();
float nextTime(float rateParameter);
int washing(key_t shm_keys[6], int shm_ids[6], float carWashTimeRate);
float generateNumberAround(float lambda);

// Main
int main (int argc, int **argv) {
	srand(time(NULL));
  	float carArrivalRate = 1/generateNumberAround(1.5); // 1 / lambda
  	float carWashTimeRate = 1/generateNumberAround(3);
  	float runtime = generateNumberAround(30);
  	printf(" System Runtime: %f\n", runtime);
	int status;
	pid_t wpid;
	key_t shm_keys[6]; // Array of shared memory keys
	int shm_ids[6]; // Array of shared memory id's
	int * system_data_shm;
	double * queue_time_sum;
	signal(SIGTSTP, stop_signal_handler);

	//Clean-up
	semctl(shm_ids[0], IPC_RMID, 0);
	semctl(shm_ids[1], IPC_RMID, 0);
	semctl(shm_ids[2], IPC_RMID, 0);
	semctl(shm_ids[3], IPC_RMID, 0);
	semctl(shm_ids[4], IPC_RMID, 0);
	semctl(shm_ids[5], IPC_RMID, 0);

	// Creating keys for shared memory segments
	shm_keys[0] = ftok("CarWash.c",'a'); // Queue key
	shm_keys[1] = ftok("CarWash.c",'b'); // System data key
	shm_keys[2] = ftok("CarWash.c",'c'); // Queue entering print line key
	shm_keys[3] = ftok("CarWash.c",'d'); // Washing station entering print line key
	shm_keys[4] = ftok("CarWash.c",'e'); // Washing station exiting print line key
	shm_keys[5] = ftok("CarWash.c",'f'); // Summary of all queue waiting times times key

	// Creating shared memory segments
	if ((shm_ids[0] = shmget(shm_keys[0], 1024, IPC_CREAT|0666)) == -1) {
		printf("Failure to create share memory segment for queue\n");
		exit(1);
	}
	if ((shm_ids[1] = shmget(shm_keys[1], 256, IPC_CREAT|0666)) == -1) {
		printf("Failure to create share memory segment for system data\n");
		exit(1);
	}
	if ((shm_ids[2] = shmget(shm_keys[2], 128, IPC_CREAT|0666)) == -1) {
		printf("Failure to create share memory segment for queue entering print line\n");
		exit(1);
	}
	if ((shm_ids[3] = shmget(shm_keys[3], 128, IPC_CREAT|0666)) == -1) {
		printf("Failure to create share memory segment for washing station entering print\n");
		exit(1);
	}
	if ((shm_ids[4] = shmget(shm_keys[4], 128, IPC_CREAT|0666)) == -1) {
		printf("Failure to create share memory segment for washing station exiting print line\n");
		exit(1);
	}
	if ((shm_ids[5] = shmget(shm_keys[5], 32, IPC_CREAT|0666)) == -1) {
		printf("Failure to create share memory segment for queue time summary\n");
		exit(1);
	}

	system_data_shm = shmat(shm_ids[1], 0, 0); // System data share memory attachment
	if (!system_data_shm) {
        printf("Failure to initialize system_data_id\n");
        exit(1);
    }

	queue_time_sum = shmat(shm_ids[5], 0, 0); // Queue time summary share memory attachment
	if (!queue_time_sum) {
        printf("Failure to initialize system_data_id\n");
        exit(1);
    }

	// Queue counters init
	system_data_shm[0] = 0; // Counter of total number of cars in the queue
	system_data_shm[1] = 0; // Index of the first car out of queue (FIFO)
	system_data_shm[2] = 0; // Counter of total number of cars in washing stations
	system_data_shm[3] = 0; // Counter of total number of cars washed

	// Washing time summary init
	queue_time_sum[0] = 0;

	// User input
	printf(" Please enter the number of the washing station: ");
	scanf("%d", &system_data_shm[4]);
	
	clock_gettime(CLOCK_REALTIME, &clock_start);

	// Arrival of cars
	while(timer() < runtime) {
    	float nextCar = nextTime(carArrivalRate);
		sleep(nextCar);
    	if(stop_flag == 1){
      		printf("\n\033[1;31m THE SYSTEM HAS BEEN STOPPED! QUEUE CLEARED.\033[0m\n");
      		break;
    	}
		if (fork() == 0) {
			washing(shm_keys, shm_ids, carWashTimeRate);
			exit(1);
		}
	}

	while ((wpid = wait(&status)) > 0); // Waiting for all child processes

	printf("\n\033[1;33m IN SUMMARY \033[0m \n\033[1;33m -------------\033[0m\n");
	printf("\t\033[0;33mTotal cars washed: %d\033[0m\n",system_data_shm[3]);
	printf("\t\033[0;33mAverage queue waiting time: %lf seconds\033[0m\n\n\n", (double) (queue_time_sum[0] / system_data_shm[3]));

	// Clean-up
	semctl(shm_ids[0], IPC_RMID, 0);
	semctl(shm_ids[1], IPC_RMID, 0);
	semctl(shm_ids[2], IPC_RMID, 0);
	semctl(shm_ids[3], IPC_RMID, 0);
	semctl(shm_ids[4], IPC_RMID, 0);

	// End
	return 0;
}

// Washing function
int washing(key_t shm_keys[6], int shm_ids[6], float carWashTimeRate){
	int queue_semid, wash_semid, exit_semid, count_semid;
	char * queue_line_shm, * enter_line_shm, * exit_line_shm;
	int * queue_shm, * system_data_shm;
	double t1, t2;
	double * queue_time_sum;
	
	struct timespec current_time;
	signal(SIGUSR1,signal_handler);

	// Queue semaphore init
	if((queue_semid = initsem(shm_keys[0])) < 0) {
		printf("Failure to initialize semaphore for queue\n");
    	exit(1);
  	}

	// Entering washing station semaphore init
	if((wash_semid = initsem(shm_keys[3])) < 0) {
		printf("Failure to initialize semaphore for enter\n");
    	exit(1);
  	}

	// Exiting washing station semaphore init
	if((exit_semid = initsem(shm_keys[4])) < 0) {
		printf("Failure to initialize semaphore for exit\n");
    	exit(1);
  	}

	// Washing stations state semaphore init (will use one of the keys that not in use by other semaphores)
	if((count_semid = initsem(shm_keys[2])) < 0) {
		printf("Failure to initialize semaphore for exit\n");
    	exit(1);
  	}

	queue_line_shm = shmat(shm_ids[2], 0, 0); // Queue entering line share memory pointer attachment
	if (!queue_line_shm) {
        printf("Failure to initialize queue_line_shm\n");
        exit(1);
    }

	enter_line_shm = shmat(shm_ids[3], 0, 0); // Queue entering line share memory pointer attachment
	if (!enter_line_shm) {
        printf("Failure to initialize enter_line_shm\n");
        exit(1);
    }

	exit_line_shm = shmat(shm_ids[4], 0, 0); // Queue entering line share memory pointer attachment
	if (!queue_line_shm) {
        printf("Failure to initialize exit_line_shm\n");
        exit(1);
    }

	queue_shm = shmat(shm_ids[0], 0, 0); // Queue share memory pointer attachment
	if (!queue_shm) {
        printf("Failure to initialize queue_shm\n");
        exit(1);
    }

	system_data_shm = shmat(shm_ids[1], 0, 0); // System data share memory pointer attachment
	if (!system_data_shm) {
        printf("Failure to initialize system_data_shm\n");
        exit(1);
    }

	queue_time_sum = shmat(shm_ids[5], 0, 0); // Queue waiting time summary share memory attachment
	if (!queue_time_sum) {
        printf("Failure to initialize queue_time_sum\n");
        exit(1);
    }

	// Car entering the queue
	DOWN(queue_semid);
	queue_shm[system_data_shm[0]] = (int) getpid();
	system_data_shm[0]++;
	snprintf(queue_line_shm, 128, "\033[1;34m Car %d entered the queue at %lf seconds \033[0m \n", (int) getpid(), timer());
	printf("%s",queue_line_shm);
	UP(queue_semid);
	
	clock_gettime(CLOCK_REALTIME, &current_time);
	t1 = (current_time.tv_sec - clock_start.tv_sec) + (double) (current_time.tv_nsec - clock_start.tv_nsec) / (double) BILLION;
	
	// Washing stations state check
	if (system_data_shm[2] >= system_data_shm[4]) {
		kill(getpid(),SIGSTOP); //send signal to suspend the process.
	}
	
	clock_gettime(CLOCK_REALTIME, &current_time);
	t2 = (current_time.tv_sec - clock_start.tv_sec) + (double) (current_time.tv_nsec - clock_start.tv_nsec) / (double) BILLION;
	
	// Washing stations state update (increase)
	DOWN(count_semid);
	system_data_shm[2]++;
	queue_time_sum[0] += (t2 - t1); // Queue waiting time per car
	UP(count_semid);

	// Car entering one of the washing stations
	DOWN(wash_semid);
	system_data_shm[1]++;
	snprintf(enter_line_shm, 128, "\033[1;32m Car %d entered the the washing station at %lf seconds \033[0m \n", (int) getpid(), timer());
	printf("%s",enter_line_shm);
	UP(wash_semid);

	sleep(nextTime(carWashTimeRate)); // Washing the car..

	// Car exiting one of the washing stations
	DOWN(exit_semid);
	snprintf(exit_line_shm, 128, "\033[0;32m Car %d exited the the washing station at %lf seconds \033[0m\n", (int) getpid(), timer());
	printf("%s",exit_line_shm);
  system_data_shm[3]++;
	kill(getpid(),SIGUSR1); //alert next one in queue
	UP(exit_semid);

	// Washing stations state update (decrease)
	DOWN(count_semid);
	system_data_shm[2]--;
	UP(count_semid);

	return 0;
}

int DOWN(int semid) {
  struct sembuf p_buf;
  p_buf.sem_num = 0;
  p_buf.sem_op = -1;
  p_buf.sem_flg = SEM_UNDO;

	//int semop(int semid, struct sembuf *sops, size_t nsops);
  if( semop(semid, &p_buf, 1) == -1 ) {
    perror("Error operation p(semid)");
    exit(1);
  }
  return 0;
}

int UP(int semid) {
  struct sembuf v_buf;
  v_buf.sem_num = 0;
  v_buf.sem_op = 1;
  v_buf.sem_flg = SEM_UNDO;

	//int semop(int semid, struct sembuf *sops, size_t nsops);
  if( semop(semid, &v_buf, 1) == -1 ) {
    perror("Error operation v(semid)");
    exit(1);
  }
  return 0;
}

int initsem(key_t semkey) {
  int status = 0, semid;

	//int semget(key_t key, int nsems, int semflg);
  if( ( semid = semget(semkey, 1, 0600 | IPC_CREAT | IPC_EXCL ) ) == -1 ) {
    if( errno == EEXIST ) {
      semid = semget( semkey, 1, 0 );
    }
  }
  else {
    union _semun arg;
    arg.val = 1;

    // int semctl(int semid, int semnum, int cmd, union semun arg);
    status = semctl(semid, 0, SETVAL, arg);
  }

  if( semid == -1 || status == -1 ) {
    perror("Error initsem");
    exit(-1);
  }
  return (semid);
}

//Timer function
double timer() {
	struct timespec current_time;
	clock_gettime(CLOCK_REALTIME, &current_time);
	return (current_time.tv_sec - clock_start.tv_sec) + (double) (current_time.tv_nsec - clock_start.tv_nsec) / (double) BILLION;
}

//Next time of the event
float nextTime(float rateParameter) {
	return - logf(1.0f - (float)rand() / (RAND_MAX + 1.0)) / rateParameter;
}

float generateNumberAround(float lambda){
  float maxLambda = lambda + 0.5;
  float minLambda = lambda - 0.5;
  return minLambda + (rand()/(float)RAND_MAX) * (maxLambda - minLambda);
}
