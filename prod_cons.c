/*prod cons works with one of each thread type
  necessary changes to the code
  implemeted struct and load function
  load function claculates 10 sine values
  -Todo:
      -use arguments in load function-DONE
      -use more threads-DONE
      -time
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
#define NUM_CONSUMER 3
#define NUM_PRODUCER 3

void *producer (void *args);
void *consumer (void *args);

//*****************
typedef struct {
  void * (*work)(void *);
  void * arg;
} workFunction;

void* work(void* arg) { //TODO: use arg
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

// initialize thread load 
int ld = 100;
workFunction thread_load = {work, (void*) &ld};
//**********************

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
  for(int t=0; t<NUM_CONSUMER; t++) {
      rc = pthread_join(cons[t], &status);
      if (rc) {
        printf("ERROR; return code from pthread_join() is %d\n", rc);
        exit(-1);
        }
  }

  queueDelete (fifo);

  return 0;
}

void *producer (void *q)
{
  queue *fifo;
  int i;

  fifo = (queue *)q;

  for (i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      printf ("producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }
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
  int i;
  workFunction d;

  fifo = (queue *)q;

  for (int i = 0; i < LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      printf ("consumer: queue EMPTY.\n");
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }
    queueDel (fifo, &d);
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
    printf ("consumer:  %s\n", (char*) thread_load.work(thread_load.arg));
  }

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