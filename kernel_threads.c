
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
  TCB* tcb = spawn_thread(curPCB,start_ptcb_main_thread);
  acquire_PTCB(tcb,task,argl,args);
  curPCB->thread_count++;

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
  PTCB* ptcb = cur_thread()->ptcb;

  /*
    we change exit value of current ptcb to 1 to show that the thread is exited
  */
  ptcb->exitVal = exitval;
  ptcb->exited=1;

  kernel_broadcast(&(ptcb->exit_cv));
  
  /*
    decrease the number of threads to the current process
  */
  PCB* proc = CURPROC;
  proc->thread_count--;
  

    /* Reparent any children of the exiting process to the 
       initial task */
  if(CURPROC->thread_count==0){
    PCB* initpcb = get_pcb(1);
    while(!is_rlist_empty(& CURPROC->children_list)) {
      rlnode* child = rlist_pop_front(& CURPROC->children_list);
      child->pcb->parent = initpcb;
      rlist_push_front(& initpcb->children_list, child);
    }

    /* Add exited children to the initial task's exited list 
       and signal the initial task */
    if(!is_rlist_empty(& CURPROC->exited_list)) {
      rlist_append(& initpcb->exited_list, &CURPROC->exited_list);
      kernel_broadcast(& initpcb->child_exit);
    }

    /* Put me into my parent's exited list */
    rlist_push_front(& CURPROC->parent->exited_list, &CURPROC->exited_node);
    kernel_broadcast(& CURPROC->parent->child_exit);

  

  assert(is_rlist_empty(& CURPROC->children_list));
  assert(is_rlist_empty(& CURPROC->exited_list));


  /* 
    Do all the other cleanup we want here, close files etc. 
   */

  /* Release the args data */
  if(CURPROC->args) {
    free(CURPROC->args);
    CURPROC->args = NULL;
  }

  /* Clean up FIDT */
  for(int i=0;i<MAX_FILEID;i++) {
    if(CURPROC->FIDT[i] != NULL) {
      FCB_decref(CURPROC->FIDT[i]);
      CURPROC->FIDT[i] = NULL;
    }
  }

  /* Disconnect my main_thread */
  CURPROC->main_thread = NULL;

  /* Now, mark the process as exited. */
  CURPROC->pstate = ZOMBIE;
  }

  /* Bye-bye cruel world */
  kernel_sleep(EXITED, SCHED_USER);

}

