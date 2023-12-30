#include "util.h"
#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_dev.h"
#include "kernel_pipe.h"
#include "kernel_sched.h"

file_ops pipe_writer = {
	.Read = (void*)pipe_error,
	.Write = pipe_write,
	.Close = pipe_writer_close
};

file_ops pipe_reader = {
	.Read = pipe_read,
	.Write = (void*)pipe_error,
	.Close = pipe_reader_close
};

int sys_Pipe(pipe_t* pipe)
{
	
	Fid_t fid[2];
	FCB* fcb[2];
	if(FCB_reserve(2,fid,fcb) == 0){
    	return -1;
	}	
	PIPE_CB* pipe_cb = (PIPE_CB*) malloc(sizeof(PIPE_CB));

	pipe->read = fid[0];
	pipe->write = fid[1];

	/*Initialize Pipe control block*/
	pipe_cb->reader = fcb[0];
	pipe_cb->writer = fcb[1];
	pipe_cb->is_empty = COND_INIT;
	pipe_cb->is_full = COND_INIT;
	pipe_cb->r_position = 0;
	pipe_cb->w_position = 0;
	pipe_cb->empty_space = PIPE_BUFFER_SIZE;

	fcb[0]->streamobj = pipe_cb;
	fcb[1]->streamobj = pipe_cb;
	
	fcb[0]->streamfunc = &pipe_reader;
	fcb[1]->streamfunc = &pipe_writer;

	return 0;

}

int pipe_reader_close(void* pipe_cb){
	if(pipe_cb == NULL){
		return -1;
	}
	
	PIPE_CB* pipe = (PIPE_CB*) pipe_cb;

	pipe->reader = NULL;

	if (pipe->empty_space == PIPE_BUFFER_SIZE && pipe->writer == NULL) {
		free(pipe->reader);
		free(pipe->writer);
		free(pipe);
	}

	return 0;
}

int pipe_writer_close(void* pipe_cb){
	if (pipe_cb == NULL){
		return -1;
	}
	PIPE_CB* pipe = (PIPE_CB*) pipe_cb;


	pipe->writer = NULL;

	if (pipe->reader == NULL) {
		free(pipe->writer);
		free(pipe->reader);
		free(pipe);
	}


	return 0;
}

int pipe_read(){
	return -1;
}

int pipe_write(void* write_index,const char* buf, unsigned int size){
	PIPE_CB* pipe = (PIPE_CB*) write_index;

	/*
		checks if the pipe is closed or the writer or reader is closed
		if one of them is closed returns an error
	*/
	if(pipe==NULL||pipe->writer == NULL||pipe->reader==NULL){
		return -1;
	}

	unsigned int free_space = pipe->empty_space;
	
	/*
		In this while loop as long as the buffer is full and the reader is open
		we signal the readers to read some data before we can write again
	*/
	 while(free_space == 0 && pipe->reader!=NULL){
		kernel_wait(&(pipe->is_full), SCHED_PIPE);

		/*
			After the reader reads some data the  w_position and r_positiot change
			so we get a new value for free_space
		*/

		free_space = pipe->empty_space;

	 }

	 /*
	 	if the buffer has empty space let k be this space 
		else if the buffer is empty let k equal the maximum buffer size 
	 */
	 unsigned int j=0;
	 if(size>free_space){
		j = free_space;
	 } else{
		j = size;
	 }

	 //Write opperation

	 for(int i=0; i<j; i++){
		pipe->buffer[pipe->w_position]= buf[i];
		pipe->w_position = (pipe->w_position+1)%j;
		pipe->empty_space--;
	 }


	kernel_broadcast(&pipe->is_full);



	return j;
}

/*This is used for when one reader try to write or when a writer try to read*/
int pipe_error() 
{
	return -1;
}
