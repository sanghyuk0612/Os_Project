#include "types.h"
#include "stat.h"
#include "user.h"

/* Possible states of a thread; */
#define FREE        0x0
#define RUNNING     0x1
#define RUNNABLE    0x2

#define STACK_SIZE  8192
#define MAX_THREAD  4

typedef struct thread thread_t, *thread_p;
typedef struct mutex mutex_t, *mutex_p;

struct thread {
  int        *sp;                /* saved stack pointer */
  char stack[STACK_SIZE];       /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
  void (*function)();
};
static thread_t all_thread[MAX_THREAD];
thread_p  current_thread;
thread_p  next_thread;
extern void thread_switch(void);
extern void thread_schedule(void); // 다시 리턴할 주소
/*
static void 
thread_schedule(void)
{
  thread_p t;

  // Find another runnable thread.
  next_thread = 0;
  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == RUNNABLE && t != current_thread) {
      next_thread = t;
      break;
    }
  }

  if (t >= all_thread + MAX_THREAD && current_thread->state == RUNNABLE) {
    // The current thread is the only runnable thread; run it. 
    next_thread = current_thread;
  }

  if (next_thread == 0) {
    printf(2, "thread_schedule: no runnable threads\n");
    exit();
  }

  if (current_thread != next_thread) {        //
    next_thread->state = RUNNING;
    current_thread->state = RUNNABLE;
    thread_switch();
  } else
    next_thread = 0;
}
*/
void thread_schedule(void){ // 다시 리턴할 주소ihread_schedule(void) {
  thread_p t;
  next_thread = 0;

  // 현재 스레드 제외하고 RUNNABLE인 거 찾기
  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == RUNNABLE && t != current_thread) {
      next_thread = t;
      break;
    }
  }

  // 못 찾으면 현재 스레드가 아직 RUNNABLE인지 확인
  if (!next_thread && current_thread->state == RUNNABLE) {
    next_thread = current_thread;
  }

  // 그래도 없으면 포기
  if (!next_thread) {
    printf(2, "thread_schedule: no runnable threads\n");
    exit();
  }

  printf(1, "switching from 0x%x to 0x%x\n", (int)current_thread, (int)next_thread);

  if (current_thread != next_thread) {
    next_thread->state = RUNNING;
    if (current_thread->state != FREE) {
      current_thread->state = RUNNABLE;
    }
    thread_switch();
  } else {
    next_thread = 0;
  }
}

void 
thread_init(void)
{
  //uthread_init((int)thread_schedule); 

  // main() is thread 0, which will make the first invocation to
  // thread_schedule().  it needs a stack so that the first thread_switch() can
  // save thread 0's state.  thread_schedule() won't run the main thread ever
  // again, because its state is set to RUNNING, and thread_schedule() selects
  // a RUNNABLE thread.
  int dummy;
  all_thread[0].sp = (void*)&dummy;
  all_thread[0].state = RUNNING;

  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
  next_thread=0;
}

/*
void 
thread_create(void (*func)())
{
  thread_p t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }
  t->sp = (int) (t->stack + STACK_SIZE);   // set sp to the top of the stack
  t->sp -= 4;                              // space for return address
  * (int *) (t->sp) = (int)func;           // push return address on stack
  t->sp -= 32;                             // space for registers that thread_switch expects
  t->state = RUNNABLE;
}
*/

//원래있던 thread_create함수
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};
//내가 정의한 구조체
void thread_exit() {
  printf(1, "thread_exit: thread %p exiting\n", current_thread);
  current_thread->state = FREE;
  thread_schedule();  // 다음 스레드로 전환
  printf(1, "thread_exit: thread 0x%x exiting\n", (int)current_thread);
}
//내가 정의한 thread_exit함수
//아래는 내가 정의한 create 함수
int
thread_create(void (*func)())
{
  thread_p t;
  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE)
      break;
  }
  if (t == all_thread + MAX_THREAD) {
    printf(2, "No more threads\n");
    return -1;
  }

  t->function = func;
  t->state = RUNNABLE;

  // 스택 포인터 설정 (uint*로 가정)
  uint *sp = (uint*)(t->stack + STACK_SIZE);

  *(--sp) =0;
  // function 주소를 스택에 푸시
  *(--sp) = (uint)func;     // ret으로 점프할 함수
  
  // 스택 포인터 등록
  t->sp = (int*)sp;

  printf(1, "Created thread at %p, func=%p, sp=%p\n", t, func, sp);
  return 0;
}
static void 
mythread(void)
{
  int i;
  printf(1, "my thread running\n");
  for (i = 0; i < 100; i++) {
    printf(1, "my thread 0x%x\n", (int) current_thread);
  }
  printf(1, "my thread: exit\n");
  current_thread->state = FREE;
  thread_schedule();
}


int 
main(int argc, char *argv[]) 
{
  thread_init();
  thread_create(mythread);
  thread_create(mythread);
  thread_schedule();
  return 0;
}
