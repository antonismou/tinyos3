#include "tinyos.h"
#include "kernel_dev.h"

typedef enum{
    SOCKET_LISTENER,
    SOCKET_UNBOUND,
    SOCKET_PEER
}Socket_type;

typedef struct listener_socket
{
    rlnode queue;
    CondVar req_available;
}listener_socket;

typedef struct unbound_socket
{
    rlnode unbound_socket;
}unbound_socket;

typedef struct peer_socket
{
    SOCKET_CB* peer;
    PIPE_CB* write_pipe;
    PIPE_CB* read_pipe;
}peer_socket;

typedef struct socket_control_block
{
    uint refcount;
    FCB* fcb;
    Socket_type type;
    port_t port;

    union
    {
        listener_socket listener_s;
        unbound_socket unbound_s;
        peer_socket peer_s;
    };
    

}SOCKET_CB;