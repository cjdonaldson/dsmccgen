#ifndef __SDVSERVER_H__
#define __SDVSERVER_H__

#include "glib-min.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <stdio.h>

struct st_server {
    gboolean socket_initd;
    gint   sockfd;
    gchar*  svmip;
    struct sockaddr_in src_addr;
    struct sockaddr_in dst_addr;
};

typedef struct st_server server_t;

gboolean set_ip( server_t* svrptr, gchar* ipptr );

gboolean init_channel( struct st_server* svrptr );

gboolean is_data( struct st_server* svrptr );

gint send_data( struct st_server* svrptr, gchar* buffptr, int buff_len );
gint peek_data( struct st_server* svrptr, gchar* buffptr, int buff_len );
gint recv_data( struct st_server* svrptr, gchar* buffptr, int buff_len );

void close_channel( struct st_server* svrptr );

#endif
