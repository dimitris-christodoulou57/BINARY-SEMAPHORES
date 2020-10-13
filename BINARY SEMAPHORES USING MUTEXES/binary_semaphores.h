#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

typedef struct {
	int value;
	int onqueue;
	pthread_mutex_t mtx;
	pthread_mutex_t queue;
}mybsem_t;

int mybsem_init(mybsem_t *sem, int value){
	
	pthread_mutexattr_t attr;
	int res;
	
	res = pthread_mutexattr_init(&attr);
	
	res = pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_ERRORCHECK);
	
	res=pthread_mutex_init(&(sem->mtx),&attr);
	
	if (res){
		printf("error init mutex\n");
	}
	
	res = pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_NORMAL);
	
	res=pthread_mutex_init(&(sem->queue),&attr);
	
	if (res){
		printf("error init mutex\n");
	}
	
	pthread_mutexattr_destroy(&attr);
	
	sem->value = value;
	sem->onqueue = 0;
	pthread_mutex_lock(&(sem->queue));
	
	return 0;
}

int mybsem_destroy(mybsem_t *sem){
	
	
	pthread_mutex_lock(&(sem->mtx));
	if (sem->onqueue != 0){
		printf("error mybsem_destroy mutex unlocked\n");
		pthread_mutex_unlock(&(sem->mtx));
		return -1;
	}
	pthread_mutex_unlock(&(sem->mtx));
	
	pthread_mutex_unlock(&(sem->queue));
	
	pthread_mutex_destroy(&(sem->mtx));
	
	pthread_mutex_destroy(&(sem->queue));
	
	return 0;
}

void mybsem_up(mybsem_t *sem){
	
	pthread_mutex_lock(&(sem->mtx));
	if (sem->value==0){
		if(sem->onqueue != 0) {
			pthread_mutex_unlock(&(sem->queue));
			(sem->onqueue)--;
		}
		else {
			sem->value=1;///////////////////////////////
		}
		pthread_mutex_unlock(&(sem->mtx));
	}
	else{
		pthread_mutex_unlock(&(sem->mtx));
	}
}

void mybsem_down(mybsem_t *sem) {
	
	pthread_mutex_lock(&(sem->mtx));
	if (sem->value==1){
		sem->value=0;
		pthread_mutex_unlock(&(sem->mtx));
	}
	else{
		(sem->onqueue)++;
		pthread_mutex_unlock(&(sem->mtx));
		pthread_mutex_lock(&(sem->queue));
	}
}
