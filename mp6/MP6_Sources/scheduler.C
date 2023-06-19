/*
 File: scheduler.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "simple_timer.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/
Queue * Queue::head = NULL;
Queue * Queue::tail = NULL;

Queue::Queue(Thread * _thread){
  curr_thread = _thread;
  next = NULL;
}

void Queue::enqueue(Queue * _queue){
  if(head == NULL){
    head = _queue;
    tail = _queue;
  }
  else{
    tail->next = _queue;
    tail = _queue;
  }
}

Thread * Queue::dequeue(){
  Thread * removed_thread = head->curr_thread;
  Queue * temp = head; 
  if(head == tail){
    head = NULL;
    tail = NULL;
  }
  else{
    head = head->next;
  }
  delete temp;
  return removed_thread;
}

Scheduler::Scheduler() {
  
}

void Scheduler::yield() {
  if(queue->head != NULL){
    // if(Machine::interrupts_enabled()){
    //   Machine::disable_interrupts();
    // }
    Thread * next_thread = Queue::dequeue();
    Thread::dispatch_to(next_thread);
    // if(!Machine::interrupts_enabled()){
    //   Machine::enable_interrupts();
    // }
  }
}

void Scheduler::resume(Thread * _thread) {
  // if(Machine::interrupts_enabled()){
  //     Machine::disable_interrupts();
  //   }
  Queue * new_queue = new Queue(_thread);
  Queue::enqueue(new_queue);
  // if(!Machine::interrupts_enabled()){
  //     Machine::enable_interrupts();
  //   }
}

void Scheduler::add(Thread * _thread) {
  // if(Machine::interrupts_enabled()){
  //     Machine::disable_interrupts();
  //   }
  resume(_thread);
  // if(!Machine::interrupts_enabled()){
  //     Machine::enable_interrupts();
  //   }
}

void Scheduler::terminate(Thread * _thread) {
  // if(Machine::interrupts_enabled()){
  //     Machine::disable_interrupts();
  //   }
  Thread * current_thread = _thread;
  current_thread->delete_stack();
  yield();
  // if(!Machine::interrupts_enabled()){
  //     Machine::enable_interrupts();
  //   }
}


RRScheduler::RRScheduler(EOQTimer * _eoq_timer){
  eoq_timer = _eoq_timer;
}

void RRScheduler::yield(){
  if(queue->head != NULL){
    if(Machine::interrupts_enabled()){
      Machine::disable_interrupts();
    }
    eoq_timer->reset_tick();
    Thread * next_thread = Queue::dequeue();
    Thread::dispatch_to(next_thread);
    if(!Machine::interrupts_enabled()){
      Machine::enable_interrupts();
    }
  }
}