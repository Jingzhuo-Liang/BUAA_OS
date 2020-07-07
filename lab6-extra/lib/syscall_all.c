#include "../drivers/gxconsole/dev_cons.h"
#include <mmu.h>
#include <env.h>
#include <printf.h>
#include <pmap.h>
#include <sched.h>
#define semalen  102

extern char *KERNEL_SP;
extern struct Env *curenv;
struct Semaphore sema[104];

int sys_init_PV_var(int sysno,int init_value)
{
	int i;
	for (i = 0;i < semalen;i++) {
		if (sema[i].valid == 0) {
			sema[i].valid = 1;
			sema[i].value = init_value;
			sema[i].head = 0;
			sema[i].tail = 0;
			//printf("alloc a semaphore %d->%d\n",i,init_value);
			return i;
		}
	}
	return -1;	
}

void sys_P(int sysno,int pv_id)
{
	if (pv_id < 0 || sema[pv_id].valid == 0) 
		return;
	sema[pv_id].value--;
	if (sema[pv_id].value < 0) {
		curenv->env_status = ENV_NOT_RUNNABLE;
		//sema[pv_id].tail = (sema[pv_id].tail + 1)%1024;
		//sema[pv_id].env_list[sema[pv_id].tail%1024] = curenv;
		//sema[pv_id].tail++;
		sema[pv_id].tail = (sema[pv_id].tail + 1)%1024;
		sema[pv_id].env_list[sema[pv_id].tail] = curenv;	
		sys_yield();
		//sched_yield();
	}
	//printf("operation P %d\n",pv_id);
}

void sys_V(int sysno,int pv_id)
{
	if (pv_id < 0 || sema[pv_id].valid == 0)
		return;
	sema[pv_id].value++;
	if (sema[pv_id].value <= 0) {
		
		//sema[pv_id].head = (sema[pv_id].head + 1)%1024;
		//sema[pv_id].env_list[sema[pv_id].head%1024]->env_status = ENV_RUNNABLE;
		//sema[pv_id].head++;
		sema[pv_id].head = (sema[pv_id].head + 1)%1024;
		sema[pv_id].env_list[sema[pv_id].head]->env_status = ENV_RUNNABLE;		//sys_yield();
		//sched_yield();
	}
	sys_yield();
	//printf("operation V %d\n",pv_id);
}

int sys_check_PV_value(int sysno,int pv_id)
{
	if (pv_id < 0)
		return -1;
	return sema[pv_id].value;
}

void sys_release_PV_var(int sysno,int pv_id)
{
	if (pv_id < 0)
		return;
	if (sema[pv_id].valid == 1) {
		sema[pv_id].valid = 0;
		int i;
		for (i = sema[pv_id].head;i < sema[pv_id].tail;i++)
			env_free(sema[pv_id].env_list[i]);
		//sema[pv_id].head = 0;
		//sema[pv_id].tail = 0;
	}
}

/* Overview:
 * 	This function is used to print a character on screen.
 * 
 * Pre-Condition:
 * 	`c` is the character you want to print.
 */
void sys_putchar(int sysno, int c, int a2, int a3, int a4, int a5)
{
	printcharc((char) c);
	return ;
}

/* Overview:
 * 	This function enables you to copy content of `srcaddr` to `destaddr`.
 *
 * Pre-Condition:
 * 	`destaddr` and `srcaddr` can't be NULL. Also, the `srcaddr` area 
 * 	shouldn't overlap the `destaddr`, otherwise the behavior of this 
 * 	function is undefined.
 *
 * Post-Condition:
 * 	the content of `destaddr` area(from `destaddr` to `destaddr`+`len`) will
 * be same as that of `srcaddr` area.
 */
void *memcpy(void *destaddr, void const *srcaddr, u_int len)
{
	char *dest = destaddr;
	char const *src = srcaddr;

	while (len-- > 0) {
		*dest++ = *src++;
	}

	return destaddr;
}

/* Overview:
 *	This function provides the environment id of current process.
 *
 * Post-Condition:
 * 	return the current environment id
 */
u_int sys_getenvid(void)
{
	return curenv->env_id;
}

