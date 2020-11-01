#include<types.h>
#include<context.h>
#include<file.h>
#include<lib.h>
#include<serial.h>
#include<entry.h>
#include<memory.h>
#include<fs.h>
#include<kbd.h>


/************************************************************************************/
/***************************Do Not Modify below Functions****************************/
/************************************************************************************/

void free_file_object(struct file *filep)
{
	if(filep)
	{
		os_page_free(OS_DS_REG ,filep);
		stats->file_objects--;
	}
}

struct file *alloc_file()
{
	struct file *file = (struct file *) os_page_alloc(OS_DS_REG); 
	file->fops = (struct fileops *) (file + sizeof(struct file)); 
	bzero((char *)file->fops, sizeof(struct fileops));
	file->ref_count = 1;
	file->offp = 0;
	stats->file_objects++;
	return file; 
}

void *alloc_memory_buffer()
{
	return os_page_alloc(OS_DS_REG); 
}

void free_memory_buffer(void *ptr)
{
	os_page_free(OS_DS_REG, ptr);
}

/* STDIN,STDOUT and STDERR Handlers */

/* read call corresponding to stdin */

static int do_read_kbd(struct file* filep, char * buff, u32 count)
{
	kbd_read(buff);
	return 1;
}

/* write call corresponding to stdout */

static int do_write_console(struct file* filep, char * buff, u32 count)
{
	struct exec_context *current = get_current_ctx();
	return do_write(current, (u64)buff, (u64)count);
}

long std_close(struct file *filep)
{
	filep->ref_count--;
	if(!filep->ref_count)
		free_file_object(filep);
	return 0;
}

struct file *create_standard_IO(int type)
{
	struct file *filep = alloc_file();
	filep->type = type;
	if(type == STDIN)
		filep->mode = O_READ;
	else
		filep->mode = O_WRITE;
	if(type == STDIN){
		filep->fops->read = do_read_kbd;
	}else{
		filep->fops->write = do_write_console;
	}
	filep->fops->close = std_close;
	return filep;
}

int open_standard_IO(struct exec_context *ctx, int type)
{
	int fd = type;
	struct file *filep = ctx->files[type];
	if(!filep){
		filep = create_standard_IO(type);
	}else{
		filep->ref_count++;
		fd = 3;
		while(ctx->files[fd])
			fd++; 
	}
	ctx->files[fd] = filep;
	return fd;
}
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/

/* File exit handler */
void do_file_exit(struct exec_context *ctx)
{
	/*the process is exiting. Adjust the refcount
	of files*/
	if(ctx){
		for(int fd = 0 ; fd < 16 ; fd++){  // TODO: Should we close the stdin,out,err also?
			if(ctx->files[fd]){
				ctx->files[fd]->ref_count--;
				if(!ctx->files[fd]->ref_count){
					free_file_object(ctx->files[fd]);
				}
			}
		}
	}
	
}

/*Regular file handlers to be written as part of the assignmemnt*/


static int do_read_regular(struct file *filep, char * buff, u32 count)
{
	/** 
	*  Implementation of File Read, 
	*  You should be reading the content from File using file system read function call and fill the buf
	*  Validate the permission, file existence, Max length etc
	*  Incase of Error return valid Error code 
	**/

	if(filep == NULL){
		return -EINVAL;
	}

	struct inode* curr_inode = filep->inode;
	
	// file existence and buff existence check
	if(curr_inode == NULL || buff == NULL){
		return -EINVAL;
	}
	
	// read permission check
	if(curr_inode->mode & O_READ != O_READ){
		return -EACCES;
	}	
	
	int bytes_read = flat_read(curr_inode, buff, count, (int*)&(filep->offp));
	if(bytes_read >0)
		filep->offp += bytes_read;
	
	return bytes_read;
}


/*write call corresponding to regular file */

static int do_write_regular(struct file *filep, char * buff, u32 count)
{
	/** 
	*   Implementation of File write, 
	*   You should be writing the content from buff to File by using File system write function
	*   Validate the permission, file existence, Max length etc
	*   Incase of Error return valid Error code 
	* */ 
 	
	if(filep == NULL){
		return -EINVAL;
	}

	struct inode* curr_inode = filep->inode;
	
	// file existence and buff existence check
	if(curr_inode == NULL || buff == NULL){
		return -EINVAL; 
	}
	
	// write permission check
	if(curr_inode->mode & O_WRITE != O_WRITE){
		return -EACCES;
	}

	int bytes_written = flat_write(curr_inode, buff, count, (int*)&(filep->offp));
	if(bytes_written >0)
		filep->offp += bytes_written;
	
	return bytes_written;
}


