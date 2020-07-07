#include "lib.h"

int mutex;
int full;
int empty;
int buf[102];
int head;
int tail;

void consumer();
void producer();

void umain()
{
	//writef("here?\n");
	mutex = syscall_init_PV_var(1);
	full = syscall_init_PV_var(0);
	empty = syscall_init_PV_var(100);
	head = 0;
	tail = 0;
	//writef("test begins\n");
	int who = fork();
	if (who == 0) {
		//writef("baba\n");
		producer();
	}
	else {
		//writef("son\n");
		consumer();
	}
}

void consumer()
{
	while(1)
	{
		//writef("000\n");
		syscall_P(full);
		//writef("111\n");
		syscall_P(mutex);
		//writef("222\n");
		head = (head + 1) % 100;
		buf[head] = head;
		writef("consumer gets %d from buf[%d]\n",head,head);
		syscall_V(mutex);
		syscall_V(empty);		
	}
}

void producer()
{
	while(1)
	{
		//writef("000\n");
		syscall_P(empty);
		//writef("111\n");
		syscall_P(mutex);
		//writef("222\n");
		tail = (tail + 1) % 100;
		buf[tail] = tail;
		writef("producer puts %d to buf[%d]\n",tail,tail);
		syscall_V(mutex);
		syscall_V(full);
	}	
}