/* Overview:
 *	This function enables the current process to give up CPU.
 *
 * Post-Condition:
 * 	Deschedule current environment. This function will never return.
 */
/*** exercise 4.6 ***/
void sys_yield(void)
{
//	struct Trapframe *old = (struct Trapframe *)(TIMESTACK-sizeof(struct Trapframe));
    //if (curenv != NULL) {
	bcopy((void *)KERNEL_SP - sizeof(struct Trapframe),(void*)TIMESTACK - sizeof(struct Trapframe),sizeof(struct Trapframe));
    //	curenv->env_tf.pc = old->cp0_epc;
    //}
	
	//printf("111\n");
	sched_yield();
}

/* Overview:
 * 	This function is used to destroy the current environment.
 *
 * Pre-Condition:
 * 	The parameter `envid` must be the environment id of a 
 * process, which is either a child of the caller of this function 
 * or the caller itself.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 when error occurs.
 */
int sys_env_destroy(int sysno, u_int envid)
{
	/*
		printf("[%08x] exiting gracefully\n", curenv->env_id);
		env_destroy(curenv);
	*/
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0) {
		return r;
	}

	printf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

/* Overview:
 * 	Set envid's pagefault handler entry point and exception stack.
 * 
 * Pre-Condition:
 * 	xstacktop points one byte past exception stack.
 *
 * Post-Condition:
 * 	The envid's pagefault handler will be set to `func` and its
 * 	exception stack will be set to `xstacktop`.
 * 	Returns 0 on success, < 0 on error.
 */
/*** exercise 4.12 ***/
int sys_set_pgfault_handler(int sysno, u_int envid, u_int func, u_int xstacktop)
{
	// Your code here.
	struct Env *env;
	int ret;
	ret = envid2env(envid,&env,0); // 1 ? PTE_V
	if (ret < 0)
		return ret;
	env->env_pgfault_handler = func;
	env->env_xstacktop = xstacktop;
	//printf("aaa\n");
	return 0;
	//	panic("sys_set_pgfault_handler not implemented");
}

/* Overview:
 * 	Allocate a page of memory and map it at 'va' with permission
 * 'perm' in the address space of 'envid'.
 *
 * 	If a page is already mapped at 'va', that page is unmapped as a
 * side-effect.
 * 
 * Pre-Condition:
 * perm -- PTE_V is required,
 *         PTE_COW is not allowed(return -E_INVAL),
 *         other bits are optional.
 *
 * Post-Condition:
 * Return 0 on success, < 0 on error
 *	- va must be < UTOP
 *	- env may modify its own address space or the address space of its children
 */
/*** exercise 4.3 ***/
int sys_mem_alloc(int sysno, u_int envid, u_int va, u_int perm)
{
	// Your code here.
	struct Env *env;
	struct Page *ppage;
	int ret;
	ret = 0;
	if (va >= UTOP|| (perm & PTE_COW)!=0 || (perm & PTE_V)==0)
	//if (va >= UTOP|| (perm & PTE_V) ==0)
		return -E_INVAL;
	if (envid2env(envid,&env,0)!=0)
		return -1;
	//if ((perm & PTE_V)==0)
	//	return  -1;
	//if ((perm & PTE_COW)!= 0)
	//	return -E_INVAL;
	if (page_alloc(&ppage) != 0)
		return -1;
	if (page_insert(env->env_pgdir,ppage,va,perm)!=0)
		return -1;
	//printf("222\n");
	return ret;
}

/* Overview:
 * 	Map the page of memory at 'srcva' in srcid's address space
 * at 'dstva' in dstid's address space with permission 'perm'.
 * Perm has the same restrictions as in sys_mem_alloc.
 * (Probably we should add a restriction that you can't go from
 * non-writable to writable?)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Note:
 * 	Cannot access pages above UTOP.
 */
