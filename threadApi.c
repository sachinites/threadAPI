#include "threadApi.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

static 
blocked_pool_t gl_blocked_th_pool;

static 
pthread_mutex_t wait_mutex;

void
init_thread_lib(unsigned int max_threads){
	init_blocked_pool(&gl_blocked_th_pool, max_threads);
	pthread_mutex_init(&wait_mutex, NULL);
}


_pthread_t*
get_blocked_thread_from_pool(blocked_pool_t *block_pool){
    int i ;
    for (i = block_pool->pool_size -1; i >=0; i--){
        if(block_pool->blocked_thread_collection[i])
            return block_pool->blocked_thread_collection[i];
    }
        return NULL;
}

void
dump_block_pool(blocked_pool_t *block_pool){
    printf("pool size = %d\n", block_pool->pool_size);
    unsigned int i = 0;
    for(;i < block_pool->pool_size; i++){
        if(block_pool->blocked_thread_collection[i]){
            printf("pool[%d] = %d\n", i, block_pool->blocked_thread_collection[i]->selfid);
             dump_thread_DS(block_pool->blocked_thread_collection[i]);
        }
        else 
            printf("block_pool->blocked_thread_collection[%d] = NULL\n", i );
    }
    return;
}

int
is_thread_in_block_pool(unsigned int thid, blocked_pool_t *block_pool){
    int i ;
    for (i = 0; i < block_pool->pool_size; i++)
    {
        if(block_pool->blocked_thread_collection[i] && 
            block_pool->blocked_thread_collection[i]->selfid == thid){
            return i;
        }
    }
    return FAILURE;
}


int
is_thread_in_block_pool_mutex(unsigned int thid, blocked_pool_t *block_pool){
    int i ;
    pthread_mutex_lock(&(block_pool->pool_mutex));
    for (i = 0; i < block_pool->pool_size; i++)
    {
        if(block_pool->blocked_thread_collection[i] && 
            block_pool->blocked_thread_collection[i]->selfid == thid){
            pthread_mutex_unlock(&(block_pool->pool_mutex));
            return i;
        }
    }
    pthread_mutex_unlock(&(block_pool->pool_mutex));
    return FAILURE;
}

void
init_blocked_pool(blocked_pool_t *block_pool, unsigned int pool_size){
    int i = 0;
    block_pool->pool_size = pool_size;
    for (i = 0; i < pool_size; i++)
        block_pool->blocked_thread_collection[i] = NULL;
    pthread_mutex_init(&(block_pool->pool_mutex), NULL);
    return;
}

int
get_empty_slot_from_pool(blocked_pool_t *block_pool){
    int i;
    for(i = block_pool->pool_size -1; i >=0; i--){
        if(block_pool->blocked_thread_collection[i] == NULL)
            return i;
    }
    return FAILURE;
}


int
get_empty_slot_from_pool_mutex(blocked_pool_t *block_pool){
    int i;
    pthread_mutex_lock(&(block_pool->pool_mutex));
    for(i = block_pool->pool_size -1; i >=0; i--){
        if(block_pool->blocked_thread_collection[i] == NULL)
            pthread_mutex_unlock(&(block_pool->pool_mutex));
            return i;
    }
    pthread_mutex_unlock(&(block_pool->pool_mutex));
    return FAILURE;
}

int
remove_thread_from_pool(blocked_pool_t *block_pool, _pthread_t *thread){
    int loc = -1;
    if(thread->selfid > (block_pool->pool_size - 1)) return FAILURE;
    pthread_mutex_lock(&(block_pool->pool_mutex));
    if((loc = is_thread_in_block_pool(thread->selfid, block_pool)) > -1)
    {
        block_pool->blocked_thread_collection[loc] = NULL;
        printf("thread %d is removed from gl_blocked_th_pool\n", thread->selfid);
    }
    else
    {
        printf("thread %d already does not exist in gl_blocked_th_pool\n", thread->selfid);
    }
    pthread_mutex_unlock(&(block_pool->pool_mutex));
    return loc;
}

void 
dump_thread_DS(_pthread_t *thread)
{
    printf("===================================\n");
    printf("selfid = %d\n", thread->selfid);
    printf("pthread_handle = %ld\n", thread->pthread_handle);
    printf("isWaiting = %d\n", thread->isWaiting);
    printf("resume_thread_id = %d\n", thread->resume_thread_id);
    printf("attr = 0x%x\n", (unsigned int )&(thread->attr));
    printf("cond = 0x%x\n", (unsigned int )&(thread->cond));
    printf("===================================\n");
    return;
}

static int 
_pool_add(blocked_pool_t *block_pool, _pthread_t *thread)
{
    int loc = -1;
    if(thread->selfid > block_pool->pool_size -1) return -1;
    loc = get_empty_slot_from_pool(block_pool);
    if(loc > -1){
        block_pool->blocked_thread_collection[loc] = thread;
        printf("thread %d is added to gl_blocked_th_pool at index %d\n", thread->selfid, loc);
        return loc;
    }
    else{
        printf("gl_blocked_th_pool is full, thread %d cannot be added\n", thread->selfid);
        return FAILURE;
    }
}

