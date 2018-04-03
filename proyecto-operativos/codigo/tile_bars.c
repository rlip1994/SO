#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int keyword_process(char* line, char* keyword) {
	int count;
	size_t nlen = strlen (keyword);

	while (line != NULL) {
	  line = strstr (line, keyword);
	  if (line != NULL) {
	    count++;
	    line += nlen;
	  }
	}
	printf("count: %i\n",count );
	return count;
}

void line_process(char* line, int line_id, char *keywords[], int num_keys, int *counters) {

	for (int i=0; i<num_keys; i++){
		counters[i*500+line_id] = keyword_process(line,keywords[i]);
		printf("posicion%i\n",i*500+line_id );
		printf("counters%i\n",counters[i*500+line_id] );
	}	
}

void print_bar(char *keywords[], int num_keys, int num_line, int* counters) {
	for (int j=0; j<num_keys; j++) {
		printf("%-15s",keywords[j]);
		for (int i=0; i<num_line; i++) {
			if (counters[j*500+i]==0)
				printf("\u2591");
			else if (counters[j*500+i]==1)
				printf("\u2592");
			else if (counters[j*500+i]==2)
				printf("\u2593");
			else
				printf("\u2589");
		}
		printf("\n");
	}
	printf("\n\n");
}

void file_process(char* filename, char *keywords[], int num_keys) {
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int counters[500*num_keys];

	fp = fopen(filename, "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);

	int num_line=0;
	while ((read = getline(&line, &len, fp)) != -1) {
		if (len<80) continue;
		printf("line: %s\n",line );
		line_process(line,num_line,keywords,num_keys,&counters);
		num_line++;
	}

	print_bar(keywords,num_keys, num_line,counters);
	
	fclose(fp);
	if (line)
		free(line);
}


int main (int argc, char *argv[]) {
	char *keywords[6];
	
	if (argc==1) {
		printf("No files specified\n");
		exit(EXIT_FAILURE);
	}
	clock_t tic = clock();
	
	int num_keys=0;
	char *token = strtok(argv[1], ",");
	
	while (token!=NULL && num_keys < 6) {
		int size = strlen(token);
		keywords[num_keys] = malloc(sizeof(char)*size+1);
		strncpy(keywords[num_keys],token,size+1);
		token = strtok(NULL, ",");
		num_keys++;
		printf("token: %s\n", token );
	}

	
	for (int i=2; i< argc; i++) {
		printf("%s\n",argv[i]);
		file_process(argv[i],keywords,num_keys);
	}
	
	clock_t toc= clock();
	
	printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);
	
	exit(EXIT_SUCCESS);
}