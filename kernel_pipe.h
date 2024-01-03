#include "tinyos.h"
#include "kernel_dev.h"
#define PIPE_BUFFER_SIZE 8192



typedef struct pipe_control_block
{
  FCB *reader, *writer;

  CondVar is_full; //for blocking writer if no space is availabe
  CondVar is_empty; //for blocking reader until data are available

  int w_position, r_position; //write, read position in buffer
  int empty_space; //counter to count the empty space of the buffer

  char buffer[PIPE_BUFFER_SIZE]; //bounded cyclic byte buffer

}PIPE_CB;

int pipe_reader_close(void* pipe_cb);
int pipe_writer_close(void* pipe_cb);
int pipe_read();
int pipe_write(void* index,const char* buf, unsigned int size);
int pipe_error();