long do_file_close(struct file *filep)
{
	/** Implementation of file close  
	*   Adjust the ref_count, free file object if needed
	*   Incase of Error return valid Error code 
	*/

	if(filep == NULL){
		return -EINVAL;
	}

	if(filep->ref_count <= 0){
		return -EINVAL;
	}
	
	filep->ref_count--;
	if(filep->ref_count == 0){ 
		free_file_object(filep);
	}
	
	return 0;
}


static long do_lseek_regular(struct file *filep, long offset, int whence)
{
	/** 
	*   Implementation of lseek 
	*   Set, Adjust the ofset based on the whence
	*   Incase of Error return valid Error code 
	* */
	
	if(filep == NULL){
		return -EINVAL;
	}
	
	long prev_offset = filep->offp;

	switch(whence){
		case SEEK_SET:
				filep->offp = offset;
				break;
		case SEEK_CUR:
				filep->offp = filep->offp + offset; 
				break;
		case SEEK_END:
				filep->offp = filep->inode->file_size + offset;
				break;
		default:	
				return -EINVAL;
				break;		
	}
	
	if( filep->offp > filep->inode->file_size || filep->offp < 0){
		filep->offp = prev_offset;
		return -EINVAL;
	}

	return filep->offp;
}


extern int do_regular_file_open(struct exec_context *ctx, char* filename, u64 flags, u64 mode)
{

	/**  
	*  Implementation of file open, 
	*  You should be creating file(use the alloc_file function to creat file), 
	*  To create or Get inode use File system function calls, 
	*  Handle mode and flags 
	*  Validate file existence, Max File count is 16, Max Size is 4KB, etc
	*  Incase of Error return valid Error code 
	* */ 
	if(ctx == NULL || filename == NULL){
		return -EINVAL;
	}


	struct inode* curr_inode = lookup_inode(filename);
	
	if(curr_inode == NULL){
		/**** file does not exist ****/

		if(flags & O_CREAT){

			// check for file permissions given in mode || are they consistent with flag?
			if(flags == O_CREAT){
				return -EINVAL;
			}

			if(flags & O_READ) {
				if(!(mode & O_READ)){
					return -EINVAL;
				}	
			}

			if(flags & O_WRITE) {
				if(!(mode & O_WRITE)){
					return -EINVAL;
				}
			}
		
			// create the inode for the file with required permissions
			curr_inode = create_inode(filename,mode);
			if(curr_inode == NULL){
				return -ENOMEM;
			}

			// create the file object
			struct file* filep = alloc_file();
			if(filep == NULL){
				return -ENOMEM;
			}
	
			// initialise the file struct elements
			filep->type = REGULAR;
			filep->mode = flags & (~O_CREAT);
			filep->fops->read = do_read_regular;
			filep->fops->write = do_write_regular;
			filep->fops->lseek = do_lseek_regular;
			filep->fops->close = do_file_close;
			filep->pipe = NULL;
			filep->msg_queue = NULL;
			filep->inode = curr_inode;
			
			// assign the fd to the file
			int fd = 3;
			while( ctx->files[fd])
				fd++;
			
			if(fd < MAX_OPEN_FILES) ctx->files[fd] = filep;
			else return -ENOMEM;
			
			return fd;
		}
	}else{   
		/**** file exists ****/
		
		// file permission access check
		if((curr_inode->mode) & flags != flags){
			return -EACCES;
		}
		
		// file size check
		if(curr_inode->file_size > FILE_SIZE){
			return -ENOMEM;
		}

		// create the file object
		struct file* filep = alloc_file();
		if(filep == NULL){
			return -ENOMEM;
		}

		// initialise the file struct elements
		filep->type = REGULAR;
		filep->mode = flags;
		filep->pipe = NULL;
		filep->fops->read = do_read_regular;
		filep->fops->write = do_write_regular;
		filep->fops->lseek = do_lseek_regular;
		filep->fops->close = do_file_close;
		filep->msg_queue = NULL;
		filep->inode = curr_inode;
			
		// assign the fd to the file
		int fd = 3;
		while( ctx->files[fd])
			fd++;
			
		if(fd < MAX_OPEN_FILES) ctx->files[fd] = filep;
		else return -ENOMEM;
		
		return fd;		
	}
	return -EINVAL;
}


/**
 * Implementation dup 2 system call;
 */

