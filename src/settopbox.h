#ifndef __SETTOPBOX_H__
#define __SETTOPBOX_H__

#include "glib-min.h"
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/time.h>

#include "sdvserver.h"
#include "dsmcc.h"

typedef enum {
    NOINITFLG      = 0x0001,
    LOOPFLG        = 0x0002,
    RETUNEFAILURES = 0x0004,
    STBDUMP        = 0x0080,

    IGNOREERROR    = 0x0200,
    VERBOSEFAIL    = 0x0400,
    VERBOSEERROR   = 0x0800,
    VERBOSEIN      = 0x1000,
    VERBOSEOUT     = 0x2000,
    DBGFSMABN      = 0x4000,
    DBGFSMFULL     = 0x8000
} STB_FLAGS;

//#define STBISSTUCK 20

/* SDV servers should respond with in 3 seconds */
enum {
    TIMEOUTPERIOD_SEC  = 3, /* seconds */
    TIMEOUTPERIOD_USEC = 0, /* micro seconds */

    TUNEFAILUREMAX     = 3, /* # of server timeouts before stb is failed */

    SECOND_UTIME       = 1000000,    /* 1000000 usec in 1 sec */
    NEVEREXPIRE        = 0xFFFFFFFF  /* end of time */
};

enum e_state {
    e_state_next,
    e_state_wait,
    e_state_tx,
    e_state_done
};

typedef enum e_state estate;

struct st_settop_box {
    estate    state;
    estate    prevstate;
    STB_FLAGS flags;
    struct    timeval dwell_time_period;
    struct    timeval dwell_time;
    struct    timeval time_out;
    guint     tunefailcnt;

    /* stb objects */
    guint8    macaddr[10];         /* mac addr 6 + tuner# 1 + reseved 3 */
    guint     sourceId;
    guint     frequency;
    guint     modulation;
    guint     mpegnumber;

    /* SDV objects */
    server_t*         srvrptr;
    guint             servicegroup;
    guint32           transxId;
    DSMCC_MSGID_SDV   msgId;
    DSMCC_MSGID_SDV   prevmsgId;

    guint             dsmcc_len;
    dsmcc_t           dsmcc;
};

typedef struct st_settop_box stb_t;

gchar* sessionId_to_string( guint8* sessionId );

void stb_init( stb_t* stbptr, guint sgnumber, server_t* srvrptr,
               guint stb_base, guint stb_number, guint flags, struct timeval dwell_time_period );

void encode_macaddr( guint8* macaddr, guint stb_base, guint stb_number );
void decode_macaddr( guint8* macaddr, guint* stb_base, guint* stb_number );

gboolean stb_run( stb_t* stbptr,
                  gint* srcidptr, gint srcid_min, gint srcid_max,
                  gboolean b_txgatedmsg );


gboolean stb_dsmcc_out( stb_t* stbptr );
gboolean stb_dsmcc_in( stb_t* stbptr );

void stb_FSM( stb_t* stbptr, gint* srcidptr, gint srcid_min, gint srcid_max );

void print_stb( stb_t* stbptr );
void dbg_print_stb( gchar* str, stb_t* stbptr );

gint stbcmp( gchar* macptr1, gchar* macptr2 );

#endif
