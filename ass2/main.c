#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

typedef struct Flow
{
  int id;
  int a_time; // arrival time
  int t_time; // transmission time
  int priority;
} Flow;

typedef struct G_data
{
  // Priority queue using simple sorted array
  Flow * q;
  pthread_mutex_t q_mutex;
  pthread_cond_t q_cv;

  // Mutex / cv pair for starting a transmission
  pthread_mutex_t start_mutex;
  pthread_cond_t start_cv;
  int start_id;

  // Mutex / cv pair for transmission / letting main thread know that the transmission is done
  pthread_mutex_t end_mutex;
  pthread_cond_t end_cv;

  pthread_mutex_t count_mutex;
  int thread_count;

  struct timeval stime;

} G_data;

G_data g_data;


// Pushes to the queue simply by inserting into the right place in the array
void pushQ(Flow* q, Flow f)
{
  int i = 0;
  while(q[i].priority != 0)
    {
      if(f.priority < q[i].priority ||
         (f.priority == q[i].priority &&
          (f.a_time < q[i].a_time || (f.a_time == q[i].a_time && f.t_time < q[i].t_time))
          ))
        {
          i++;
        }
      else
        {
          break;
        }
    }

  int j = i;
  while(q[j].priority != 0) j++;

  for(; j > i; j--)
    {
      q[j] = q[j-1];
    }

  q[i] = f;
}

// Gets the top (highest priority) element from the queue
Flow popQ(Flow* q)
{
  int i = 0;
  while(q[i+1].priority != 0) i++;
  Flow f = q[i];
  q[i].priority = 0;
  return f;
}

// Checks if queue is empty
int isEmpty(Flow* q)
{
  return (q[0].priority == 0);
}

// Gets time in seconds since the beginning of the simulation
double getTimeDiff()
{
  struct timeval cur_time;
  gettimeofday(&cur_time, NULL);
  return (cur_time.tv_sec - g_data.stime.tv_sec) + ((cur_time.tv_usec - g_data.stime.tv_usec) / 1000000.0);
}

// Called in a new thread, simulates a flow
void *processFlow(void *t)
{
  Flow f = *((Flow *) t);
  usleep(f.a_time * 100000);
  double atime = getTimeDiff();
  double ttime = f.t_time / 10.0;
  printf("Flow %2d arrives: arrival time (%.2f), transmission time (%.1f), priority (%2d). \n" , f.id, atime, ttime, f.priority);

  // Lock to add this process to the global queue
  pthread_mutex_lock(&g_data.q_mutex);
  pushQ(g_data.q, f);
  if(g_data.start_id >= 0)
    {
      printf("Flow %2d waits for the finish of the flow %2d. \n" , f.id, g_data.start_id);
    }
  pthread_cond_signal(&g_data.q_cv);
  pthread_mutex_unlock(&g_data.q_mutex);

  // Lock to wait for main thread to choose this flow from the queue
  pthread_mutex_lock(&g_data.start_mutex);
  while(g_data.start_id != f.id)
    {
      pthread_cond_wait(&g_data.start_cv, &g_data.start_mutex);
    }
  pthread_mutex_unlock(&g_data.start_mutex);

  // Lock for transmission
  pthread_mutex_lock(&g_data.end_mutex);
  printf("Flow %2d starts its transmission at time %.2f.\n" , f.id, getTimeDiff());
  usleep(f.t_time * 100000);
  printf("Flow %2d finishes its transmission at time %.2f.\n" , f.id, getTimeDiff());
  // Reset the start id and signal main thread to find a new flow
  g_data.start_id = -1;
  pthread_cond_signal(&g_data.end_cv);
  pthread_mutex_unlock(&g_data.end_mutex);

  // Decrement the thread count
  pthread_mutex_lock(&g_data.count_mutex);
  g_data.thread_count--;
  pthread_mutex_unlock(&g_data.count_mutex);
  return;
}

int main(int argc, char** argv)
{
  // First read the input file
  FILE * fp;

  if(argc <= 1)
    {
      printf("Usage: MFS [filename]\n");
      return 1;
    }
  fp = fopen(argv[1], "r");
  if (fp == NULL)
    {
      printf("Failed to open file\n");
      return 2;
    }

  int num_flows;
  fscanf(fp, "%d\n", &num_flows);

  Flow * flows = malloc(sizeof(Flow) * num_flows);

  int i;
  for(i = 0; i < num_flows; i++)
    {
      fscanf(fp, "%d:%d,%d,%d",&flows[i].id,&flows[i].a_time, &flows[i].t_time, &flows[i].priority);
    }

  fclose(fp);

  // Initialize shared data
  g_data.q = calloc(num_flows + 1, sizeof(Flow));
  g_data.start_id = -1;
  pthread_mutex_init(&g_data.q_mutex, NULL);
  pthread_cond_init(&g_data.q_cv, NULL);
  pthread_mutex_init(&g_data.start_mutex, NULL);
  pthread_cond_init(&g_data.start_cv, NULL);
  pthread_mutex_init(&g_data.end_mutex, NULL);
  pthread_cond_init(&g_data.end_cv, NULL);
  pthread_mutex_init(&g_data.count_mutex, NULL);


  int num_threads = num_flows;
  pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
  g_data.thread_count = num_threads;

  gettimeofday(&g_data.stime, NULL);

  // create threads
  for(i = 0; i < num_flows; i++)
    {
      pthread_create(&threads[i], NULL, processFlow, &flows[i]);
    }

  // main loop for managing the threads, ran until every thread is closed
  while(g_data.thread_count > 0)
    {
      pthread_mutex_lock(&g_data.q_mutex);
      Flow f;
      // if the queue is empty, wait for a flow
      if(isEmpty(g_data.q))
        {
          pthread_cond_wait(&g_data.q_cv, &g_data.q_mutex);
        }
      f = popQ(g_data.q);
      pthread_mutex_unlock(&g_data.q_mutex);

      pthread_mutex_lock(&g_data.start_mutex);
      g_data.start_id = f.id;
      pthread_cond_broadcast(&g_data.start_cv);
      pthread_mutex_unlock(&g_data.start_mutex);

      pthread_mutex_lock(&g_data.end_mutex);
      // check if transmission is already done at this point and if not, wait
      if(g_data.start_id >= 0)
        {
          pthread_cond_wait(&g_data.end_cv, &g_data.end_mutex);
        }
      pthread_mutex_unlock(&g_data.end_mutex);
    }


  // cleanup
  for(i = 0; i < num_flows; i++)
    {
      pthread_join(threads[i], NULL);
    }

  free(flows);
  free(threads);
  free(g_data.q);

  pthread_mutex_destroy(&g_data.q_mutex);
  pthread_cond_destroy(&g_data.q_cv);
  pthread_mutex_destroy(&g_data.start_mutex);
  pthread_cond_destroy(&g_data.start_cv);
  pthread_mutex_destroy(&g_data.end_mutex);
  pthread_cond_destroy(&g_data.end_cv);

  return 0;
}

