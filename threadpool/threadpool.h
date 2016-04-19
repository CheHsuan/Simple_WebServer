#include <pthread.h>

typedef struct {
	void (*function)(void *);
	void *argument;
} threadpool_task_t;

typedef struct {
	pthread_mutex_t lock;
	pthread_cond_t notify;
	pthread_t *threads;
	threadpool_task_t *queue;
	int thread_num;
	int queue_size;
	int head; 
	int tail;
	int count;
	int shutdown_flag;
}threadpool_t;

int threadpool_init(threadpool_t *pool, int thread_num, int queue_size);
int threadpool_addQueue(threadpool_t *pool, void (*function)(void *), void *arg);
void thread_func(void *thread_arg);
int threadpool_destroy(threadpool_t *pool);
int threadpool_freeMem(threadpool_t *pool);
