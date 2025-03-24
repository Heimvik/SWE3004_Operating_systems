
void strformatps(char* dst,const int nrows, const int nfields, char*** content){
	int dstindex = 0;
	for(int row = 0;row<nrows;row++){
		for(int field = 0;field<nfields;field++){
			dst[dstindex++] = '|';
			char* currentchar = content[row][field];
			while(*(currentchar)!= '\0'){
				dst[dstindex++] = (*currentchar++);
			}
			while(dstindex%FIELDSIZE != 0){
				dst[dstindex++] = ' ';
			}
			dst[dstindex++] = '|';
		}
		dst[dstindex++] = '\n';
	}
	dst[dstindex] = '\0';
	return;
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
    const char* header[NFIELDS] = {"Name","PID","STATE", "PRIORITY"};
	char content[1+NPROC][NFIELDS][FIELDSIZE];

	char strstate[10];
	int singleprocindex= -1;

	acquire(&ptable.lock);
	struct proc* p = ptable.proc;
	for(int procindex = 1; procindex<1+NPROC; procindex++){
		strprocstate(strstate,p->state);
		safestrcpy(content[procindex][0],(char*)(p->name),strlen(p->name));
		safestrcpy(content[procindex][1],(char*)(&(p->pid)),sizeof(char));
		safestrcpy(content[procindex][2],(char*)strstate,strlen(strstate));
		safestrcpy(content[procindex][3],(char*)(&(p->nice)),sizeof(char));
		if(pid == p->pid){
			singleprocindex = procindex;
			break;
		}
		p++;
	}
	release(&ptable.lock); //Release the lock to ptable asap
	for(int i = 0;i<NFIELDS;i++){
		safestrcpy(content[0][i],header[i],FIELDSIZE-1);
	}
	if(singleprocindex != -1){
		char strprint[2*NFIELDS*(FIELDSIZE+2)];
		memmove((void*)content[1],(void*)content[singleprocindex],(uint)(NFIELDS*FIELDSIZE));
		strformatps(strprint,2,NFIELDS,(char***)content);
		cprintf(strprint);
	} else {
		char strprint[NPROC*NFIELDS*(FIELDSIZE+2)];
		strformatps(strprint,NPROC,NFIELDS,(char***)content);
		cprintf(strprint);
	}
	return;
}
