#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/shm.h> //Memoria compartida
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define POOL 30
#define FILEKEY "/bin/cat"
#define KEY 500
#define KEYNUM 6000
#define MAXBUF 10

clock_t tic;
int num_zone;
int id_zone;
int *estado;

int keyword_process(char* line, char* keyword) {
	int count = 0;
	size_t nlen = strlen (keyword);

	while (line != NULL) {
	  line = strstr (line, keyword);
	  if (line != NULL) {
	    count++;
	    line += nlen;
	  }
	}
	return count;
}

void line_process(char* line, int line_id, char *keywords[], int num_keys, int *counters) {

	for (int i=0; i<num_keys; i++){
		counters[i*500+line_id] = keyword_process(line,keywords[i]);
	}
}

void print_bar(char *keywords[], int num_keys, int num_line, int* counters) {

	int wait = 0;

	while(estado[4] == 0){
		int key_line = estado[3];

		if(estado[5] != 1){
			estado[5] = 1;
			shmctl(id_zone, SHM_LOCK, (struct shmid_ds*)NULL); 
			printf("%-15s",keywords[key_line]);
			for (int i=0; i<num_line; i++) {
				if (counters[key_line*500+i]==0){
					printf("\u2591");
				}
				else if (counters[key_line*500+i]==1){
					printf("\u2592");
				}
				else if (counters[key_line*500+i]==2){
					printf("\u2593");
				}
				else{
					printf("\u2589");
				}
				printf(" ");
			}
			printf("\n");
			
			key_line++;

			if(key_line == num_keys){
				wait = 1;
			}

			shmctl(id_zone, SHM_UNLOCK, (struct shmid_ds*)NULL);
			estado[3] =key_line;
			estado[4] = wait;
			estado[5] = 0;
		}
		usleep(50000*POOL);
	}

	
}

void file_process(char* filename, char *keywords[], int num_keys) {
	int parent_pid = getpid();
	int pid;
	int status;

	// creacion del pool de procesos 
	for (int i=0; i<POOL; ++i){
		if ((pid = fork()) <=0 ) {
			break;
		}
	}

	int key = ftok(FILEKEY, KEY);
	int keyNum = ftok(FILEKEY, KEYNUM);
    if (key == -1 || keyNum == -1) { 
       fprintf (stderr, "Error with key \n");
    }

    /* we create the shared memory */
    id_zone = shmget (key, sizeof(int)*500*num_keys, 0777 | IPC_CREAT);
    int num_zone = shmget (keyNum, sizeof(int)*MAXBUF, 0777 | IPC_CREAT);
    if (num_zone == -1 || id_zone == -1) {
       fprintf (stderr, "Error with id_zone \n");
    }

	int* counters;

    counters = (int*)shmat (id_zone, (char *)0, 0);
    estado = (int*)shmat (num_zone, (char*)0,0);

	if(pid == 0){


		counters = (int*)shmat (id_zone,NULL, 0);
		estado = (int*)shmat (num_zone,NULL, 0);

		int wait = 0;
		while(estado[1] == 0){
			
			int num_line = estado[0];

			FILE * fp;
			size_t len = 0;
			ssize_t read;
			char * line = NULL;

			fp = fopen(filename, "r");
			if (fp == NULL){
				exit(EXIT_FAILURE);
			}

			if(estado[2] != 1){
				estado[2] = 1;
				shmctl(num_zone, SHM_LOCK, (struct shmid_ds*)NULL); 
					
					if(wait == 0){						
						for (int i = 0; i < num_line; ++i){
							(read = getline(&line, &len, fp));
						}
						if((read = getline(&line, &len, fp)) == -1){
							wait = 1;
							shmctl(num_zone, SHM_UNLOCK, (struct shmid_ds*)NULL);
							estado[0] = num_line;
							estado[1] = wait;

						}else{
							printf("Hijo %d y la línea es %d  --", getpid(), num_line);
							printf("y dice: %s\n", line);
							line_process(line,num_line,keywords,num_keys,counters);
							num_line++;
						}

					}
				shmctl(num_zone, SHM_UNLOCK, (struct shmid_ds*)NULL);
				estado[0] = num_line;
				estado[1] = wait;
				estado[2] = 0;
				fclose(fp);
				if (line){
					free(line);
				}
			}
			usleep(50000*POOL);
		}

		print_bar(keywords,num_keys, estado[0],counters);

		
	}else if(pid == -1){
		printf("ERROR FORK\n");
	}else{
		for (int i=0; i<POOL; ++i){
			if (getpid()==parent_pid) {
				pid = wait(&status);
			}
		}

		printf("Líneas totales %d\n", estado[0]);
		shmctl(num_zone, SHM_UNLOCK, (struct shmid_ds*)NULL);
		estado = (int*)shmat (num_zone,NULL, 0);
		estado[0] = 0;
		estado[1] = 0;
		estado[2] = 0;
		estado[3] = 0;
		estado[4] = 0;
		estado[5] = 0;

		clock_t toc= clock();
		printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);

	}
	
	//while ((read = getline(&line, &len, fp)) != -1) {
		//if (len<80) continue;
		//
	//}

	//print_bar(keywords,num_keys, num_line,counters);
	
	shmdt((int*) counters);
	shmctl(id_zone, IPC_RMID, (struct shmid_ds*)NULL);

	shmdt((char*) estado);
	shmctl(num_zone, IPC_RMID, (struct shmid_ds*)NULL);
}


int main (int argc, char *argv[]) {
	char *keywords[6];
	
	if (argc==1) {
		printf("No files specified\n");
		exit(EXIT_FAILURE);
	}

	
	tic = clock();

	int num_keys=0;
	char *token = strtok(argv[1], ",");
	while (token!=NULL && num_keys < 6) {
		int size = strlen(token);
		keywords[num_keys] = malloc(sizeof(char)*size+1);
		strncpy(keywords[num_keys],token,size+1);
		token = strtok(NULL, ",");
		num_keys++;
	}
	
	for (int i=2; i< argc; i++) {
		printf("%s\n",argv[i]);
		file_process(argv[i],keywords,num_keys);
	}

	exit(EXIT_SUCCESS);
}