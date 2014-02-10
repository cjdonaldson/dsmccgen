#ifndef __SERVICEGROUP_H__
#define __SERVICEGROUP_H__

#include <pthread.h>
#include <stdio.h>
#include "settopbox.h"

#define SLEEPRATE 2

int abort_request;

struct st_servicegroup {
    pthread_t thread;

    server_t* srvrptr;

    struct timeval next_time;

    guint     holdoff;
    guint     rate;
    guint     dwell;

    guint     servicegroup;

    guint     stbbase;
    guint     stbcnt;

    guint     flags;

    guint     srcidmin;
    guint     srcidmax;

    stb_t*    stbbegin;
    stb_t*    stbend;
};

typedef struct st_servicegroup servicegroup_t;

void* sg_run_task( void* ptr );

gboolean sg_init( servicegroup_t* sgptr, gboolean b_clear );

void sg_free( servicegroup_t* sgptr );

#endif
