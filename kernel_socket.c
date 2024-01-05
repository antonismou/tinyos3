#include "kernel_socket.h"
#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_pipe.h"
#include "kernel_cc.h"


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
	return NOFILE;
}


int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{
	if (sock < 0 || sock > MAX_FILEID || 	//check if socked is valid
	port < 0 || port > MAX_PORT ||			//check if port is valid
	PORT_MAP[port] == NULL || 
	PORT_MAP[port]->type !=SOCKET_LISTENER)//check if port have listener
	{
		return -1;
	}

	FCB* fcb_socket = get_fcb(sock);

	SOCKET_CB* socket = fcb_socket->streamobj;
	SOCKET_CB* listener = PORT_MAP[port];

	CONNECTION_REQUEST* request = (CONNECTION_REQUEST*)xmalloc(sizeof(CONNECTION_REQUEST));

	//initialize request
	request->admitted = 0;
	request->peer = socket;
	request->connected_cv = COND_INIT;
	rlnode_init(&request->queue_node, request);

	//add request to the listener's request queue
	rlist_push_back(&listener->listener_s.queue, &request->queue_node);

	//signal the listener
	kernel_signal(&listener->listener_s.req_available);
	
	listener->refcount++;

	while (!request->admitted) {
		int retWait = kernel_timedwait(&request->connected_cv, SCHED_IO, timeout);
		
		//request timeout
		if(!retWait){
			return -1;
		}
	}

	listener->refcount--;
	
	return 0;
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
