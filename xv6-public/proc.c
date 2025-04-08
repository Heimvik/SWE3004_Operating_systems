#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;


static const int weights[40] = {
	88818,     71054,     56843,     45475,     36380,
	29104,     23283,     18626,     14901,     11921,
	9537,     7629,     6104,     4883,     3906,
  3125,     2500,     2000,     1600,     1280,
  1024,     819,     655,     524,     419,
  336,     268,     215,     172,     137,
  110,     88,     70,     56,     45,
  36,     29,     23,     18,     15,
};

static long long unsigned int totalticks = 0; // Total milliticks since boot [mticks] 

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

int calcvruntime(int runtime,int nice){
	if(nice < 0 || nice > 39){
		panic("Invalid nice value");
	}
	//cprintf("Calculating vruntime for %d with nice %d, result: %d\n",runtime,nice,(runtime * weights[DEAFULT_NICE]) / weights[nice]);
	return (runtime * weights[DEAFULT_NICE]) / weights[nice];
}


void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock); 

  p->state = RUNNABLE;
  p->schedstate.nice = DEAFULT_NICE;
  p->schedstate.runtime = 0;
  p->schedstate.vruntime = 0;
  p->schedstate.timeslice = 0;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;
  np->schedstate.nice = curproc->schedstate.nice;
  np->schedstate.runtime = curproc->schedstate.runtime;
  np->schedstate.vruntime = curproc->schedstate.vruntime;
  np->schedstate.timeslice = curproc->schedstate.timeslice;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
		p->schedstate.nice = 20;
		p->schedstate.runtime = 0;
		p->schedstate.vruntime = 0;
		p->schedstate.timeslice = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
rrscheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  for(;;){
	  // Enable interrupts on this processor.
	  sti();
	  
	  // Loop over process table looking for process to run.
	  acquire(&ptable.lock);
	  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(p->state != RUNNABLE)
			continue;
			
		// Switch to chosen process.  It is the process's job
		// to release ptable.lock and then reacquire it
		// before jumping back to us.
		c->proc = p;
		switchuvm(p);  //Switche to the process's page table (each page table is spesific to a process)
		p->state = RUNNING;
		//printvariabletable(ptable.proc);
		//printgantline(ptable.proc);
		
		swtch(&(c->scheduler), p->context);
		totalticks+= MTICKS;

		switchkvm();

		// Process is done running for now.
		// It should have changed its p->state before coming back.
		c->proc = 0;
    }
    release(&ptable.lock);

  }
}

void
cfsscheduler(void)
{
	acquire(&ptable.lock);
	struct proc *p = ptable.proc; //Start with the fist one by default
	release(&ptable.lock);
	struct cpu *c = mycpu();
	c->proc = 0;
	
	for(;;){
		sti();
		acquire(&ptable.lock);
		
		//1. Find the one with the smallest vruntime from the RUNNABLE processes (this may preempt the current process, i.e. if another one wakes up with a smaller vruntime)
		int runnableprocfound = 0;
		int minvruntime = 0x7FFFFFFF; //Set to max value
		int weightsum = 0;
		for(struct proc* iterp = ptable.proc; iterp < &ptable.proc[NPROC]; iterp++){
			if(iterp->state == RUNNABLE){
				runnableprocfound = 1;

				if(iterp->schedstate.nice >= 0 && iterp->schedstate.nice <= 39){
					weightsum += weights[iterp->schedstate.nice];
				}
				
				if(iterp->schedstate.vruntime < minvruntime && iterp->schedstate.vruntime >= 0){
					minvruntime = iterp->schedstate.vruntime;
					p = iterp; //This is the process we want to run if it is still here in the end
				}
			}
		}
		if(!runnableprocfound){
			release(&ptable.lock);
			continue;
		}
		totalticks+= MTICKS;
		
		//2. Calculate its timeslice
		if(weightsum == 0){
			panic("zero div");
		}
		p->schedstate.timeslice = SCHED_LATENCY + weights[p->schedstate.nice]/weightsum;
		
		//3. Run it for this timeslice, unless preemted. Use actual runtime to compare
		c->proc = p;			//Assign the process to this CPU
		switchuvm(p);			//Switch from the schedulers page table to the process's page table
		p->state = RUNNING;	
		logtick(p, totalticks);

		swtch(&(c->scheduler), p->context);			//Exe appears in and out of this swtch by doing context switching (including stack and instruction pointers)

		switchkvm();			//Switch back to the scheduler's page table
		c->proc = 0;			//Unassign the process from this CPU
		release(&ptable.lock);
	}
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock)) 
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  struct proc *p = myproc();
  p->state = RUNNABLE;
  p->schedstate.runtime += MTICKS;
  p->schedstate.vruntime += calcvruntime(MTICKS,p->schedstate.nice);
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
	struct proc *p;
	int sleepingprocfound = 0;
	int runnableprocfound = 0;
	int minvruntime = 0x7FFFFFFF; //Set to max value

	for(struct proc* iterp = ptable.proc; iterp < &ptable.proc[NPROC]; iterp++){
		//TODO: If there is no process in the RUNNABLE state when a process wakes up, you can set the vruntime of the process to be woken up to “0”)
		if(iterp->state == SLEEPING && iterp->chan == chan){
			sleepingprocfound = 1;
			p = iterp; //This is the process we want to wake up
		}
		if(iterp->state == RUNNABLE){
			runnableprocfound = 1;
			if(iterp->schedstate.vruntime < minvruntime && iterp->schedstate.vruntime > 0){
				minvruntime = iterp->schedstate.vruntime;
			}
		}
  	}
	if(sleepingprocfound){
		p->state = RUNNABLE;
		if(runnableprocfound){
			p->schedstate.vruntime = minvruntime-calcvruntime(MTICKS,p->schedstate.nice);
		} else {
			p->schedstate.vruntime = 0;
			//How tf does this makes sence? Others can be sleeping, and this would go to 0, making the othes vruntime way larger than this on 
		}
	}
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
 