int 
add_thread_to_pool(blocked_pool_t *block_pool , _pthread_t *thread)
{
    if (!block_pool) return -1;
    int rc = SUCCESS;
    pthread_mutex_lock(&block_pool->pool_mutex);

    if(is_thread_in_block_pool(thread->selfid, block_pool) < 0)
    {
        rc = _pool_add(block_pool, thread);
        pthread_mutex_unlock(&block_pool->pool_mutex);
        return rc;
    }
    else
    {
        printf("Thread %d is already in blocked pool\n", thread->selfid);
        pthread_mutex_unlock(&block_pool->pool_mutex);
        return FAILURE;
    }
}

void pthread_init(_pthread_t *_pthread, unsigned int tid, unsigned int JOINABLE)
{
    _pthread->selfid = tid;
    pthread_attr_init(&_pthread->attr);
    JOINABLE ? pthread_attr_setdetachstate(&_pthread->attr, PTHREAD_CREATE_JOINABLE):
        pthread_attr_setdetachstate(&_pthread->attr, PTHREAD_CREATE_DETACHED);
    pthread_cond_init(&_pthread->cond, NULL);
    _pthread->isWaiting = FALSE;
    _pthread->resume_thread_id = -1;
}

void cleanup_pthread(_pthread_t *thread)
{
    pthread_attr_destroy(&(thread->attr));
    pthread_cond_destroy(&(thread->cond));
    return;
}

void
wait_t (_pthread_t *thread_to_block, unsigned int line_no){
        pthread_mutex_lock(&wait_mutex);
        thread_to_block->isWaiting = TRUE;
        if(pthread_cond_wait (&(thread_to_block->cond), &wait_mutex)){
            printf("pthread_cond_wait failed, thread id = %d, line_no = %d", thread_to_block->selfid, line_no);
            thread_to_block->isWaiting = FALSE;
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&wait_mutex);
}

void
signal_t (_pthread_t *signalled_thread){
    remove_thread_from_pool(&gl_blocked_th_pool, signalled_thread);
    if(pthread_cond_signal(&(signalled_thread->cond))){
        pthread_exit(NULL);
    }
    signalled_thread->isWaiting = FALSE;
    signalled_thread->resume_thread_id = 0;
}

void tentative_wait(_pthread_t *thread){
	if(is_thread_in_block_pool_mutex(0, &gl_blocked_th_pool) > -1)
		wait_t(thread,  __LINE__);

}

void
send_wait_order(_pthread_t *thread){
	add_thread_to_pool(&gl_blocked_th_pool, thread);
}

int
wait_event(_pthread_t *thread, wait_queue_t *wait_q){

	char res = 0;
	pthread_mutex_lock(wait_q->mutex);
	res = wait_q->condn_callback_fn(wait_q->fn_arg);
	if(res){
		/* thread should get block*/
		if(enqueue(wait_q->q, thread) == 0){
			pthread_mutex_unlock(wait_q->mutex);
			return -1;
		}

		wait_t (thread, __LINE__);	
	}
	else{
		/* thread should not get block - Dont do anything*/
	}
	pthread_mutex_unlock(wait_q->mutex);
	return 0;
}

void
wake_up(wait_queue_t *wait_q){

	char res = 0;
	_pthread_t *thread = NULL;
	int i  = 0, j = wait_q->q->count;
	struct Queue_t *q = wait_q->q;

	pthread_mutex_lock(wait_q->mutex);
	for(i = 0; i < j; i++){
		res = wait_q->condn_callback_fn(wait_q->fn_arg);
		if(res){
			/* stay blocked*/
			enqueue(q, deque(q));
		}
		else{
			thread = deque(q);
			signal_t(thread);
		}
	}
	pthread_mutex_unlock(wait_q->mutex);	
}

void
init_wait_Queue(wait_queue_t *wait_q, condn_check_fn fn, void *fn_arg){
        memset(wait_q, 0, sizeof(wait_queue_t));
	wait_q->q = initQ();
	wait_q->condn_callback_fn = fn;
	wait_q->fn_arg = fn_arg;
	pthread_mutex_t *mutex = calloc(1, sizeof(pthread_mutex_t));
	pthread_mutex_init(mutex, NULL);
	wait_q->mutex = mutex;
}

/* fn arg, and wait_q itself needs to be freed in the caller*/
void
free_wait_queue_internals(wait_queue_t *wait_q){
	free(wait_q->q);
	free(wait_q->mutex);
	wait_q->mutex = NULL;
	wait_q->q = NULL;
	wait_q->condn_callback_fn = NULL;
	//wait_q->fn_arg = NULL;
}
