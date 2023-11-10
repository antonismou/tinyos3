
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_cc.h"

/** 
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{
  PCB* curPCB = CURPROC;
  TCB* tcb = spawn_thread(curPCB,);
  //prepei na baloume edo    ^ ena func san to start_main_thread
  acquire_PTCB(tcb,task,argl,args);

  wakeup(tcb);

  return (Tid_t)tcb->ptcb;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) cur_thread();
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
	return -1;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
  PTCB* ptcb= (PTCB*) tid;//nomizo pos etsi briskoume to tid
  /*We check if this ptcb is on the list of this pcb 
  if isn't we return -1
  also we check if this thread is exited and also we return -1
  */
  if(!(rlist_find(&(CURPROC->ptcb_list),ptcb,NULL)) ||
      !(ptcb->exited)){
    return -1;
  }
  /*else we change the flag detached to true and
  broadcast all the threads that waitting
  */
  ptcb->detached = 1;
  CondVar* toBroadcast = &(ptcb->exit_cv);
  kernel_broadcast(toBroadcast);
	return 1;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{

}

