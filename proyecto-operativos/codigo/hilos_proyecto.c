
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "poolhilos.h"
#include "poolhilos.c"


#define THREAD 5 // la cantidad de hilos que se puede hacer debe ser menor a 10
#define QUEUE  256 // el valor debe ser menor a  65536




//.................... datos que ocupan las funciones en un struct ............................

struct datos_funciones {
  
  	int num_keys;
	int num_line;
	int *counters;
	char* line;
	char* filename;
	char* token;
	threadpool *pool;
	int id;
};

int tasks = 0, done = 0, d = 0;
clock_t tic,toc;
pthread_mutex_t lock;
char *keywords[6];
char *arreglo[1000000];

//..............  declaracion de funciones ......................................

void line_process(void *datos);
void contadores(void *datos);
void file_process(void *datos);
int keyword_process(char* line, char* keyword);
void print_bar(void *datos);


void contadores (void *datos){
	
	//printf("................................entra a contadores \n");
	struct datos_funciones *data;
	data = (struct datos_funciones *)datos;
	char* token = data->token;
	int num_keys = data->num_keys;

	 while (token!=NULL && num_keys < 6) {
		int size = strlen(token);
		keywords[num_keys] = malloc(sizeof(char)*size+1);

		strncpy(keywords[num_keys],token,size+1);
		//printf("........................................%s\n",keywords[num_keys]);
		token = strtok(NULL, ",");
		num_keys++;
	}

	
	data->token = token;
	data->num_keys = num_keys;
	//printf("num_keys en contadores %i\n",data->num_keys);

	done++;
}

void file_process(void *datos) {
	
	//printf("......................................entra a file_process \n");
	struct datos_funciones *data;
	
	data = (struct datos_funciones *)datos;									
	struct threadpool *pool = (threadpool*)data->pool;

	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int num_keys = data->num_keys;

	char* filename = data->filename;
	int num_line=0;
		
	fp = fopen(filename, "r");
	if (fp == NULL)
		exit(EXIT_FAILURE);

	//printf("data->ID %i\n", data->id);
	int cont=0;

	// obtengo todas las lineas del archivo
	while ((read = getline(&line, &len, fp)) != -1) {

		if (len<80) continue;
		data->line = line;
		data->num_line = num_line;
		data->num_keys = num_keys;
		
		arreglo[cont] = malloc(sizeof(char)*(len+1));
		strcpy(arreglo[cont],line);
		//printf("arreglo de %d: %s\n",cont,arreglo[cont]);
		//printf("contador: %i\n",num_line );
		//printf("num_keys en file_process%i\n",data->num_keys);
		//printf("num_line en file_process%i\n",data->num_line);
		//printf("line: %s\n", data->line);
		//printf("counters%i\n",counters);
		//printf("ID en file_process%i\n",data->id);

		num_line++;
		cont++;
		data->num_line = num_line;    //  se incrementa el numero de linea	
	}	
	
	//empieza la tarea de procesar las lineas
	if(threadpool_anadirTarea(pool, &line_process,(void*)data, 0) == 0) {

        pthread_mutex_lock(&lock);
        tasks++;
        pthread_mutex_unlock(&lock);
	}

	// imprime las baras 
	if(threadpool_anadirTarea(pool, &print_bar,(void*)data, 0) == 0) {

        pthread_mutex_lock(&lock);
        tasks++;
        pthread_mutex_unlock(&lock);
	}


	fclose(fp);
	//if (line)
		//free(line);


	done++;
}

void print_bar(void *datos){
	//printf("...................................entra a imprimir \n");
	struct datos_funciones *data;
	data = (struct datos_funciones *)datos;

	int num_keys= data->num_keys;
	int num_line= data->num_line;
	int counters[500*num_keys];

	// ingresar los datos a pata
	for(int k=0;k<num_line;k++){
		for(int l=0;l<num_keys;l++){
			counters[l*500+k] = data->counters[l*500+k];
			//printf("counters en print%i\n", counters[l*500+k]);
		}

	}

	for (int j=0; j<num_keys; j++) {
		printf("%-15s",keywords[j]);

		for (int i=0; i<num_line; i++) {
				//printf("%s\n", );
			if (counters[j*500+i]==0){
				printf("\u2591");
			}
			else if (counters[j*500+i]==1){
				printf("\u2592");
			}
			else if (counters[j*500+i]==2){
				printf("\u2593");
			}
			else{
				printf("\u2589");
			}
		}
		printf("\n");
	}
	printf("\n\n");
	toc = clock();
	printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);
}


void line_process(void *datos) {

	struct datos_funciones *data;
	data = (struct datos_funciones *)datos;

	threadpool *pool = (struct threadpool*)data->pool;
	int num_keys = data->num_keys;
	int counters[500*num_keys];
	
	int num_line = data->num_line;

	for(int k = 0; k<num_line;k++){
		char* line = arreglo[k];
		//printf("linea que va a procesar %s\n", line );
		for (int i=0; i<num_keys; i++){
			counters[i*500+k] = keyword_process(line,keywords[i]);
			data->counters[i*500+k] = counters[i*500+k];
			//printf("posicion%i\n",i*500+k );
			//printf("data->counters %i\n",data->counters[i*500+k]);
			//printf("keywords %s\n",keywords[i]);
		}
	}	
	done++;
}

// funcion que retorna la cantidad de veces que aparece una palabra en la linea
int keyword_process( char* line, char* keyword) {
	//printf("entra a keyword_process\n");
	int  count =0;
	size_t nlen = strlen (keyword);

	while (line != NULL) {
	  line = strstr (line, keyword);
	  //printf("linea dentro del while %s\n",line );
	  if (line != NULL) {
	  	//printf("count %i\n", count );
	    count++;
	    line += nlen;
	  }
	}
	//printf("count: %i\n",count);
	return count;
}


int main(int argc, char *argv[])
{
    //...........................................................
	char *keywords[6];
	char *token = strtok(argv[1], ",");;
	int num_keys = 0;
	int tasks = 0; 

    struct threadpool *pool;
    struct datos_funciones datos; 

    datos.num_keys = 0;
	datos.token = token;
	datos.id = 1;

    pthread_mutex_init(&lock, NULL);
    tic = clock();
	//...........................................................

	if (argc==1) {
		printf("No files specified\n");
		exit(EXIT_FAILURE);
	}

    assert((pool = threadpool_generar(THREAD, QUEUE, 0)) != NULL);

	datos.pool = (threadpool*)malloc(sizeof(threadpool) * THREAD);
	datos.pool = pool;
	datos.counters = malloc(sizeof(500*num_keys));
   
   if(threadpool_anadirTarea(pool, &contadores,(void*)&datos, 0) == 0) {
        pthread_mutex_lock(&lock);
        tasks++;
        pthread_mutex_unlock(&lock);
    }  

    
    for(int i=2; i< argc; i++){
    	printf("nombre del archivo: %s\n",argv[i]);

    	datos.filename = argv[i];

    	if(threadpool_anadirTarea(pool, &file_process,(void*)&datos, 0) == 0) {
       		 pthread_mutex_lock(&lock);
        		tasks++;
        	pthread_mutex_unlock(&lock);
   		 } 
    }

	//................................................................................

   	while((tasks / 2) > done) {
        usleep(10000);
    }

    fprintf(stderr, "...................Added %d num_keys\n", datos.num_keys);

    
    assert(threadpool_destruir(pool, 0) == 0);
    fprintf(stderr, "Did %d tasks\n", done);

    return 0;
}