/*** exercise 4.4 ***/
int sys_mem_map(int sysno, u_int srcid, u_int srcva, u_int dstid, u_int dstva,
				u_int perm)
{
	int ret;
	u_int round_srcva, round_dstva;
	struct Env *srcenv;
	struct Env *dstenv;
	struct Page *ppage;
	Pte *ppte;

	ppage = NULL;
	ret = 0;
	round_srcva = ROUNDDOWN(srcva, BY2PG);
	round_dstva = ROUNDDOWN(dstva, BY2PG);

    //your code here
		
	if ((srcva >= UTOP)||(dstva >= UTOP))
		return -E_INVAL;
	
	if ((perm & PTE_V)==0)
	//if ((perm & PTE_COW)!=0 || (perm & PTE_V)==0)
		return -E_INVAL;
	//srcenv = &envs[ENVX(srcid)];
	//dstenv = &envs[ENVX(dstid)];
	if (envid2env(srcid,&srcenv,0)!=0 || envid2env(dstid,&dstenv,0)!=0)
		return -1;
	ppage = page_lookup(srcenv->env_pgdir,round_srcva,&ppte);
	/*
	if (ppage == NULL|| ((*ppte & PTE_R)==0&&(perm & PTE_R)==1)) {
		return -1;
	}
	if (page2kva(ppage) >= UTOP) {
		return -1;
	}
	if (page_insert(dstenv->env_pgdir,ppage,round_dstva,perm) !=0)
		return -1;
	//printf("333\n");
	return 0;
	*/
	if (ppage == NULL)
	{
		return -E_INVAL;
	}
	if (((*ppte & PTE_R) == 0) && ((perm & PTE_R) == 1))
	{
		return -E_INVAL;
	}

	ppage = pa2page(PTE_ADDR(*ppte));
	ret = page_insert(dstenv->env_pgdir, ppage, round_dstva, perm);
	if (ret < 0)
	{
		return ret;
	}

	ret = 0;
	return ret;
}

/* Overview:
 * 	Unmap the page of memory at 'va' in the address space of 'envid'
 * (if no page is mapped, the function silently succeeds)
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Cannot unmap pages above UTOP.
 */
/*** exercise 4.5 ***/
int sys_mem_unmap(int sysno, u_int envid, u_int va)
{
	// Your code here.
	int ret = 0;
	//printf("444-begin");
	if (va >= UTOP)
		return -E_INVAL;
	struct Env *env;
	if (envid2env(envid,&env,0) != 0) // 0 or PTE_V
		return -1;
	page_remove(env->env_pgdir,va);
	//printf("444\n");
	return ret;
	//	panic("sys_mem_unmap not implemented");
}

/* Overview:
 * 	Allocate a new environment.
 *
 * Pre-Condition:
 * The new child is left as env_alloc created it, except that
 * status is set to ENV_NOT_RUNNABLE and the register set is copied
 * from the current environment.
 *
 * Post-Condition:
 * 	In the child, the register set is tweaked so sys_env_alloc returns 0.
 * 	Returns envid of new environment, or < 0 on error.
 */
/*** exercise 4.8 ***/
int sys_env_alloc(void)
{
	// Your code here.
	int r;
	struct Env *e;
	//printf("1010-begin\n");
	r = env_alloc(&e,curenv->env_id);
	if (r !=0 )
		return r;
	//struct Trapframe *old = (struct Trapframe *)(TIMESTACK-sizeof(struct Trapframe));
	//bcopy(&(curenv->env_tf),&(e->env_tf),sizeof(struct Trapframe)); 
	bcopy((void *)KERNEL_SP - sizeof(struct Trapframe),&(e->env_tf),sizeof(struct Trapframe));
	e->env_tf.pc = e->env_tf.cp0_epc; 
	e->env_status = ENV_NOT_RUNNABLE;
	e->env_pri = curenv->env_pri;
	e->env_tf.regs[2] = 0;
	return e->env_id;
	//printf("1010\n");
	//	panic("sys_env_alloc not implemented");
}

/* Overview:
 * 	Set envid's env_status to status.
 *
 * Pre-Condition:
 * 	status should be one of `ENV_RUNNABLE`, `ENV_NOT_RUNNABLE` and
 * `ENV_FREE`. Otherwise return -E_INVAL.
 * 
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if status is not a valid status for an environment.
 * 	The status of environment will be set to `status` on success.
 */
