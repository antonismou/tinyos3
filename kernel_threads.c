
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

  return (Tid_t) (tcb->ptcb);
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) cur_thread()->ptcb;
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
  PTCB* ptcb = (PTCB*) tid;
  PCB* curproc = CURPROC;

  /* If there is no thread with the given tid in this process we exit with return value -1 */
  if(rlist_find(&curproc->ptcb_list, ptcb, NULL) == NULL){ // xreiazetai to & ? NAI
    return -1;
  }

  /* If the tid corresponds to the current thread we exit with return value -1 */
  if(cur_thread()->ptcb == ptcb){
    return -1;
  }

  /* If the tid corresponds to a detached or an exited thread we exit with return value -1 */
  if(ptcb->detached == 1){ // xreiazetai? NAI
    return -1;  
  }

  /* Increase refcount to know how many TCBs wait for this PTCB to complete
     in order to continue working */
  ptcb->refCount++;

  /* Put the current (calling) thread to sleep state until this PTCB exits or detaches */
  while(ptcb->exited == 0 && ptcb->detached == 0){ // yparxei periptwsi na ginei detached? NAI
    kernel_wait(&(ptcb->exit_cv), SCHED_USER);
  }  

  // xreiazontai ta broadcasts i ta kanoun oi exit/detach?

  /* decrease refcount */
  ptcb->refCount--;

  /* Check if the PTCB beacame detached while the current thread was sleeping. 
     In this case we wake up the sleeping thread and exit with return value -1 */
  if (ptcb->detached == 1){
    //kernel_broadcast(&ptcb->exit_cv);
    return -1;
  }

  // elegxos null?

  /* Save PTCB's exit status in *exitval */\
  if(exitval != NULL){
    *exitval = ptcb->exitVal;
  } 
 
  // 0 i 1
  
  /* If everything is successfull we free up the memory used for the joined thread (PTCB) */ 
  if(ptcb->refCount == 1){ // When the refcount is 0 we must remove the ptcb
    rlist_remove( &(ptcb->ptcb_node) ); 
    free(ptcb);  
  }

	return 0;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
  PTCB* ptcb= (PTCB*) tid;
  PCB* curproc = CURPROC;
  /*We check if this ptcb is on the list of this pcb 
  if isn't we return -1
  also we check if this thread is exited and also we return -1
  */
  if((rlist_find(&(curproc->ptcb_list),ptcb,NULL)) == NULL ||
      (ptcb->exited) == 1 ){
    return -1;
  }

  /*else we change the flag detached to true and
  broadcast all the threads that waitting
  */
  ptcb->detached = 1;
  CondVar* toBroadcast = &(ptcb->exit_cv);
  kernel_broadcast(toBroadcast);
	return 0;
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
  if(proc->thread_count==0){
    if(get_pid(proc) != 1){
      PCB* initpcb = get_pcb(1);
      while(!is_rlist_empty(& proc->children_list)) {
        rlnode* child = rlist_pop_front(& proc->children_list);
        child->pcb->parent = initpcb;
        rlist_push_front(& initpcb->children_list, child);
      }

      /* Add exited children to the initial task's exited list 
        and signal the initial task */
      if(!is_rlist_empty(& proc->exited_list)) {
        rlist_append(& initpcb->exited_list, &proc->exited_list);
        kernel_broadcast(& initpcb->child_exit);
      }

      /* Put me into my parent's exited list */
      rlist_push_front(& proc->parent->exited_list, &proc->exited_node);
      kernel_broadcast(& proc->parent->child_exit);
    }
  

  assert(is_rlist_empty(& proc->children_list));
  assert(is_rlist_empty(& proc->exited_list));


  /* 
    Do all the other cleanup we want here, close files etc. 
   */
    /*Clean up all PTCB in the ptcb_list
    we check for the ptcb_list for nodes if is empty we cnotninue
    else we pop clean up and we check again
    */
    while(is_rlist_empty(&proc->ptcb_list) != 0){
      rlnode* ptcb_node;
      ptcb_node = rlist_pop_front(&proc->ptcb_list);
      free(ptcb_node->ptcb);
    }
    /* Release the args data */
    if(proc->args) {
      free(proc->args);
      proc->args = NULL;
    }

    /* Clean up FIDT */
    for(int i=0;i<MAX_FILEID;i++) {
      if(proc->FIDT[i] != NULL) {
        FCB_decref(proc->FIDT[i]);
        proc->FIDT[i] = NULL;
      }
    }

    /* Disconnect my main_thread */
    proc->main_thread = NULL;

    /* Now, mark the process as exited. */
    proc->pstate = ZOMBIE;
  }

  /* Bye-bye cruel world */
  kernel_sleep(EXITED, SCHED_USER);

}