/*
Syscall setnice(int pid, int nice) shold have properties:
Input:
- The pid of the process to set the nice value of
- The nice value to set

Output:
- 0 on successfull set of nicevalue
- -1 on no process that has the pid value || not valid pid
*/
int setnice(int pid, int nice){
	if(nice<0 || nice > 39){
		return -1;
	}
	acquire(&ptable.lock);
	for(struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(pid == p->pid && pid != 0){
			p->schedstate.nice = nice;
			release(&ptable.lock);
			return 0; 
		}
	}
	release(&ptable.lock);
	return -1;
}

/*
Syscall getnice(int pid) shold have properties:
Input:
- The pid of the process we want to get the nice value of.

Output:
- nice value of target process on success
- -1 if unsuccessfull (no process with the pelected pid)
*/
int getnice(int pid){
	acquire(&ptable.lock);
	for(struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(pid == p->pid && pid != 0){
			int nice = p->schedstate.nice; //Neccessary as we cannot defer the release of mutex unil after return
			release(&ptable.lock);
			return nice;
		}
	}
	release(&ptable.lock);
	return -1;
}

/*
Helper functions for ps syscall
*/
void strprocstate(char* result,enum procstate state){
	switch(state){
		case(UNUSED):
			safestrcpy(result, "UNUSED", 10);
			break;
		case(EMBRYO):
			safestrcpy(result, "EMBRYO", 10);
			break;
		case(SLEEPING):
			safestrcpy(result, "SLEEPING", 10);
			break;
		case(RUNNABLE):
			safestrcpy(result, "RUNNABLE", 10);
			break;
		case(RUNNING):
			safestrcpy(result, "RUNNING", 10);
			break;
		case(ZOMBIE):
			safestrcpy(result, "ZOMBIE", 10);
			break;
	}
}

int strint(int src, char *dst) {
	if (dst == 0) {
		return -1;
	}
	char temp[32];
	int i = 0;
	int isneg = 0;
	if (src < 0) {
		isneg = 1;
		src = -src;
	}
	while (src > 0 || i == 0) {
	  temp[i++] = (src % 10) + '0'; // Converts a number to a string (NB: in reverse order)
	  src /= 10;
	}
	if (isneg) {
		temp[i++] = '-';
	}
	int j = 0;
	while (i > 0) {
		dst[j++] = temp[--i]; //Reverses the strign back into the given buffer, FIFO-like
	}
	dst[j] = '\0';
  
	return 1;
  }
  
  void cprintfpad(const char *str, int width) {
	  int len = strlen(str);
	  cprintf("%s", str);  // Print the string
	  for (int i = len; i < width; i++) {
		  cprintf(" ");  // Add spaces for padding
	  }
  }

void printheader(){
	cprintfpad("NAME",FIELDSIZE);
	cprintfpad("PID",FIELDSIZE);
	cprintfpad("STATE",FIELDSIZE);
	cprintfpad("NICE",FIELDSIZE);
	cprintfpad("RUNTIME/WEIGHT",FIELDSIZE);
	cprintfpad("RUNTIME",FIELDSIZE);
	cprintfpad("VRUNTIME",FIELDSIZE);
	cprintf("TOTAL TICKS %d\n",totalticks);
	cprintf("\n");
}
void printcontent(struct proc* p){
	char strpid[FIELDSIZE],
		strstate[FIELDSIZE],
		strnice[FIELDSIZE],
		strroverw[FIELDSIZE],
		strruntime[FIELDSIZE],
		strvruntime[FIELDSIZE];
		
		
	strint(p->pid,strpid);
	strprocstate(strstate,p->state);
	strint(p->schedstate.nice,strnice);
	strint(p->schedstate.runtime/weights[p->schedstate.nice],strroverw);
	strint(p->schedstate.runtime,strruntime);
	strint(p->schedstate.vruntime,strvruntime);

	cprintfpad(p->name,FIELDSIZE);
	cprintfpad(strpid,FIELDSIZE);
	cprintfpad(strstate,FIELDSIZE);
	cprintfpad(strnice,FIELDSIZE);
	cprintfpad(strroverw,FIELDSIZE);
	cprintfpad(strruntime,FIELDSIZE);
	cprintfpad(strvruntime,FIELDSIZE);
	cprintf("\n");
}

