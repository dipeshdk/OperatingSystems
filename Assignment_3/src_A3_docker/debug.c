#include <debug.h>
#include <context.h>
#include <entry.h>
#include <lib.h>
#include <memory.h>


/*****************************HELPERS******************************************/

/* 
 * allocate the struct which contains information about debugger
 *
 */
struct debug_info *alloc_debug_info()
{
	struct debug_info *info = (struct debug_info *) os_alloc(sizeof(struct debug_info)); 
	if(info)
		bzero((char *)info, sizeof(struct debug_info));
	return info;
}

/*
 * frees a debug_info struct 
 */
void free_debug_info(struct debug_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct debug_info));
}

/*
 * allocates memory to store registers structure
 */
struct registers *alloc_regs()
{
	struct registers *info = (struct registers*) os_alloc(sizeof(struct registers)); 
	if(info)
		bzero((char *)info, sizeof(struct registers));
	return info;
}

/*
 * frees an allocated registers struct
 */
void free_regs(struct registers *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct registers));
}

/* 
 * allocate a node for breakpoint list 
 * which contains information about breakpoint
 */
struct breakpoint_info *alloc_breakpoint_info()
{
	struct breakpoint_info *info = (struct breakpoint_info *)os_alloc(
		sizeof(struct breakpoint_info));
	if(info)
		bzero((char *)info, sizeof(struct breakpoint_info));
	return info;
}

/*
 * frees a node of breakpoint list
 */
void free_breakpoint_info(struct breakpoint_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct breakpoint_info));
}

/*
 * Fork handler.
 * The child context doesnt need the debug info
 * Set it to NULL
 * The child must go to sleep( ie move to WAIT state)
 * It will be made ready when the debugger calls wait_and_continue
 */
void debugger_on_fork(struct exec_context *child_ctx)
{
	child_ctx->dbg = NULL;	
	child_ctx->state = WAITING;	
}


/******************************************************************************/

/* This is the int 0x3 handler
 * Hit from the childs context
 */
long int3_handler(struct exec_context *ctx)
{
	if(!ctx) return -1;
	
	struct exec_context *debugger_ctx =  get_ctx_by_pid(ctx->ppid);
	if(!debugger_ctx || !(debugger_ctx->dbg)) return -1;

	ctx->regs.entry_rip--;

	debugger_ctx->dbg->regs_bfr_bpt->r15 = ctx->regs.r15;
	debugger_ctx->dbg->regs_bfr_bpt->r14 = ctx->regs.r14;
	debugger_ctx->dbg->regs_bfr_bpt->r13 = ctx->regs.r13;
	debugger_ctx->dbg->regs_bfr_bpt->r12 = ctx->regs.r12;
	debugger_ctx->dbg->regs_bfr_bpt->r11 = ctx->regs.r11;
	debugger_ctx->dbg->regs_bfr_bpt->r10 = ctx->regs.r10;
	debugger_ctx->dbg->regs_bfr_bpt->r9 = ctx->regs.r9;
	debugger_ctx->dbg->regs_bfr_bpt->r8 = ctx->regs.r8;
	debugger_ctx->dbg->regs_bfr_bpt->rbp = ctx->regs.rbp;
	debugger_ctx->dbg->regs_bfr_bpt->rdi = ctx->regs.rdi;
	debugger_ctx->dbg->regs_bfr_bpt->rsi = ctx->regs.rsi;
	debugger_ctx->dbg->regs_bfr_bpt->rdx = ctx->regs.rdx;
	debugger_ctx->dbg->regs_bfr_bpt->rcx = ctx->regs.rcx;
	debugger_ctx->dbg->regs_bfr_bpt->rbx = ctx->regs.rbx;
	debugger_ctx->dbg->regs_bfr_bpt->rax = ctx->regs.rax;
	debugger_ctx->dbg->regs_bfr_bpt->entry_rip = ctx->regs.entry_rip;
	debugger_ctx->dbg->regs_bfr_bpt->entry_cs = ctx->regs.entry_cs;
	debugger_ctx->dbg->regs_bfr_bpt->entry_rflags = ctx->regs.entry_rflags;
	debugger_ctx->dbg->regs_bfr_bpt->entry_rsp = ctx->regs.entry_rsp;
	debugger_ctx->dbg->regs_bfr_bpt->entry_ss = ctx->regs.entry_ss;

	debugger_ctx->regs.rax = ctx->regs.entry_rip;
	ctx->regs.entry_rip++;
	
	ctx->regs.entry_rsp -= 8;
	*(u64*)ctx->regs.entry_rsp = ctx->regs.rbp;

	int idx = 0;
	debugger_ctx->dbg->bt_info[idx++] = ctx->regs.entry_rip-1;
	u64 addr = ctx->regs.entry_rsp;
	while(idx < MAX_BACKTRACE){
		u64 data  = (u64)*(u64*)(addr + 8);
		if(data == END_ADDR){
			break;
		}
		debugger_ctx->dbg->bt_info[idx++] = data;
		addr = (u64)*(u64*)(addr);
	}
	debugger_ctx->dbg->count_backtrace = idx;

	
	ctx->regs.rax = 0x0;
	debugger_ctx->state = READY;
	ctx->state = WAITING;
	schedule(debugger_ctx);
	return 0;
}


