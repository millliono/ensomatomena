/*prod cons works with one of each thread type
  necessary changes to the code
  implemeted struct and load function
  load function claculates 10 sin values
  -Todo:
      -use arguments in load function-DONE
      -use more threads-DONE
      -time -DONE
      -different num of thread types
      -remove sleep -DONE
      -consumer infinite loop - NEVER STOPS
      -report

*/

//producer-consumer

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>


#define QUEUESIZE 10
#define LOOP 20
#define NUM_PRODUCER 10
#define NUM_CONSUMER 20

void *producer (void *args);
void *consumer (void *args);

// struct that contains load
// and timestamp
typedef struct {
  void * (*work)(void *);
  void * arg;
  double produce_time;
} workFunction;

void* work(void* arg) {
  // calculate  sin values for thread load
  int* loops = (int*) arg;
  float result = 0;
  for (int i = 0; i < *loops; i++) {
      result = sin(i*3.14/180);
  }
  char* str = "calculated stuff";
  void* ptr = (void*) str;
  return ptr;
}

workFunction thread_load;
double elapsed_time = 0;

typedef struct {
  workFunction buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in);
void queueDel (queue *q, workFunction *out);

int main ()
{
  queue *fifo;
  pthread_t pros[NUM_PRODUCER];
  pthread_t cons[NUM_CONSUMER];
  
  int rc;
  void *status;

  fifo = queueInit ();
  if (fifo ==  NULL) {
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }

  for(int t=0; t<NUM_PRODUCER; t++) {
  rc = pthread_create(&pros[t], NULL, producer, fifo);
  if (rc) {
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
    }
  }
  for(int t=0; t<NUM_CONSUMER; t++) {
  rc = pthread_create(&cons[t], NULL, consumer, fifo);
  if (rc) {
    printf("ERROR; return code from pthread_create() is %d\n", rc);
    exit(-1);
    }
  }
  for(int t=0; t<NUM_PRODUCER; t++) {
       rc = pthread_join(pros[t], &status);
       if (rc) {
          printf("ERROR; return code from pthread_join() is %d\n", rc);
          exit(-1);
          }
  }
  while (!fifo->empty){
      
  }
  for(int t=0; t<NUM_CONSUMER; t++) {
      pthread_cancel(cons[t]);
  }
  queueDelete (fifo);
  printf("mean elapsed time (us): %f\n ", elapsed_time / LOOP*NUM_PRODUCER);
  return 0;
}

void *producer (void *q)
{
  queue *fifo;
  fifo = (queue *)q;
    
  // reps is the argument to the load function
  int reps = 100;

  for (int i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
    thread_load.work = work;
    thread_load.arg = (void*) &reps;
    struct timeval produce_time;
    gettimeofday(&produce_time, NULL);
    double t1 = produce_time.tv_sec * 1000000 + produce_time.tv_usec;
    thread_load.produce_time = t1;
    queueAdd (fifo, thread_load);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
    printf ("producer:  input item\n");
  }
  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  workFunction d;

  fifo = (queue *)q;

  while(1) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      printf ("consumer: queue EMPTY.\n");
      printf("Thread %ld sleeping\n", pthread_self());
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    struct timeval consume_time;
    gettimeofday(&consume_time, NULL);
    double t2 = consume_time.tv_sec * 1000000 + consume_time.tv_usec;
    queueDel (fifo, &d);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
    elapsed_time += t2 - d.produce_time;
    printf ("consumer:  %s\n", (char*) d.work(d.arg));
  }
  printf("Thread %ld returned\n", pthread_self());
  return (NULL);
}


queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);
	
  return (q);
}

void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);	
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

void queueAdd (queue *q, workFunction in)
{
  q->buf[q->tail] = in;
  q->tail++;
  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel (queue *q, workFunction *out)
{
  *out = q->buf[q->head];

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}