/*
Syscall ps(int pid)
Should print name, pid, state and nice of:
- All processes if pid = 0
- The process with the pid value if pid != 0
- Nothing if pid not in process list

Input:
- The pid of any process that we want to display, all if pid = 0;'
*/
void ps(int pid){
	acquire(&ptable.lock);
	if(pid==0){
		int hasHeader = 0;
		for(struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			if(!hasHeader){
				printheader();
				hasHeader = 1;
			}
			printcontent(p);
		}
	} else if(pid>0){
		for(struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			if(pid == p->pid){
				printheader();
				printcontent(p);
				break;
			}
		}
	}
	release(&ptable.lock);
}


//-------DEBUG AND VISUALIZATION FUNCTIONS-------//

void printvariabletable(struct proc* ptable){
	cprintfpad("PID",FIELDSIZE);
	cprintfpad("NICE",FIELDSIZE);
	cprintfpad("TIMESLICE",FIELDSIZE);
	cprintfpad("RUNTIME",FIELDSIZE);
	cprintfpad("VRUNTIME",FIELDSIZE);
	cprintf("\n");
	for(struct proc* p = ptable; p < &ptable[NPROC]; p++){
		if(p->pid != 0){
			char strpid[FIELDSIZE],
			strnice[FIELDSIZE],
			strtimeslice[FIELDSIZE],
			strruntime[FIELDSIZE],
			strvruntime[FIELDSIZE];
			strint(p->pid,strpid);
			strint(p->schedstate.nice,strnice);
			strint(p->schedstate.timeslice,strtimeslice);
			strint(p->schedstate.runtime,strruntime);
			strint(p->schedstate.vruntime,strvruntime);
			cprintfpad(strpid,FIELDSIZE);
			cprintfpad(strnice,FIELDSIZE);
			cprintfpad(strtimeslice,FIELDSIZE);
			cprintfpad(strruntime,FIELDSIZE);
			cprintfpad(strvruntime,FIELDSIZE);
			cprintf("\n");
		}
	}
}

void printgantline(struct proc* ptable) {
    int firstprint = 1;
    static int printedpids[NPROC] = {0};
    
    if (firstprint) {
        for(struct proc* p = ptable; p < &ptable[NPROC]; p++) {
            if(p->pid != 0 && !printedpids[p->pid]) {
                char pidstr[8];
                char nicestr[4];
                char combined[GANTFIELDSIZE] = {0};
                
                strint(p->pid, pidstr);
                strint(p->schedstate.nice, nicestr);
                
                int pos = 0;
                for(int i = 0; pidstr[i] && pos < GANTFIELDSIZE-1; i++) {
                    combined[pos++] = pidstr[i];
                }
                if(pos < GANTFIELDSIZE-1) combined[pos++] = '(';
                for(int i = 0; nicestr[i] && pos < GANTFIELDSIZE-1; i++) {
                    combined[pos++] = nicestr[i];
                }
                if(pos < GANTFIELDSIZE-1) combined[pos++] = ')';
                combined[pos] = '\0';
                
                cprintfpad(combined,GANTFIELDSIZE);
                printedpids[p->pid] = 1;
            }
        }
        cprintf("\n");
        firstprint = 0;
    }
    
    // Print state symbols
    for(struct proc* p = ptable; p < &ptable[NPROC]; p++) {
        if(p->pid != 0) {
            if(p->state == RUNNABLE) {
                cprintfpad("r", GANTFIELDSIZE);
            } else if(p->state == SLEEPING) {
                cprintfpad("z", GANTFIELDSIZE);
            } else if(p->state == RUNNING) {
                cprintfpad("#", GANTFIELDSIZE);
            } else if(p->state == ZOMBIE) {
                cprintfpad("Z", GANTFIELDSIZE);
            } else {
                cprintfpad(".", GANTFIELDSIZE);
            }
        }
    }
    //cprintf("\n");
}
/*
Function that writes the pid, nice, vruntime\n in the current timeslice, takes in a process pointer and adds it to the current line in the char** buffer.

void logtick(struct proc* p, int tick) {
	if(!(tick>= 0 && tick < LOGTICKS)){
		panic("Ran out of buffer space");
		
	}
	char strpid[LOGPIDSIZE];
	char strnice[LOGNICESIZE];
	char strvruntime[LOGVRUNTIMESIZE];
	char* buf = logbuffer[tick];

	strint(p->pid, strpid);
	strint(p->schedstate.nice, strnice);
	strint(p->schedstate.vruntime, strvruntime);

	memcpy(buf, strpid, strlen(strpid));
	buf += strlen(strpid);
	*buf++ = ',';

	memcpy(buf, strnice, strlen(strnice));
	buf += strlen(strnice);
	*buf++ = ',';

	memcpy(buf, strvruntime, strlen(strvruntime));
	buf += strlen(strvruntime);
	*buf++ = '\n';
	*buf = '\0';
}
*/


