#include <msg_queue.h>
#include <context.h>
#include <memory.h>
#include <file.h>
#include <lib.h>
#include <entry.h>



/************************************************************************************/
/***************************Do Not Modify below Functions****************************/
/************************************************************************************/

struct msg_queue_info *alloc_msg_queue_info()
{
	struct msg_queue_info *info;
	info = (struct msg_queue_info *)os_page_alloc(OS_DS_REG);
	
	if(!info){
		return NULL;
	}
	return info;
}

void free_msg_queue_info(struct msg_queue_info *q)
{
	os_page_free(OS_DS_REG, q);
}

struct message *alloc_buffer()
{
	struct message *buff;
	buff = (struct message *)os_page_alloc(OS_DS_REG);
	if(!buff)
		return NULL;
	return buff;	
}

void free_msg_queue_buffer(struct message *b)
{
	os_page_free(OS_DS_REG, b);
}

/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/

int enqueue(struct message* msg, struct msg_queue_info* mqi, int mem_idx){
    int fidx = mqi->front[mem_idx];
	int lidx = mqi->last[mem_idx];

	if ((mqi->front[mem_idx] == 0 && mqi->last[mem_idx] == MAX_MSG-1) || (mqi->last[mem_idx] == (mqi->front[mem_idx]-1)%(MAX_MSG-1))) { 
        return -EINVAL; 
    } 
    
	if (mqi->front[mem_idx] == -1){ 
        mqi->front[mem_idx] = mqi->last[mem_idx] = 0; 
    }else if (mqi->last[mem_idx] == MAX_MSG-1 && mqi->front[mem_idx] != 0) { 
        mqi->last[mem_idx] = 0; 
    }else{ 
        mqi->last[mem_idx]++; 
    }

	mqi->msg_buffer[mem_idx*MAX_MSG + mqi->last[mem_idx]].from_pid = msg->from_pid;
	mqi->msg_buffer[mem_idx*MAX_MSG + mqi->last[mem_idx]].to_pid = msg->to_pid;
	int i =0;
	while(i< MAX_TXT_SIZE && msg->msg_txt[i]!='\0'){
		mqi->msg_buffer[mem_idx*MAX_MSG + mqi->last[mem_idx]].msg_txt[i] = msg->msg_txt[i];
		i++;
	}
	mqi->msg_buffer[mem_idx*MAX_MSG + mqi->last[mem_idx]].msg_txt[i] = '\0'; 
	return 0;
}


int dequeue(struct message* msg, struct msg_queue_info* mqi, int mem_idx){
	if (mqi->front[mem_idx] == -1){ 
        return 0; 
    } 
  
    msg->from_pid = mqi->msg_buffer[mem_idx*MAX_MSG + mqi->front[mem_idx]].from_pid;
	msg->to_pid = mqi->msg_buffer[mem_idx*MAX_MSG + mqi->front[mem_idx]].to_pid;
	int i =0;
	while(i< MAX_TXT_SIZE && mqi->msg_buffer[mem_idx*MAX_MSG + mqi->front[mem_idx]].msg_txt[i]!='\0'){
		msg->msg_txt[i] = mqi->msg_buffer[mem_idx*MAX_MSG + mqi->front[mem_idx]].msg_txt[i];
		i++;
	}
	msg->msg_txt[i] = '\0';
  
    if (mqi->front[mem_idx] == mqi->last[mem_idx]) 
    { 
        mqi->front[mem_idx] = -1; 
        mqi->last[mem_idx] = -1; 
    } 
    else if (mqi->front[mem_idx] == MAX_MSG-1) 
        mqi->front[mem_idx] = 0; 
    else
        mqi->front[mem_idx]++;
        
	return 1;
}	


int do_create_msg_queue(struct exec_context *ctx)
{
	/** 
	 * Implement functionality to
	 * create a message queue
	 **/
	if(ctx == NULL){
		return -EINVAL;
	}
	// assign the fd to the file
	int fd = 3;
	while( fd <16 && ctx->files[fd])
		fd++;
	
	if(fd >= MAX_OPEN_FILES) return -ENOMEM;
	
	// create the file object
	struct file* filep = alloc_file();
	if(filep == NULL){
		return -ENOMEM;
	}

	// initialise the file struct elements
	filep->type = MSG_QUEUE;
	filep->mode = 0x0; // TODO
	filep->fops->read = NULL;
	filep->fops->write = NULL;
	filep->fops->lseek = NULL;
	filep->fops->close = NULL;
	filep->pipe = NULL;
	filep->inode = NULL; // TODO
	
	struct msg_queue_info* mqi = alloc_msg_queue_info();
	if(mqi == NULL){
		return -EINVAL;
	}

	// initialise the mqi elements
	for(int i = 0; i < MAX_MEMBERS; i++){
		mqi->members_mapping[i] = NO_PID;
		mqi->front[i] = -1;
		mqi->last[i] = -1;
		
		for(int j = 0; j < MAX_MEMBERS; j++){
			mqi->blocked[i][j] = NO_PID;
		}
	}

	mqi->members_mapping[0] = ctx->pid;
	mqi->msg_buffer = alloc_buffer();
	if(mqi->msg_buffer == NULL) return -ENOMEM;

	filep->msg_queue = mqi;

	ctx->files[fd] = filep;

	return fd;
}


