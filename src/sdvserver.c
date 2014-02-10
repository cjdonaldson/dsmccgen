

#include "dsmcc.h"
#include "sdvserver.h"

/* variables are declared globally as multiple functions use them */
#define POLLTIMEOUT 0          /* x msec */
#define POLLFLAGS POLLIN


gboolean
set_ip( server_t *svrptr, gchar *ipptr )
{
    gint len;
    
    len = strlen( ipptr );
    svrptr->svmip = malloc( len + 1 );
    
    if ( svrptr->svmip == 0 )
    {
        perror( "set_ip malloc" );
        return FALSE;
    }
            
    memcpy( svrptr->svmip, ipptr, len );
        
    svrptr->sockfd = -1;
    svrptr->socket_initd = FALSE;
        
    return TRUE;
}


gboolean
init_channel( server_t *svrptr )
{
    struct hostent *host;
    int yes;

    yes = 1;

    if ( svrptr->socket_initd )
        return TRUE;

    g_printf( "initing server: %s \n", svrptr->svmip );

    host = gethostbyname( svrptr->svmip );
    if ( !host )
    {
        perror( "init_channel gethostbyname" );
        return FALSE;
    }

    svrptr->sockfd = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( svrptr->sockfd == -1 )
    {
        perror( "init_channel socket" );
        return FALSE;
    }

    // lose the pesky "Address already in use" error message
    if ( setsockopt( svrptr->sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof( int ) ) == -1)
    {
        perror("init_channel setsockopt");
        close_channel( svrptr );
        return FALSE;
    }

    svrptr->src_addr.sin_family = AF_INET;            /* host byte order */
    svrptr->src_addr.sin_port = htons( MYPORT );      /* short, network byte order */
    svrptr->src_addr.sin_addr.s_addr = inet_addr( MYADDR );
    memset( svrptr->src_addr.sin_zero, '\0', sizeof svrptr->src_addr.sin_zero );

    svrptr->dst_addr.sin_family = AF_INET;            /* host byte order */
    svrptr->dst_addr.sin_port = htons( DSTPORT );      /* short, network byte order */
    memcpy( &svrptr->dst_addr.sin_addr.s_addr, host->h_addr, host->h_length );
    memset( svrptr->dst_addr.sin_zero, '\0', sizeof svrptr->dst_addr.sin_zero );

    /* bind() this comm link to a specific out going port */
    /* SVM reports back only on this port */
    if ( bind( svrptr->sockfd, (struct sockaddr *)&svrptr->src_addr, sizeof svrptr->src_addr ) == -1 )
    {
        perror( "init_channel bind" );
        close_channel( svrptr );
        return FALSE;
    }

    /* define the other side of the comm link */
    if ( connect( svrptr->sockfd, (struct sockaddr *)&svrptr->dst_addr, sizeof svrptr->dst_addr ) == -1 )
    {
        perror( "init_channel connect" );
        return -1;
    }

    /* we are done with the init */
    svrptr->socket_initd = TRUE;
    g_printf( "inited server: %s    fd: %i\n", svrptr->svmip, svrptr->sockfd );

    return TRUE;
}

gint
send_data( server_t *svrptr, gchar *buffptr, int buff_len )
{
    #define TXFLAGS 0
    gint bytes_sent;
    
    bytes_sent = sendto( svrptr->sockfd, buffptr, buff_len, TXFLAGS,
                   (struct sockaddr *)&svrptr->dst_addr, sizeof svrptr->dst_addr );

    if ( bytes_sent == -1)
    {
        perror("send");
        return 0;
    }
    else if ( bytes_sent != buff_len )
        puts( "partial tx\n" );
        
    return bytes_sent;
}

gboolean
is_data( server_t *svrptr )
{
/*  rv > 0 if data available
 *  rv = 0 if no data available
 *  rv < 0 if error
 */
    /* we only have 1 fd */
    struct pollfd ufds[1];
    gint rv;

    if ( !init_channel( svrptr ) )
    {
        g_printf( "dsmcc_init failed\n" );
        return FALSE;
    }

    ufds[0].fd = svrptr->sockfd;
    ufds[0].events = POLLFLAGS;

    rv = poll( ufds, 1, POLLTIMEOUT );

    if ( rv == -1 )
        perror( "is_data" ); /* error occurred in poll() */

    return ( rv > 0 );
}


gint
peek_data( server_t *svrptr, gchar *buffptr, int buff_len )
{
/*  return > 0 if data available
 *  return = 0 if no data available
 *  return < 0 if error
 */
    gint i;
    gint msglen;

    i = sizeof svrptr->dst_addr;
    msglen = recvfrom( svrptr->sockfd, buffptr, buff_len, MSG_PEEK,
              (struct sockaddr *)&svrptr->dst_addr, &i );

    if ( msglen == -1 )
        perror( "peek_data" );

    return msglen;
}

gint
recv_data( server_t *svrptr, gchar *buffptr, int buff_len )
{
/*  return > 0 if data available
 *  return = 0 if no data available
 *  return < 0 if error
 */
    gint msglen;
    gint i;

    i = sizeof svrptr->dst_addr;

    msglen = recvfrom( svrptr->sockfd, buffptr, buff_len, 0,
              (struct sockaddr *)&svrptr->dst_addr, &i );

    if ( msglen == -1 )
        perror( "recv_data" );

    return msglen;
}

void
close_channel( server_t *svrptr )
{
    if ( svrptr->socket_initd )
    {
        g_printf( "closing server: %s\n",svrptr->svmip );
        close( &svrptr->sockfd );
        svrptr->sockfd = 0;
        svrptr->socket_initd = FALSE;

        free( svrptr->svmip );
    }
}