int fd_dup2(struct exec_context *current, int oldfd, int newfd)
{
	/** 
	*  Implementation of the dup2 
	*  Incase of Error return valid Error code 
	**/
	if(current == NULL){
		return -EINVAL;
	}

	if(oldfd >= MAX_OPEN_FILES || newfd >= MAX_OPEN_FILES || oldfd <0 || newfd <0){
		return -EINVAL;
	}

	// oldfd not open
	if(!(current->files[oldfd])){
		return -EINVAL;
	}
	
	// if oldfd = newfd then do nothing
	if(oldfd == newfd){
		return newfd;
	}
	
	// close newfd if it is open
	if(current->files[newfd]){
		long ret_close = do_file_close(current->files[newfd]);
		
		// check if ret_close is any error
		if(ret_close < 0){
			return ret_close;
		}	
	}
	
	current->files[newfd] = current->files[oldfd];
	current->files[oldfd]->ref_count++;
	return newfd;	
}

int do_sendfile(struct exec_context *ctx, int outfd, int infd, long *offset, int count) {
	/** 
	*  Implementation of the sendfile 
	*  Incase of Error return valid Error code 
	**/

	if(ctx == NULL){
		return -EINVAL;
	}

	if(outfd >= MAX_OPEN_FILES || infd >= MAX_OPEN_FILES || outfd <0 || infd <0){
		return -EINVAL;
	}

	int ret_count = count;
	struct file* infile = ctx->files[infd];
	struct file* outfile = ctx->files[outfd];
	
	// check infd and outfd are opened. 
	if(!outfile || !infile){
		return -EINVAL;
	}
	
	// check infd has read access
	if(infile->mode & O_READ != O_READ)
		return -EACCES;
	
	// check outfd has write access
	if(outfile->mode & O_WRITE != O_WRITE)
		return -EACCES;
	
	long before_read_offset = infile->offp;
	long bytes_written = 0;
	char* buff = (char*)alloc_memory_buffer();
	
	if(offset != NULL){
		// infile offset changed to the value from which we need to start reading
		int ret_lseek = do_lseek_regular(infile,*offset,SEEK_SET);
		if(ret_lseek <0){
			free_memory_buffer((void*)buff);
			return ret_lseek;
		}
			
			
		while(count > 0){
			int count1 = count < 0x1000 ? count : 0x1000;
			count -= count1;
			int ret_read = do_read_regular(infile,buff,count1);
			if(ret_read < 0){
				*offset += bytes_written;
				
				//infile offset set back to the old value
				ret_lseek = do_lseek_regular(infile, before_read_offset, SEEK_SET);
				if(ret_lseek <0){
					free_memory_buffer((void*)buff);
					return ret_lseek;
				}
				free_memory_buffer((void*)buff);
				return ret_read;
			}
			
			int ret_write;
			for(int cbytes = count1; cbytes >=0 ; cbytes++){
				ret_write = do_write_regular(outfile,buff,count1);
				if(ret_write > 0){
					bytes_written += ret_write;
					break;
				}
			}

			if(ret_write < 0){
				*offset += bytes_written;
				//infile offset set back to the old value
				ret_lseek = do_lseek_regular(infile,before_read_offset,SEEK_SET);
				if(ret_lseek <0){
					free_memory_buffer((void*)buff);
					return ret_lseek;
				}
				free_memory_buffer((void*)buff);
				return ret_write;
			}else if(ret_write == 0){
				break;
			}
		}
		
		*offset += bytes_written;
		
		//infile offset set back to the old value
		ret_lseek = do_lseek_regular(infile,before_read_offset,SEEK_SET);
		if(ret_lseek <0){
			free_memory_buffer((void*)buff);
			return ret_lseek;
		}
		
	}else{
		while(count > 0){
			int count1 = count < 0x1000 ? count : 0x1000;
			count -= count1;
			int ret_read = do_read_regular(infile,buff,count1);
			if(ret_read < 0){
				// change the infile offset
				int ret_lseek = do_lseek_regular(infile,bytes_written,SEEK_CUR);
				if(ret_lseek <0){
					free_memory_buffer((void*)buff);
					return ret_lseek;
				}
				free_memory_buffer((void*)buff);
				return ret_read;
			}
				
			int ret_write;
			for(int cbytes = count1; cbytes >=0 ; cbytes++){
				ret_write = do_write_regular(outfile,buff,count1);
				if(ret_write > 0){
					bytes_written += ret_write;
					break;
				}
			}
			
			if(ret_write < 0){
				*offset += bytes_written;
				//infile offset set back to the old value
				int ret_lseek = do_lseek_regular(infile,before_read_offset,SEEK_SET);
				if(ret_lseek <0){
					free_memory_buffer((void*)buff);
					return ret_lseek;
				}
				free_memory_buffer((void*)buff);
				return ret_write;
			}else if( ret_write == 0){
				break;
			}
		}
	}
	
	// free the memory buffer allocated above
	free_memory_buffer((void*)buff);
	
	// return the number of bytes written in outfd on success
	return ret_count;
}