/*
 * Exit handler.
 * Called on exit of Debugger and Debuggee 
 */
void debugger_on_exit(struct exec_context *ctx)
{
	if(!ctx) return;
	if(ctx->dbg != NULL){
		struct breakpoint_info* node = ctx->dbg->head;
		struct breakpoint_info* prev_node = NULL;
		while(node){
			prev_node = node;
			node = node->next;
			free_breakpoint_info(prev_node);
		}
		ctx->dbg->head = NULL;
		free_regs(ctx->dbg->regs_bfr_bpt);
		ctx->dbg->regs_bfr_bpt = NULL;
		free_debug_info(ctx->dbg);
		ctx->dbg = NULL;
	}else{
		struct exec_context *debugger_ctx =  get_ctx_by_pid(ctx->ppid);
		if(!debugger_ctx) return;
		debugger_ctx->regs.rax = CHILD_EXIT;
		debugger_ctx->state = READY;
	}	
}
	
/*
 * called from debuggers context
 * initializes debugger state
 */
int do_become_debugger(struct exec_context *ctx)
{
	if(!ctx){
		return -1;
	}
	ctx->dbg = alloc_debug_info();
	if(ctx->dbg == NULL) return -1;
	ctx->dbg->head = NULL;
	ctx->dbg->bpt_num = 0;
	ctx->dbg->count_bpt = 0;
	ctx->dbg->regs_bfr_bpt = alloc_regs();
	if(ctx->dbg->regs_bfr_bpt == NULL) return -1;
	return 0;
}

/*
 * called from debuggers context
 */
int do_set_breakpoint(struct exec_context *ctx, void *addr)
{
	if(!ctx || !(ctx->dbg)) return -1;	
	
	struct breakpoint_info *node = ctx->dbg->head;
	struct breakpoint_info *prev_node = NULL;
	
	while(node){
		if(node->addr == (long)addr){
			node->status = 1;
			return 0;
		}
		prev_node = node;
		node = node->next;
	}
	
	if(ctx->dbg->count_bpt == MAX_BREAKPOINTS){
		return -1;
	}
	
	struct breakpoint_info *new_node = alloc_breakpoint_info();
	if(!new_node) return -1;

	ctx->dbg->bpt_num++;
	ctx->dbg->count_bpt++;

	new_node->status = 1;
	new_node->addr = (long)addr;
	new_node->next = NULL;
	new_node->num =  ctx->dbg->bpt_num;
	
	if(ctx->dbg->head == NULL) ctx->dbg->head = new_node;	
	else prev_node->next = new_node;
	*(unsigned*)addr = (*(unsigned*)addr & ~(0xFF)) | INT3_OPCODE; 
	
	return 0;
}


/*
 * called from debuggers context
 */
int do_remove_breakpoint(struct exec_context *ctx, void *addr)
{
	if(!ctx || !(ctx->dbg)) return -1;

	struct breakpoint_info *node = ctx->dbg->head;
	struct breakpoint_info *prev_node = NULL;
	
	while(node && node->addr != (u64)addr){
		prev_node = node;
		node = node->next;
	}
	
	if(!node || node->addr != (u64)addr) return -1;
	
	*(unsigned*)addr = (*(unsigned*)addr & ~(0xFF)) | PUSHRBP_OPCODE;
	
	if(prev_node == NULL) ctx->dbg->head = node->next;
	else prev_node->next = node->next;

	free_breakpoint_info(node);
	ctx->dbg->count_bpt--;
	
	return 0;	
}


/*
 * called from debuggers context
 */
