#include "kernel_socket.h"
#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_pipe.h"
#include "kernel_sched.h"
#include "kernel_proc.h"


file_ops socket_file_ops = {
	.Read = socket_read,
	.Write = socket_write,
	.Close = socket_close
};

Fid_t sys_Socket(port_t port)
{
	if(port < 0 || port > MAX_PORT){
		return NOFILE;
	}

	Fid_t fid;
	FCB* fcb;
	if(FCB_reserve(1,&fid,&fcb) == 0){
    	return NOFILE;
	}	
	SOCKET_CB* socket_cb = (SOCKET_CB*) malloc(sizeof(SOCKET_CB));

	fcb->streamfunc = &socket_file_ops;
	fcb->streamobj = socket_cb;

	socket_cb->fcb = fcb;
	socket_cb->refcount = 0;
	socket_cb->port = port;
	socket_cb->type = SOCKET_UNBOUND;

	return fid;
}

int sys_Listen(Fid_t sock)
{
	return -1;
}


Fid_t sys_Accept(Fid_t lsock)
{
	// DO ALL THE INITIAL CHECKS NEEDED:

	/* Check if the file id is not legal and return error */
	if(lsock<0 || lsock>=MAX_FILEID){
		return NOFILE;
	}

	/* Check if the file id is not initialized by Listen() and return error */
	FCB* fcb = get_fcb(lsock); 
	
	/* no need to check if fcb==NULL because 
	   that happens only if fid is legal
	   which we already checked */

	if(fcb->streamobj==NULL || fcb->streamfunc!=&socket_file_ops){
		return NOFILE;
	}

	SOCKET_CB* socket1 = fcb->streamobj;

	if(socket1->type!= SOCKET_LISTENER){
		return NOFILE;
	} 

		// thelei elegxo gia ta ports san tin connect?? 

	/* The available file ids for the process are exhausted and return error */
	PCB* cur = CURPROC;

		// iterate or FCB_RESERVE?

	int fidFoundFlag=0;
    for(int i=0; i<MAX_FILEID; i++) {
		if(cur->FIDT[i]==NULL){
			fidFoundFlag=1;
			break;
		}    	
    }

	if(fidFoundFlag==0){
		return NOFILE;
	}


	// MAIN CODE:

	/* Increase refcount */
	socket1->refcount++;

	/* Wait for a request */
	


	return NOFILE;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	return -1;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	return -1;
}

int socket_close(void* socket){
	if(socket == NULL){
		return -1;
	}

	SOCKET_CB* socket_cb = (SOCKET_CB*) socket;

	if(socket_cb->type == SOCKET_LISTENER){

	}else if (socket_cb->type == SOCKET_PEER){
		if(!(pipe_reader_close(socket_cb->peer_s.read_pipe) || pipe_writer_close(socket_cb->peer_s.write_pipe))){
			return -1;
		}
		socket_cb->peer_s.peer = NULL;
	}
	socket_cb->refcount--;



	return 0;
}


int socket_read(void* socketcb_t, char *buf, unsigned int n)
{
	SOCKET_CB* socket = (SOCKET_CB*) socketcb_t;

	/* Check if the socket or the destination buffer is NULL.
       If any of them is NULL, return an error (-1) */
	if(socket==NULL || buf==NULL){
		return -1;
	}

	/* Check if the socket type is SOCKET_PEER, and if the peer and read_pipe are not NULL.
       If any of these conditions is false, return an error (-1) */
	if (socket->type != SOCKET_PEER || socket->peer_s.peer == NULL || socket->peer_s.read_pipe == NULL){
		return -1;
	}

	/* Call the pipe_read function to perform the read operation */
	int bytesRead = pipe_read(socket->peer_s.read_pipe, buf, n);

	return bytesRead; // Return the result of the pipe_read operation
}

int socket_write(void* socketcb_t, const char *buf, unsigned int n)
{
	SOCKET_CB* socket = (SOCKET_CB*) socketcb_t;

	/* Check if the socket or the destination buffer is NULL.
       If any of them is NULL, return an error (-1) */
	if(socket==NULL || buf==NULL){
		return -1;
	}

	/* Check if the socket type is SOCKET_PEER, and if the peer and write_pipe are not NULL.
       If any of these conditions is false, return an error (-1) */
	if (socket->type != SOCKET_PEER || socket->peer_s.peer == NULL || socket->peer_s.write_pipe == NULL){
		return -1;
	}

	/* Call the pipe_write function to perform the write operation */
	int bytesWritten = pipe_write(socket->peer_s.write_pipe, buf, n);

	return bytesWritten; // Return the result of the pipe_write operation
}
