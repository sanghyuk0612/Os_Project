#include "types.h"
#include "stat.h"
#include "user.h"
#include "syscall.h"
#define FREE        0x0
#define RUNNING     0x1
#define RUNNABLE    0x2
#define WAIT        0x3
#define EXITED      0x4

#define STACK_SIZE  8192
#define MAX_THREAD  10

typedef struct thread thread_t, *thread_p;
typedef struct mutex mutex_t, *mutex_p;

struct thread {
  int        *sp;     /* saved stack pointer */
  int        tid;    /* thread id */
  int        ptid;   /* parent thread id */
  char stack[STACK_SIZE];
  int        state;
  void     (*func)(void);  // 실행할 함수 포인터
};

struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

static thread_t all_thread[MAX_THREAD];
thread_p  current_thread;
thread_p  next_thread;
extern void thread_switch(void);

static void 
thread_schedule(void) {
  //thread_p t;
  next_thread = 0;
  //기존방식
  /*
  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == RUNNABLE && t != current_thread) {
      next_thread = t;
      break;
    }
  }
  if (t >= all_thread + MAX_THREAD && current_thread->state == RUNNABLE) {
  next_thread = current_thread;
  }
  */ //여기까지
  //라운드 로빈 방식(돌아가면서 수행)
  int start = current_thread - all_thread;
  for (int i = 1; i < MAX_THREAD; i++) {
    int idx = (start + i) % MAX_THREAD;
    if (all_thread[idx].state == RUNNABLE) {
      next_thread = &all_thread[idx];
      break;
    }
  }
  if (!next_thread && current_thread->state == RUNNABLE) {
    next_thread = current_thread;
  }
  //여기까지가 라운드로빈 

  if (next_thread == 0) {
    printf(2, "thread_schedule: no runnable threads\n");
    exit();
  }

  if (current_thread != next_thread) {
    next_thread->state = RUNNING;
    current_thread->state = RUNNABLE;
    printf(1, ">> switching from tid=%d to tid=%d\n", current_thread->tid, next_thread->tid);
    thread_switch();
  } else {
    next_thread = 0;
  }
}

void 
thread_init(void) {
  uthread_init((int)thread_schedule);

  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
  current_thread->tid = 0;
  current_thread->ptid = 0;
}

void thread_exit() {
  current_thread->state = EXITED;

  // 디버깅: 각 스레드 상태 출력
  //for (int i = 0; i < MAX_THREAD; i++) {
  //  printf(1, "thread[%d] state = %d\n", i, all_thread[i].state);
  //}

  int any_alive = 0;
  for (int i = 0; i < MAX_THREAD; i++) {
    if (&all_thread[i] == current_thread) continue;
    if (all_thread[i].state == RUNNABLE || all_thread[i].state == RUNNING /* || all_thread[i].state == EXITED*/) {  
      any_alive = 1;
      break;
    }
  }

  if (!any_alive) {
    printf(1, "[thread_exit] 모든 스레드 종료됨 → 프로세스 종료\n");
    exit();  // 🔥 커널까지 종료 알림
  }

  thread_schedule();
  printf(1, "[thread_exit] fallback exit()\n");
  exit();
}
void thread_entry(void) {
  if (!current_thread || !current_thread->func) {
    printf(1, "[thread_entry] current_thread or func is NULL\n");
    thread_exit();
  }
  //printf(1,"thread_entry ex");

  current_thread->func();  // 등록된 함수 실행
  thread_exit();           // 끝나면 정리
}
typedef unsigned int uintptr_t;
int thread_create(void (*func)(void)) {
  thread_p t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE)
      break;
  }

  if (t >= all_thread + MAX_THREAD) {
    printf(1, "thread_create: no more threads\n");
    return -1;
  }

  t->tid = t - all_thread;
  t->ptid = current_thread->tid;
  t->state =RUNNABLE;
  t->func = func;

  // 스택 포인터 설정 (uint*로 가정)
  uint *sp = (uint*)(t->stack + STACK_SIZE);

  // 인자로 func 전달
  *(--sp) = (uint)func;               // argument
  *(--sp) = 0;                        // fake return address (never returns)
  *(--sp) = (uint)thread_entry;        // return address: thread_entry
  
  // 스택 포인터 등록
  t->sp = (int*)sp;
  printf(1, "Created thread at %p, func=%p, sp=%p\n", t, func, sp);
  return t->tid;
}

static void 
thread_join(int tid) {
  thread_p t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->tid == tid)
      break;
  }

  if (t >= all_thread + MAX_THREAD || t->state == FREE) {
    printf(1, "thread_join: no such thread %d\n", tid);
    return;
  }

  if (t->ptid != current_thread->tid) {
    printf(1, "thread_join: thread %d is not a child of current thread %d\n", tid, current_thread->tid);
    return;
  }

  while (t->state != EXITED) {
    //printf(1, "waiting for thread[%d] state=%d\n", t->tid, t->state);
    thread_schedule();
  }

  t->state = FREE;
}

void 
child_thread(void) {
  printf(1, "child_thread addr = 0x%x\n", (int)child_thread);
  if (!current_thread) {
    printf(1, "current_thread is NULL!\n");
    exit();
  }
  printf(1, "[child_thread] 실행됨! tid=%d\n", current_thread->tid);
  for (int i = 0; i < 5; i++) {
    printf(1, "child thread 0x%x\n", (int) current_thread);
  }
  printf(1, "child thread: exit\n");
  thread_exit();
}

void 
mythread(void) {
  int tid[5];
  printf(1, "my thread running\n");

  for (int i = 0; i < 5; i++) {
    tid[i] = thread_create(child_thread);
  }
  for (int i = 0; i < 5; i++) {
    thread_join(tid[i]);
  }
  printf(1, "my thread: exit\n");
  thread_exit();
}

int 
main(int argc, char *argv[]) {
  thread_init();
  printf(1, "mythread addr = 0x%x\n", (uint)mythread);
  printf(1, "child_thread addr = 0x%x\n", (uint)child_thread);
  int tid = thread_create(mythread);
  thread_join(tid);
  thread_exit();
}