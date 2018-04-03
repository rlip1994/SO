
#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#define MAX_THREADS 10
#define MAX_QUEUE 65536



typedef struct threadpool threadpool;

//......................................................................

threadpool *threadpool_generar(int cant_hilos, int tam_cola, int flags);

int threadpool_anadirTarea(threadpool *pool, void (*function)(void *),
                   void *argument, int flags);
int threadpool_destruir(threadpool *pool, int flags);


#endif /* _THREADPOOL_H_ */