int do_msg_queue_rcv(struct exec_context *ctx, struct file *filep, struct message *msg)
{
	/** 
	 * Implement functionality to
	 * recieve a message
	 **/
	if(ctx == NULL || filep == NULL){
		return -EINVAL;
	}

	int my_pid = ctx->pid;
	struct msg_queue_info* mqi = filep->msg_queue;
	
	// if filep does not point to the message queue then -EINVAL
	if(mqi == NULL || filep->type!= MSG_QUEUE){
		return -EINVAL;
	}

	int mem_idx = NO_INDEX;
	for(int i =0; i< MAX_MEMBERS; i++){
		if(my_pid == mqi->members_mapping[i]){
			mem_idx = i;
			break;
		}
	}
	
	// if the process is not the member of the message queue 
	if(mem_idx == NO_INDEX){
		return -EINVAL;
	}

	return dequeue(msg,mqi, mem_idx);
}


int do_msg_queue_send(struct exec_context *ctx, struct file *filep, struct message *msg)
{
	/** 
	 * Implement functionality to
	 * send a message
	 **/
	if(ctx == NULL || filep == NULL){
		return -EINVAL;
	}
	int sender_pid = msg->from_pid;
	struct msg_queue_info* mqi = filep->msg_queue;
	
	// if filep does not point to the message queue then -EINVAL
	if(mqi == NULL || filep->type!= MSG_QUEUE){
		return -EINVAL;
	}

	// to_pid decides unicast and broadcast
	if(msg->to_pid == BROADCAST_PID){
		int ret_count = 0;
		for(int i=0 ;i < MAX_MEMBERS; i++){
			if(mqi->members_mapping[i]!= NO_PID && mqi->members_mapping[i]!= sender_pid){

				int blocked = 0;
				
				for(int j=0 ;j < MAX_MEMBERS; j++){
					if(mqi->blocked[i][j] == sender_pid){
						blocked = 1;
						break;
					}
				}
				
				if(!blocked){
					enqueue(msg,mqi,i);
					ret_count++;
				}
			}
		}
		return ret_count;
	}else{
		int mem_idx = NO_INDEX;
		for(int i=0 ;i < MAX_MEMBERS;i++){
			if(mqi->members_mapping[i] == msg->to_pid){
				mem_idx = i;
				break;
			}
		}

		// message is addressed to a process that is not a member of the message queue
		if(mem_idx == NO_INDEX){
			return -EINVAL;
		}
		
		int blocked = 0;
		
		for(int j=0 ;j < MAX_MEMBERS; j++){
			if(mqi->blocked[mem_idx][j] == sender_pid){
				blocked = 1;
				break;
			}
		}
		
		if(!blocked){
			enqueue(msg,mqi,mem_idx);
			return 1;
		}else{
			// blocked user
			return -EINVAL;
		}

	}
	
	return -EINVAL;
}


void do_add_child_to_msg_queue(struct exec_context *child_ctx)
{
	/** 
	 * Implementation of fork handler 
	 **/
	// traverse all fds and find fds representing the msg_queue
	int fd = 3;
	if(child_ctx){
		while( fd < 16 && child_ctx->files[fd] != NULL){
			if(child_ctx->files[fd]->type == MSG_QUEUE){
				// add child to the MSG_QUEUE
				struct msg_queue_info* mqi = child_ctx->files[fd]->msg_queue;
				
				if(mqi!= NULL){
					for(int i = 0; i < MAX_MEMBERS; i++){
						if(mqi->members_mapping[i] == NO_PID){
							mqi->members_mapping[i] = child_ctx->pid;	
							break;
						}
					}
					child_ctx->files[fd]->ref_count++;	
				}
			}
			fd++; 
		}
	}
}


void do_msg_queue_cleanup(struct exec_context *ctx)
{
	/** 
	 * Implementation of exit handler 
	 **/
	if(ctx){
		int my_pid = ctx->pid;
		for(int i =0 ; i < MAX_OPEN_FILES ; i++){
			if(ctx->files[i]->type == MSG_QUEUE){
				do_msg_queue_close(ctx,i);
			}
		}
	}
}


