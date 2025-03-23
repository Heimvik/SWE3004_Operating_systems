# Project 1 notes

## Control flow of a system call
The execution goes as follows:
1. Upon OS boot, tvinit() is run. This initializes the IDT (Interrupt Descriptor Table) which holds all the interrupt handlers held at the same index in vectors array. All the interrupt vectors are set up by the vectors.S. The system call interrupt vectors are defined system call number offset.
2. Whenever the user does a system call fork() exit(), etc, we will jump to the corresponding label in SYSCALL(). The syscall number is put in the eax register as the argument and int $T_SYSCALL is called. T_SYSCALL is the offset to the IDT entries where syscall handler adresses are stored (T_SYSCALL = 64 defined in systemcall.h). 
3. A normal interrupt is done, where HW saves context by pushing registers onto the current process' kernel stack and then jumping to the instruction address at the respective entry in IDT. The way vectors.S is set up by the vectors.pl, is that at a trap is invoked (jmp alltrap) at any interrupt. 
4. alltraps sets up the trapframe of the current executing process which contains everything the user process needs to be restored. After having set up the trapframe on the stack with the pointer to the location on the kernel stack  in eax, it makes a call to label trap in traps.c
5. traps(struct trapframe* tf) is the trap handler and takes in the pointer to the trapframe (stored in eax) from alltraps and decides what to do with it. Upon a system call it assigns the trapframe to the current executing process and invokes syscall() in syscall.c
6. In syscall.c we have all the system call table and with all the wrapper syscall functions indexed by their syscall number. The wrapper functions are implemented to make seperation between the fetching of arguments with argxxx() and the actual functionality of the syscall. The wrapper syscall is called
7. In the wrapper syscall in sysproc.c/sysfile.c the real, basic functionality is called with the wrapper-fetched arguments.
8. ***System call executing***
9. Return value returned to wrapper syscall.
10. The now returned syscall() in syscall.c now sets curproc->eax register to the return value of the wrapper syscall, and then proceeds to trap.c where syscall() was called first time.
11. In trap() in trap.c we return again unless the process we came from is killed.
12. We return to alltraps(), where the trapframe adress at the kernel stack is deallocated, for the return to continue execution down to the next label, being trapret. Here we pop off the trapframe off the stack. We then perform the iret (interrupt return) instruction. This restores the context of the user space process done in HW, also effectively lowering the priviledge level back to user mode. 
