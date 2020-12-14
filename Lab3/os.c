// os.c
// Runs on LM4F120/TM4C123/MSP432
// Lab 3 starter file.
// Daniel Valvano
// March 24, 2016


#include "os.h"
#include "CortexM.h"
#include "BSP.h"

// function definitions in osasm.s
void StartOS(void);
void runperiodicevents(void);


#define NUMTHREADS  6        // maximum number of threads
#define NUMPERIODIC 2        // maximum number of periodic threads
#define STACKSIZE   100      // number of 32-bit words in stack per thread
struct tcb{
  int32_t *sp;       // pointer to stack (valid for threads not running
  struct tcb *next;  // linked-list pointer
  int32_t *blocked;  // nonzero if blocked on this semaphore
  uint32_t sleep;    // nonzero if this thread is sleeping
};
typedef struct tcb tcbType;
tcbType tcbs[NUMTHREADS];
tcbType *RunPt;
int32_t Stacks[NUMTHREADS][STACKSIZE];

// ******** OS_Init ************
// Initialize operating system, disable interrupts
// Initialize OS controlled I/O: periodic interrupt, bus clock as fast as possible
// Initialize OS global variables
// Inputs:  none
// Outputs: none
void OS_Init(void){
  DisableInterrupts();
  BSP_Clock_InitFastest();// set processor clock to fastest speed
  // perform any initializations needed
  BSP_PeriodicTask_Init(runperiodicevents, 1000, 2); // init hw timer to run periodic events
}

void SetInitialStack(int i){
  tcbs[i].sp = &Stacks[i][STACKSIZE-16]; // thread stack pointer
  Stacks[i][STACKSIZE-1] = 0x01000000; // Thumb bit
  Stacks[i][STACKSIZE-3] = 0x14141414; // R14
  Stacks[i][STACKSIZE-4] = 0x12121212; // R12
  Stacks[i][STACKSIZE-5] = 0x03030303; // R3
  Stacks[i][STACKSIZE-6] = 0x02020202; // R2
  Stacks[i][STACKSIZE-7] = 0x01010101; // R1
  Stacks[i][STACKSIZE-8] = 0x00000000; // R0
  Stacks[i][STACKSIZE-9] = 0x11111111; // R11
  Stacks[i][STACKSIZE-10] = 0x10101010; // R10
  Stacks[i][STACKSIZE-11] = 0x09090909; // R9
  Stacks[i][STACKSIZE-12] = 0x08080808; // R8
  Stacks[i][STACKSIZE-13] = 0x07070707; // R7
  Stacks[i][STACKSIZE-14] = 0x06060606; // R6
  Stacks[i][STACKSIZE-15] = 0x05050505; // R5
  Stacks[i][STACKSIZE-16] = 0x04040404; // R4
}

