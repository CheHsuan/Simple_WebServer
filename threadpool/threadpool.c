#include <stdlib.h>
#include "threadpool.h"

void thread_func(void *thread_arg)
{
	threadpool_t *pool = (threadpool_t *)thread_arg;
	threadpool_task_t task;
	while(1){
		pthread_mutex_lock(&(pool->lock));

		//if there is no task then blocked 
		while((pool->count == 0) && (pool->shutdown_flag == 0))
			pthread_cond_wait(&(pool->notify), &(pool->lock));

		//leave
		if((pool->count == 0) && (pool->shutdown_flag == 1)){
			pthread_mutex_unlock(&(pool->lock));
			pthread_exit(0);
		}

		//assign the task
		task.function = pool->queue[pool->head].function;
		task.argument = pool->queue[pool->head].argument;
		pool->head += 1;
		pool->head = (pool->head == pool->queue_size) ? 0 : pool->head;
		pool->count -= 1;
		pthread_mutex_unlock(&(pool->lock));
		//execute the task
		(*(task.function))(task.argument);	
	}
}

int threadpool_addQueue(threadpool_t *pool, void (*function)(void *), void *arg)
{	
	pthread_mutex_lock(&(pool->lock));

	if(pool->shutdown_flag == 1){
		pthread_mutex_unlock(&(pool->lock));
		return -2;
	}

	//check the queue, if full then return error
	if(pool->count == pool->queue_size){
		pthread_mutex_unlock(&(pool->lock));
		return -1;
	}

	//add task to queue
	pool->queue[pool->tail].function = function;
	pool->queue[pool->tail].argument = arg;
	pool->tail += 1;
	pool->tail = (pool->tail == pool->queue_size) ? 0 : pool->tail;
	pool->count += 1;
	pthread_cond_signal(&(pool->notify));

	pthread_mutex_unlock(&(pool->lock));
	return 0;
}

int threadpool_init(threadpool_t *pool, int thread_num, int queue_size)
{
	if((thread_num <= 0) || (queue_size <= 0))
		return -1;
	pool->queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);
	pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
	pthread_mutex_init(&(pool->lock), NULL);
	pthread_cond_init(&(pool->notify), NULL);
	pool->thread_num = thread_num;
	pool->queue_size = queue_size;
	pool->head = 0;
	pool->tail = 0;
	pool->count = 0;
	pool->shutdown_flag = 0;
	for(int i = 0; i < thread_num; i++)
		pthread_create(&(pool->threads[i]), NULL,(void *)&thread_func, (void *)pool);
	return 0;
}

int threadpool_destroy(threadpool_t *pool)
{
	pool->shutdown_flag = 1;
	pthread_cond_broadcast(&(pool->notify));
	for(int i = 0; i < pool->thread_num; i++)
		pthread_join(pool->threads[i], NULL);
	return 0;
}

int threadpool_freeMem(threadpool_t *pool)
{
	if(pool->threads) {
		free(pool->threads);
		free(pool->queue);

		pthread_mutex_lock(&(pool->lock));
		pthread_mutex_destroy(&(pool->lock));
		pthread_cond_destroy(&(pool->notify));
	}
	free(pool);    
	return 0;
}
