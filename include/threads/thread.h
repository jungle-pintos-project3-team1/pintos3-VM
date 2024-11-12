#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/interrupt.h"
#ifdef VM
#include "vm/vm.h"
#endif


/* States in a thread's life cycle. */
enum thread_status {
	THREAD_RUNNING,     /* Running thread. */
	THREAD_READY,       /* Not running but ready to run. */
	THREAD_BLOCKED,     /* Waiting for an event to trigger. */
	THREAD_DYING        /* About to be destroyed. */
};

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

//==================================================================
//				Project 1 - mlfqs
//------------------------------------------------------------------
#define NICE_DEFAULT 0
#define RECENT_CPU_DEFAULT 0
#define LOAD_AVG_DEFAULT 0
//==================================================================
/* A kernel thread or user process.
 *
 * Each thread structure is stored in its own 4 kB page.  The
 * thread structure itself sits at the very bottom of the page
 * (at offset 0).  The rest of the page is reserved for the
 * thread's kernel stack, which grows downward from the top of
 * the page (at offset 4 kB).  Here's an illustration:
 *
 *      4 kB +---------------------------------+
 *           |          kernel stack           |
 *           |                |                |
 *           |                |                |
 *           |                V                |
 *           |         grows downward          |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           |                                 |
 *           +---------------------------------+
 *           |              magic              |
 *           |            intr_frame           |
 *           |                :                |
 *           |                :                |
 *           |               name              |
 *           |              status             |
 *      0 kB +---------------------------------+
 *
 * The upshot of this is twofold:
 *
 *    1. First, `struct thread' must not be allowed to grow too
 *       big.  If it does, then there will not be enough room for
 *       the kernel stack.  Our base `struct thread' is only a
 *       few bytes in size.  It probably should stay well under 1
 *       kB.
 *
 *    2. Second, kernel stacks must not be allowed to grow too
 *       large.  If a stack overflows, it will corrupt the thread
 *       state.  Thus, kernel functions should not allocate large
 *       structures or arrays as non-static local variables.  Use
 *       dynamic allocation with malloc() or palloc_get_page()
 *       instead.
 *
 * The first symptom of either of these problems will probably be
 * an assertion failure in thread_current(), which checks that
 * the `magic' member of the running thread's `struct thread' is
 * set to THREAD_MAGIC.  Stack overflow will normally change this
 * value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
 * the run queue (thread.c), or it can be an element in a
 * semaphore wait list (synch.c).  It can be used these two ways
 * only because they are mutually exclusive: only a thread in the
 * ready state is on the run queue, whereas only a thread in the
 * blocked state is on a semaphore wait list. */
struct thread {
	/* Owned by thread.c. */
	tid_t tid;                          /* Thread identifier. */
	enum thread_status status;          /* Thread state. */
	char name[16];                      /* Name (for debugging purposes). */
	int priority;                       /* Priority. */

	//==================================================================
	//				Project 1 - Alarm Clock
	//------------------------------------------------------------------
	int64_t				wakeup_ticks;
	//==================================================================

	//==================================================================
	//				Project 1 - Priority Donation
	//------------------------------------------------------------------
	int 				original_priority;	/* 기부 받은 후에 다시 원래의 우선순위로 돌아오기 위한 저장용 변수*/
	struct lock* 		wait_on_lock;		/* 스레드가 현재 얻기 위해 기다리고 있는 lock, 스레드는 이 lock이 release되기를 기다린다.*/
	struct list 		donations;			/* 스레드가 점유하고 있는 lock을 요청하면서 priority를 기부해준 스레드들을 연결 리스트 형태로 저장한다. */

	/*	다른 스레드가 점유하고 있는 lock을 요청 했을 때 , 다른 스레드에게 priority를 기부하면서 해당 스레드의 donations에 들어갈 때 사용되는 elem
		기존에 있던 elem은 ready_list와 sleep_list에서 사용중이므로 donations에서는 사용될 수 없다. 
		elem은 한 리스트에서만 사용되어야 한다. ready_list와 sleep_list는 구조상 두 리스트에 공존 할 수 없기 때문에 
		기존 코드에서는 하나의 elem만으로도 구현이 가능했다. 
		lock과 마찬가지로 lock요청은 한 스레드당 하나만 가능하므로 이를 위한 elem도 하나만 있으면 된다.*/
	struct list_elem	donation_elem;		
	//==================================================================


	//==================================================================
	//				Project 1 - mlfqs
	//------------------------------------------------------------------
	int 				nice;
	int 				recent_cpu;
	struct list_elem	allelem;
	//==================================================================

	/* Shared between thread.c and synch.c. */
	struct list_elem elem;              /* List element. */

#ifdef USERPROG
	/* Owned by userprog/process.c. */
	uint64_t *pml4;                     /* Page map level 4 */
#endif
#ifdef VM
	/* Table for whole virtual memory owned by thread. */
	struct supplemental_page_table spt;
#endif

	/* Owned by thread.c. */
	struct intr_frame tf;               /* Information for switching */
	unsigned magic;                     /* Detects stack overflow. */
};

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

void do_iret (struct intr_frame *tf);

//==================================================================
//				Project 1 - Alarm Clock
//------------------------------------------------------------------

/* 잠들 스레드를 sleep_list에 삽입해준다.
	이 때 ticks가 작은 스레드가 앞부분에 위치하도록 정렬하여 삽입한다. */
void ThreadSleep(int64_t ticks);

// ticks를 기준으로 정렬할 때 사용하는 함수
bool CompareThreadByTicks(const struct list_elem* l, const struct list_elem* r, void *aux UNUSED);

// sleep_list를 확인해서 꺠어날 스레드들을 ready_list로 옮겨주는 함수 
void ThreadWakeUp(int64_t current_ticks);

//==================================================================


//==================================================================
//				Project 1 - Priority Scheduling
//------------------------------------------------------------------

// 우선순위를 기준으로 정렬할 때 사용하는 함수
bool CompareThreadByPriority(const struct list_elem* l, const struct list_elem* r, void *aux UNUSED);

// yield를 할 때 우선순위를 기준으로 양보하는 함수 
void ThreadYieldByPriority();

//==================================================================

//==================================================================
//				Project 1 - Priority Donatnion
//------------------------------------------------------------------

// 현재 스레드가 원하는 lock을 가진 holder에게 현재 스레드의 priority를 기부하는 함수
// 이 때 holder도 또다시 다른 lock의 release를 기다리고 있는지를 확인하여 연쇄적으로 
// 모든 holder들의 priority를 모두 바꿔주어야 한다. 
void DonatePriority();
void ThreadUpdatePriorityFromDonations();
//==================================================================


//==================================================================
//				Project 1 - mlfqs
//------------------------------------------------------------------
void mlfqsCalculatePriority (struct thread *th);
void mlfqsCalculateRecentCPU (struct thread *th);
void mlfqsCalculateLoadAvg (void);
void mlfqsIncrementRecentCPU (void);
void mlfqsRecalculateRecentCPU (void);
void mlfqsRecalculatePrioirty (void);

//==================================================================

#endif /* threads/thread.h */