int do_enable_breakpoint(struct exec_context *ctx, void *addr)
{
	if(!ctx || !(ctx->dbg)) return -1;	
	
	struct breakpoint_info *node = ctx->dbg->head;
	
	while(node){
		if(node->addr == (long)addr){
			if(node->status == 0){
				node->status = 1;
				*(unsigned*)addr = (*(unsigned*)addr & (0xFFFFFF00)) | INT3_OPCODE; 
			} 
			return 0;
		}
		node = node->next;
	}

	return -1;
}

/*
 * called from debuggers context
 */
int do_disable_breakpoint(struct exec_context *ctx, void *addr)
{
	if(!ctx || !(ctx->dbg)) return -1;	
	
	struct breakpoint_info *node = ctx->dbg->head;
	
	while(node){
		if(node->addr == (long)addr){
			if(node->status){
				node->status = 0;
				*(unsigned*)addr = (*(unsigned*)addr & (0xFFFFFF00)) | PUSHRBP_OPCODE; 
			} 
			return 0;
		}
		node = node->next;
	}

	return -1;
}

/*
 * called from debuggers context
 */ 
int do_info_breakpoints(struct exec_context *ctx, struct breakpoint *ubp)
{
	if(!ctx || !(ctx->dbg)) return -1;	
	
	struct breakpoint_info *node = ctx->dbg->head;
	
	int i = 0;
	while(node){
		ubp[i].addr = node->addr;
		ubp[i].num = node->num;
		ubp[i].status = node->status;

		node = node->next;
		i++;
	}
	return i;
}

/*
 * called from debuggers context
 */
int do_info_registers(struct exec_context *ctx, struct registers *regs)
{
	if(!ctx || !regs) return -1;

	regs->r15 = ctx->dbg->regs_bfr_bpt->r15;
	regs->r14 = ctx->dbg->regs_bfr_bpt->r14;
	regs->r13 = ctx->dbg->regs_bfr_bpt->r13;
	regs->r12 = ctx->dbg->regs_bfr_bpt->r12;
	regs->r11 = ctx->dbg->regs_bfr_bpt->r11;
	regs->r10 = ctx->dbg->regs_bfr_bpt->r10;
	regs->r9 = ctx->dbg->regs_bfr_bpt->r9;
	regs->r8 = ctx->dbg->regs_bfr_bpt->r8;
	regs->rbp = ctx->dbg->regs_bfr_bpt->rbp;
	regs->rdi = ctx->dbg->regs_bfr_bpt->rdi;
	regs->rsi = ctx->dbg->regs_bfr_bpt->rsi;
	regs->rdx = ctx->dbg->regs_bfr_bpt->rdx;
	regs->rcx = ctx->dbg->regs_bfr_bpt->rcx;
	regs->rbx = ctx->dbg->regs_bfr_bpt->rbx;
	regs->rax = ctx->dbg->regs_bfr_bpt->rax;
	regs->entry_rip = ctx->dbg->regs_bfr_bpt->entry_rip;
	regs->entry_cs = ctx->dbg->regs_bfr_bpt->entry_cs;
	regs->entry_rflags = ctx->dbg->regs_bfr_bpt->entry_rflags;
	regs->entry_rsp = ctx->dbg->regs_bfr_bpt->entry_rsp;
	regs->entry_ss = ctx->dbg->regs_bfr_bpt->entry_ss;

	return 0;
}

/* 
 * Called from debuggers context
 */
int do_backtrace(struct exec_context *ctx, u64 bt_buf)
{
	if(!ctx || !(ctx->dbg)) return -1;
	
	int count = ctx->dbg->count_backtrace;
	for(int i =0 ;i < count; i++){
		((u64 *)bt_buf)[i] = ctx->dbg->bt_info[i];
	}
	
	return count;
}

/*
 * When the debugger calls wait
 * it must move to WAITING state 
 * and its child must move to READY state
 */
s64 do_wait_and_continue(struct exec_context *ctx)
{
	// The debugger should go to the WAITING state and the debugee should
	// go to the READY state and should be scheduled now.
	int child_is_present = 0;
	struct exec_context* child_ctx = NULL;
	
	for(int pidd = 1; pidd < MAX_PROCESSES ; pidd++){
		if(pidd == ctx->pid) continue;
		child_ctx = get_ctx_by_pid(pidd);
		if(child_ctx && child_ctx->ppid == ctx->pid){
			child_is_present = 1;
			break;
		}
	}

	if(!child_is_present) return -1;

	child_ctx->state = READY;
	ctx->state = WAITING;
	schedule(child_ctx);
	return 0;
}
