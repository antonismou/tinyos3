#include "kernel_socket.h"
#include "tinyos.h"
#include "kernel_streams.h"
#include "kernel_pipe.h"


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
	return -1;
}


int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	return -1;
}

int socket_read(){
	return -1;
}

int socket_write(){
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


