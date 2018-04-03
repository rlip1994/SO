
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "poolhilos.h" 


//.............................................

// formas de apagado 
typedef enum {
    apagado_inmediato = 1, 
    apagado_descente  = 2
} threadpool_tipoApagado;

typedef enum {
    threadpool_graceful       = 1
} threadpool_destroy_flags_t;

// struct para pasar las funciones 
typedef struct {
    void (*function)(void *); // nombre de la funcion
    void *argument; // argumentos que tiene la funcion (entradas)
} threadpool_tareas;


//.............................................

struct threadpool {

  // datos necesarios que debe el hilo para manejarlo en el pool
  pthread_mutex_t lock;
  pthread_cond_t notify; // encargado de notificar a los hilos 
  pthread_t *hilos; // array que contiene los hilos. 
  threadpool_tareas *cola; // struct para indicar la tarea 
  int cant_hilos; // numero de hilos que hay o se crearon
  int tam_cola; // tamano de la cola tarea
  int head; // indexa el primer elemenot
  int tail; // indexa el siguiente elemento
  int cant; // numero de tareas pendientes
  int estado; // pool esta apagada (flag)
  int started; // subprocesos iniciados 

};

/*funcion principal para manejar el hilo en el pool*/
static void *threadpool_hilos(void *threadpool);

int threadpool_liberar(threadpool *pool);

void manejoErrores(int numError);



/*funcion que crea los hilos  que van a conformar el pool */
threadpool *threadpool_generar(int cant_hilos, int tam_cola, int flags) 
{
    threadpool *pool;
    (void) flags;

    // validaciones de las entradas 
    if(cant_hilos <= 0 || cant_hilos > MAX_THREADS || tam_cola <= 0 || tam_cola > MAX_QUEUE) {
        return NULL;
    }

    if((pool = (threadpool *)malloc(sizeof(threadpool))) == NULL) {
        goto err;
    }

    /* inicializar datos  */
    pool->cant_hilos = 0;
    pool->tam_cola = tam_cola;
    pool->head = pool->tail = pool->cant = 0;
    pool->estado = pool->started = 0;
    pool->hilos = (pthread_t *)malloc(sizeof(pthread_t) * cant_hilos);
    pool->cola = (threadpool_tareas *)malloc(sizeof(threadpool_tareas) * tam_cola); 

    
    if((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
       (pthread_cond_init(&(pool->notify), NULL) != 0) ||
       (pool->hilos == NULL) ||
       (pool->cola == NULL)) {
        goto err;
    }

    /* creacion de hilos */
    for(int i = 0; i < cant_hilos; i++) {
        if(pthread_create(&(pool->hilos[i]), NULL,
                          threadpool_hilos, (void*)pool) != 0) {
            threadpool_destruir(pool, 0);
            return NULL;
        }
        pool->cant_hilos++;
        pool->started++;
    }

    return pool;

 err:
    if(pool) {
        threadpool_liberar(pool);
    }
    return NULL;
}

int threadpool_anadirTarea(threadpool *pool, void (*function)(void *),
                   void *argument, int flags)
{   
    int err = 0;
    int next;
    (void) flags;

    /*validaciones de las entradas*/

    if(pool == NULL || function == NULL) {
        err =1;
        manejoErrores(-1);
    }

    if(pthread_mutex_lock(&(pool->lock)) != 0) {
        err =1;
        manejoErrores(-2);
    }

    next = (pool->tail + 1) % pool->tam_cola;

    /*asignacion de la tarea a los hilos*/

    do {
        // ..... validaciones .....

        /* validar si la cola  de tareas esta llena */
        if(pool->cant == pool->tam_cola) {
            err =1;
            manejoErrores(-3);
            break;
        }

        /* validar si pool esta apagado */
        if(pool->estado) {
            err =1;
            manejoErrores(-4);
            break;
        }

        //..... ingresar la tarea en la cola ..... 

        pool->cola[pool->tail].function = function;
        pool->cola[pool->tail].argument = argument;
        pool->tail = next;
        pool->cant += 1;

         
        if(pthread_cond_signal(&(pool->notify)) != 0) {
            err =1;
            manejoErrores(-2);             
            break;
        }
    } while(0);

    if(pthread_mutex_unlock(&pool->lock) != 0) {
        err =1;
        manejoErrores(-2);   
        
    }
    return err;
}

int threadpool_destruir(threadpool *pool, int flags)
{
    int i, err = 0; // 0 = todo salio bien  ...  1 = ocurrio algun error 

    if(pool == NULL) {
        manejoErrores(-1);
        err = 1;
    }

    if(pthread_mutex_lock(&(pool->lock)) != 0) {
        manejoErrores(-2);
        err = 1;
    }

    do {
        /* todo apagado */
        if(pool->estado) {
           manejoErrores(-4);
           err = 1;
            break;
        }
       
        pool->estado = (flags & threadpool_graceful) ?
            apagado_descente : apagado_inmediato;

        /* despertador de hilos */
        if((pthread_cond_broadcast(&(pool->notify)) != 0) ||
           (pthread_mutex_unlock(&(pool->lock)) != 0)) {
            manejoErrores(-2);
            err = 1;
            break;
        }

        /* unir trabajadores */
        for(i = 0; i < pool->cant_hilos; i++) {
            if(pthread_join(pool->hilos[i], NULL) != 0) {
                manejoErrores(-5);
                err = 1;
            }
        }
    } while(0);

    /* todo salio bien, libera */
    if(!err) {
        threadpool_liberar(pool);
    }
    return err;
}

// liberar pool 
int threadpool_liberar(threadpool *pool)
{
    if(pool == NULL || pool->started > 0) {
        return -1;
    }

    if(pool->hilos) {
        free(pool->hilos);
        free(pool->cola);
 
        /* bloqueo del mutex y la variable de condicion*/
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
    }
    free(pool);    
    return 0;
}


static void *threadpool_hilos(void *threadPool)
{
     // structs 
    threadpool *pool = (threadpool *)threadPool;
    threadpool_tareas tarea;

    for(;;) {
        /* espera a la variable condicional */
        pthread_mutex_lock(&(pool->lock));

        /* espera la variable condicional  y bloquea */
        while((pool->cant == 0) && (!pool->estado)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        // todo apagado
        if((pool->estado == apagado_inmediato) ||
           ((pool->estado == apagado_descente) &&
            (pool->cant == 0))) {
            break;
        }

        /* indicar tarea */
        tarea.function = pool->cola[pool->head].function;
        tarea.argument = pool->cola[pool->head].argument;
        pool->head = (pool->head + 1) % pool->tam_cola;
        pool->cant -= 1;

        /* desbloqueo de mutex*/
        pthread_mutex_unlock(&(pool->lock));

        /* trabaja / ejecuta las funciones con sus argumentos respectivos */
        (*(tarea.function))(tarea.argument);
    }

    pool->started--;
     // cuando se termina de realizar las tareas se desbloquea 
    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return(NULL);
}

void manejoErrores(int numError){
    switch(numError){

        case -1:      /* pool invalido*/
            printf("pool invalido \n");
            break;
        case -2:     /* lock fail */ 
            printf("lock fail\n");
            break;
        case -3:     /* cola llena*/
            printf("cola llena\n");
            break;
        case -4:    /* shutdown*/
            printf("shutdownl\n");
            break;
        case -5:    /* hilo fail*/
            printf("hilo fail\n");
            break;

    }
}