/*** exercise 4.14 ***/
int sys_set_env_status(int sysno, u_int envid, u_int status)
{
	// Your code here.
	struct Env *env;
	int ret;
	//printf("bbb-begin\n");
	ret = envid2env(envid,&env,0);
	if (ret != 0)
		return ret;
	if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE && status != ENV_FREE)
		return -E_INVAL;
	env->env_status = status;
	if (status == ENV_FREE)
		env_destroy(env);
	//LIST_INSERT_HEAD(&env_sched_list[0],env,env_link); // difference
	//printf("bbb\n");
	return 0;
	//	panic("sys_env_set_status not implemented");
}

/* Overview:
 * 	Set envid's trap frame to tf.
 *
 * Pre-Condition:
 * 	`tf` should be valid.
 *
 * Post-Condition:
 * 	Returns 0 on success, < 0 on error.
 * 	Return -E_INVAL if the environment cannot be manipulated.
 *
 * Note: This hasn't be used now?
 */
int sys_set_trapframe(int sysno, u_int envid, struct Trapframe *tf)
{

	return 0;
}

/* Overview:
 * 	Kernel panic with message `msg`. 
 *
 * Pre-Condition:
 * 	msg can't be NULL
 *
 * Post-Condition:
 * 	This function will make the whole system stop.
 */
void sys_panic(int sysno, char *msg)
{
	// no page_fault_mode -- we are trying to panic!
	panic("%s", TRUP(msg));
}

/* Overview:
 * 	This function enables caller to receive message from 
 * other process. To be more specific, it will flag 
 * the current process so that other process could send 
 * message to it.
 *
 * Pre-Condition:
 * 	`dstva` is valid (Note: NULL is also a valid value for `dstva`).
 * 
 * Post-Condition:
 * 	This syscall will set the current process's status to 
 * ENV_NOT_RUNNABLE, giving up cpu. 
 */
/*** exercise 4.7 ***/
void sys_ipc_recv(int sysno, u_int dstva)
{
	//printf("555-begin\n");
	if (dstva >= UTOP)
		return;
	curenv->env_ipc_recving = 1;
	curenv->env_ipc_dstva = dstva;
	curenv->env_status = ENV_NOT_RUNNABLE;
	//printf("555-end\n");
	sys_yield();
}

/* Overview:
 * 	Try to send 'value' to the target env 'envid'.
 *
 * 	The send fails with a return value of -E_IPC_NOT_RECV if the
 * target has not requested IPC with sys_ipc_recv.
 * 	Otherwise, the send succeeds, and the target's ipc fields are
 * updated as follows:
 *    env_ipc_recving is set to 0 to block future sends
 *    env_ipc_from is set to the sending envid
 *    env_ipc_value is set to the 'value' parameter
 * 	The target environment is marked runnable again.
 *
 * Post-Condition:
 * 	Return 0 on success, < 0 on error.
 *
 * Hint: the only function you need to call is envid2env. 
 */
/*** exercise 4.7 ***/
int sys_ipc_can_send(int sysno, u_int envid, u_int value, u_int srcva,
					 u_int perm)
{
	//printf("666-begin\n");
	int r;
	struct Env *e;
	struct Page *p;
	Pte *pte;
	if (envid2env(envid,&e,0) != 0 || srcva >= UTOP) {
	//	printf("envid:%d\n",envid);
		return -1;
	}
	if (e->env_ipc_recving == 0){
	//	printf("999----srcva:0x%x\n",srcva);
		return -E_IPC_NOT_RECV;
	}
	e->env_ipc_recving = 0;
	e->env_ipc_value = value;
	e->env_status = ENV_RUNNABLE;
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_perm = perm | PTE_V | PTE_R; // perm -> perm | PTE_V | PTE_R
	if (srcva != 0) {
		//printf("here?\n");
		p = page_lookup(curenv->env_pgdir,srcva,&pte);
		if (p != NULL) 
		    page_insert(e->env_pgdir,p,e->env_ipc_dstva,perm);
		else {
	//		printf("777\n");
			return -1;
		}
	}
	//printf("666-end\n");
	return 0;
}