int do_msg_queue_get_member_info(struct exec_context *ctx, struct file *filep, struct msg_queue_member_info *info)
{
	/** 
	 * Implementation of exit handler 
	 **/
	if(ctx == NULL || filep == NULL){
		return -EINVAL;
	}

	struct msg_queue_info* mqi = filep->msg_queue;
	
	// if filep does not point to the message queue then -EINVAL
	if(mqi == NULL || filep->type!= MSG_QUEUE){
		return -EINVAL;
	}

	// 	populate the member count field and member_pid
	int mem_count = 0;
	for(int i=0; i< MAX_MEMBERS; i++){
		if(mqi->members_mapping[i]!= NO_PID){
			info->member_pid[mem_count] = mqi->members_mapping[i];
			mem_count++;
		}
	}
	info->member_count = mem_count;

	return 0;
}


int do_get_msg_count(struct exec_context *ctx, struct file *filep)
{
	/** 
	 * Implement functionality to
	 * return pending message count to calling process
	 **/
	if(ctx == NULL || filep == NULL){
		return -EINVAL;
	}

	struct msg_queue_info* mqi = filep->msg_queue;
	
	// if filep does not point to the message queue then -EINVAL
	if(mqi == NULL || filep->type!= MSG_QUEUE){
		return -EINVAL;
	}
	
	int my_pid = ctx->pid;
	int mem_idx = NO_INDEX;
	for(int i=0; i< MAX_MEMBERS; i++){
		if(mqi->members_mapping[i] == my_pid){
			mem_idx = i;
			break;
		}
	}
	
	// if the process is not the member of the message queue 
	if(mem_idx == NO_INDEX){
		return -EINVAL;
	}
	
	if(mqi->front[mem_idx] == -1){
		return 0;
	}
	int msg_count = 0;
	if(mqi->last[mem_idx] >= mqi->front[mem_idx]){
		msg_count = mqi->last[mem_idx] - mqi->front[mem_idx] + 1;
	}else{
		msg_count += MAX_MSG - mqi->front[mem_idx];
		msg_count += mqi->last[mem_idx] + 1;
	}

	return msg_count;
}


int do_msg_queue_block(struct exec_context *ctx, struct file *filep, int pid)
{
	/** 
	 * Implement functionality to
	 * block messages from another process 
	 **/
	if(ctx == NULL || filep == NULL){
		return -EINVAL;
	}

	if( pid < 0){
		return -EINVAL;
	}

	struct msg_queue_info* mqi = filep->msg_queue;
	
	// if filep does not point to the message queue then -EINVAL
	if(mqi == NULL || filep->type!= MSG_QUEUE){
		return -EINVAL;
	}

	int my_pid = ctx->pid;
	int mem_idx = NO_INDEX;
	for(int i =0 ;i < MAX_MEMBERS ;i++){
		if(mqi->members_mapping[i] == my_pid){
			mem_idx = i;
			break;
		}
	}

	if(mem_idx == NO_INDEX){
		return -EINVAL; 
	}

	for(int i =0; i < MAX_MEMBERS ; i++){
		if(mqi->blocked[mem_idx][i] == NO_PID){
			mqi->blocked[mem_idx][i] = pid;
			break;
		}
	}
	
	return 0;
}


int do_msg_queue_close(struct exec_context *ctx, int fd)
{
	/** 
	 * Implement functionality to
	 * remove the calling process from the message queue 
	 **/
	if(ctx == NULL){
		return -EINVAL;
	}

	struct file* filep = ctx->files[fd];
	if(filep == NULL){
		return -EINVAL;
	}

	struct msg_queue_info* mqi = filep->msg_queue;
	
	// if filep does not point to the message queue then -EINVAL
	if(mqi == NULL || filep->type!= MSG_QUEUE ){
		return -EINVAL;
	}

	int my_pid = ctx->pid;
	int mem_idx = NO_INDEX;
	
	for(int i =0 ;i < MAX_MEMBERS ;i++){
		if(mqi->members_mapping[i] == my_pid){
			mem_idx = i;
			break;
		}
	}

	if(mem_idx == NO_INDEX){
		return -EINVAL; 
	}

	mqi->members_mapping[mem_idx] = NO_PID;
	mqi->front[mem_idx] = -1;
	mqi->last[mem_idx] = -1;
	for(int i =0; i < MAX_MEMBERS ;i++){
		for(int j =0 ; j< MAX_MEMBERS ;j++){
			if(i == mem_idx){
				mqi->blocked[i][j] = NO_PID;	
			}else if(mqi->blocked[i][j] == my_pid){
				mqi->blocked[i][j] = NO_PID;	
			}
		}
	}

	int mem_count = 0;
	for(int i =0 ;i < MAX_MEMBERS ;i++){
		if(mqi->members_mapping[i]!= NO_PID){
			mem_count++;
		}
	}

	filep->ref_count--;
	if(mem_count == 0){
		free_msg_queue_buffer(mqi->msg_buffer);
		free_msg_queue_info(mqi);
		filep->msg_queue = NULL;
		free_file_object(filep);
		filep = NULL;
	}

	return 0;
}