//******** OS_AddThreads ***************
// Add six main threads to the scheduler
// Inputs: function pointers to six void/void main threads
// Outputs: 1 if successful, 0 if this thread can not be added
// This function will only be called once, after OS_Init and before OS_Launch
int OS_AddThreads(void(*thread0)(void),
                  void(*thread1)(void),
                  void(*thread2)(void),
                  void(*thread3)(void),
                  void(*thread4)(void),
                  void(*thread5)(void)){
// initialize TCB circular list
// initialize RunPt
// initialize six stacks, including initial PC
	int32_t status;
  status = StartCritical();
  tcbs[0].next = &tcbs[1]; // 0 points to 1
  tcbs[1].next = &tcbs[2]; // 1 points to 2
  tcbs[2].next = &tcbs[3]; // 2 points to 3
  tcbs[3].next = &tcbs[4]; // 3 points to 4
	tcbs[4].next = &tcbs[5]; // 4 points to 5
	tcbs[5].next = &tcbs[0]; // 5 points to 0
  
// initialize blocked fields to 0
// zero values means not block
  tcbs[0].blocked = 0;
  tcbs[1].blocked = 0;                 
  tcbs[2].blocked = 0;
  tcbs[3].blocked = 0;
  tcbs[4].blocked = 0;
  tcbs[5].blocked = 0;
                    
// initialize sleep fields to 0
// zero values means not sleeping
  tcbs[0].sleep = 0;
  tcbs[1].sleep = 0;
  tcbs[2].sleep = 0;
  tcbs[3].sleep = 0;
  tcbs[4].sleep = 0;
  tcbs[5].sleep = 0;
                    
  SetInitialStack(0); Stacks[0][STACKSIZE-2] = (int32_t)(thread0); // PC
  SetInitialStack(1); Stacks[1][STACKSIZE-2] = (int32_t)(thread1); // PC
  SetInitialStack(2); Stacks[2][STACKSIZE-2] = (int32_t)(thread2); // PC
	SetInitialStack(3); Stacks[3][STACKSIZE-2] = (int32_t)(thread3); // PC
	SetInitialStack(4); Stacks[4][STACKSIZE-2] = (int32_t)(thread4); // PC
  SetInitialStack(5); Stacks[5][STACKSIZE-2] = (int32_t)(thread5); // PC
                   
  RunPt = &tcbs[0];        // thread 0 will run first
  EndCritical(status);
  return 1;	// successful
}

#define NUMPERIODICTHREADS 2
uint32_t periodic_thread_index = 0;
struct event_tcb{
  void (*task)(void);
  uint32_t period;
};

struct event_tcb periodic_thread[NUMPERIODICTHREADS];
uint32_t max_period = 0;
//******** OS_AddPeriodicEventThread ***************
// Add one background periodic event thread
// Typically this function receives the highest priority
// Inputs: pointer to a void/void event thread function
//         period given in units of OS_Launch (Lab 3 this will be msec)
// Outputs: 1 if successful, 0 if this thread cannot be added
// It is assumed that the event threads will run to completion and return
// It is assumed the time to run these event threads is short compared to 1 msec
// These threads cannot spin, block, loop, sleep, or kill
// These threads can call OS_Signal
// In Lab 3 this will be called exactly twice
int OS_AddPeriodicEventThread(void(*thread)(void), uint32_t period){
// ****IMPLEMENT THIS****
  int32_t status;
  status = StartCritical();
  
  if (period > max_period){
    max_period = period;
  }
  periodic_thread[periodic_thread_index].task = thread;
  periodic_thread[periodic_thread_index].period = period;
  periodic_thread_index++;
  
  EndCritical(status);  
  return 1;

}
void static runperiodicevents(void){
// ****IMPLEMENT THIS****
// **RUN PERIODIC THREADS, DECREMENT SLEEP COUNTERS
  uint32_t static counter = 0;
  counter = (counter + 1) % max_period;
  int i;
  //Decrement sleep counters
  for (i = 0; i < NUMTHREADS; i++) 
  {
    if (tcbs[i].sleep) 
    {
      tcbs[i].sleep--;
    }
  }
  
  //Run periodic threads
  for (i = 0; i< NUMPERIODICTHREADS; i++)
  {
    if ((counter % periodic_thread[i].period) == 0){
      periodic_thread[i].task(); // run this periodic task
    }
  }
}

//******** OS_Launch ***************
// Start the scheduler, enable interrupts
// Inputs: number of clock cycles for each time slice
// Outputs: none (does not return)
// Errors: theTimeSlice must be less than 16,777,216
void OS_Launch(uint32_t theTimeSlice){
  STCTRL = 0;                  // disable SysTick during setup
  STCURRENT = 0;               // any write to current clears it
  SYSPRI3 =(SYSPRI3&0x00FFFFFF)|0xE0000000; // priority 7
  STRELOAD = theTimeSlice - 1; // reload value
  STCTRL = 0x00000007;         // enable, core clock and interrupt arm
  StartOS();                   // start on the first task
}
// runs every ms
void Scheduler(void){ // every time slice
// ROUND ROBIN, skip blocked and sleeping threads
  RunPt = RunPt->next; // skip at least one
  while((RunPt->sleep)||(RunPt-> blocked))
  {
    RunPt = RunPt->next; // find one not sleeping and not blocked
  }
}

