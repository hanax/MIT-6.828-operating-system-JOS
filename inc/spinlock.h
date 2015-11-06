#ifndef JOS_INC_THREAD_H
#define JOS_INC_THREAD_H

#include <inc/types.h>

// Mutual exclusion lock.
typedef struct _spinlock {
  uint32_t locked;       // Is the lock held?
} pthread_mutex_t;

typedef int pthread_mutexattr_t;


#endif /* !JOS_INC_THREAD_H */