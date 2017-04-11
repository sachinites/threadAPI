#ifndef __QUEUE__
#define __QUEUE__

#include <pthread.h>

#define Q_DEFAULT_SIZE  50
struct Queue_t{
        void *elem[Q_DEFAULT_SIZE];
        unsigned int front;
        unsigned int rear;
        unsigned int count;
	pthread_mutex_t q_mutex;
};

struct Queue_t* initQ(void);

int 
is_queue_empty(struct Queue_t *q);

int 
is_queue_full(struct Queue_t *q);

int 
enqueue(struct Queue_t *q, void *ptr);

void*
deque(struct Queue_t *q);

void
print_Queue(struct Queue_t *q);

#define Q_COUNT(q)	(q->count)
#endif
