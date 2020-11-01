#ifndef __MSG_QUEUE_H_
#define __MSG_QUEUE_H_

#include <types.h>
#include <context.h>

#define MAX_TXT_SIZE 24 
#define MAX_MEMBERS 4 
#define BROADCAST_PID 0xffffffff 
#define NO_PID -1
#define NO_INDEX -1
#define MAX_MSG 32 
struct message{

	u32 from_pid;
	u32 to_pid;
	
	// will be a null terminated string
	char msg_txt[MAX_TXT_SIZE];
};

struct msg_queue_info{
	// define all the data structures
	// you need to maintain the message queue
	
	// 4 members, 0,1,2,3
	int members_mapping[MAX_MEMBERS];

	// 4 queues
	struct message *msg_buffer;
	int front[MAX_MEMBERS];
	int last[MAX_MEMBERS];

	// 4 blocked members array
	int blocked[MAX_MEMBERS][MAX_MEMBERS];
};

struct msg_queue_member_info{
	u32 member_count;
	u32 member_pid[MAX_MEMBERS];
};

extern int do_create_msg_queue(struct exec_context *ctx);
extern void do_add_child_to_msg_queue(struct exec_context *child_ctx);
extern void do_msg_queue_cleanup(struct exec_context *ctx);
extern int do_msg_queue_get_member_info(struct exec_context *ctx, struct file *filep, struct msg_queue_member_info *info);
extern int do_msg_queue_send(struct exec_context *ctx, struct file *filep, struct message *msg);
extern int do_msg_queue_rcv(struct exec_context *ctx, struct file *filep, struct message *msg);
extern int do_get_msg_count(struct exec_context *ctx, struct file *filep);
extern int do_msg_queue_block(struct exec_context *ctx, struct file *filep, int pid);
extern int do_msg_queue_close(struct exec_context *ctx, int fd);
#endif