//******** OS_Suspend ***************
// Called by main thread to cooperatively suspend operation
// Inputs: none
// Outputs: none
// Will be run again depending on sleep/block status
void OS_Suspend(void){
  STCURRENT = 0;        // any write to current clears it
  INTCTRL = 0x04000000; // trigger SysTick
// next thread gets a full time slice
}

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(uint32_t sleepTime){
// set sleep parameter in TCB
// suspend, stops running
  RunPt->sleep = sleepTime;
  OS_Suspend();
}

// ******** OS_InitSemaphore ************
// Initialize counting semaphore
// Inputs:  pointer to a semaphore
//          initial value of semaphore
// Outputs: none
void OS_InitSemaphore(int32_t *semaPt, int32_t value){
  *semaPt = value;
}

// ******** OS_Wait ************
// Decrement semaphore and block if less than zero
// Lab2 spinlock (does not suspend while spinning)
// Lab3 block if less than zero
// Inputs:  pointer to a counting semaphore
// Outputs: none
void OS_Wait(int32_t *semaPt){
 DisableInterrupts();
 (*semaPt) = (*semaPt) - 1;
 if((*semaPt) < 0){
   RunPt->blocked = semaPt; // reason it is blocked
   EnableInterrupts();
   OS_Suspend();       // run thread switcher
 }
 EnableInterrupts();
}

// ******** OS_Signal ************
// Increment semaphore
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate
// Inputs:  pointer to a counting semaphore
// Outputs: none
void OS_Signal(int32_t *semaPt){
  tcbType *pt;
  DisableInterrupts();
  (*semaPt) = (*semaPt) + 1;
  if((*semaPt) <= 0){
    pt = RunPt->next;   // search for a thread blocked on this semaphore
    while(pt->blocked != semaPt){
      pt = pt->next;
    }
    pt->blocked = 0;    // wakeup this one
  }
  EnableInterrupts();
}

#define FIFOSIZE    10      // number of 32-bit words in fifo
uint32_t PutI;      // index of where to put next
uint32_t GetI;      // index of where to get next
uint32_t Fifo[FIFOSIZE];
int32_t CurrentSize;// 0 means FIFO empty, FSIZE means full
uint32_t LostData;  // number of lost pieces of data

// ******** OS_FIFO_Init ************
// Initialize FIFO.  
// One event thread producer, one main thread consumer
// Inputs:  none
// Outputs: none
void OS_FIFO_Init(void){
  PutI = GetI = 0;   // Empty
  OS_InitSemaphore(&CurrentSize, 0);
  LostData = 0;
}

// ******** OS_FIFO_Put ************
// Put an entry in the FIFO.  
// Exactly one event thread puts,
// do not block or spin if full
// Inputs:  data to be stored
// Outputs: 0 if successful, -1 if the FIFO is full
int OS_FIFO_Put(uint32_t data){
  if(CurrentSize == FIFOSIZE)
  {
      LostData++;
      return -1;         // full
  } else
  {
      Fifo[PutI] = data; // Put
      PutI = (PutI+1)%FIFOSIZE;
      OS_Signal(&CurrentSize);
      return 0;   // success
  }
}

// ******** OS_FIFO_Get ************
// Get an entry from the FIFO.   
// Exactly one main thread get,
// do block if empty
// Inputs:  none
// Outputs: data retrieved
uint32_t OS_FIFO_Get(void){uint32_t data;
  OS_Wait(&CurrentSize);    // block if empty
  data = Fifo[GetI];        // get
  GetI = (GetI+1)%FIFOSIZE; // place to get next
  return data;
}
