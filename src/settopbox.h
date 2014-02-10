#ifndef __SETTOPBOX_H__
#define __SETTOPBOX_H__

#include <glib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>

#include "sdvserver.h"
#include "dsmcc.h"


#define NOINITFLG      0x0001
#define LOOPFLG        0x0002
#define RETUNEFAILURES 0x0004
#define STBDUMP        0x0080

#define IGNOREERROR    0x0200
#define VERBOSEFAIL    0x0400
#define VERBOSEERROR   0x0800
#define VERBOSEIN      0x1000
#define VERBOSEOUT     0x2000
#define DBGFSMABN      0x4000
#define DBGFSMFULL     0x8000


//#define STBISSTUCK 20

/* SDV servers should respond with in 3 seconds */
#define TIMEOUTPERIOD_SEC  3 /* seconds */
#define TIMEOUTPERIOD_USEC 0 /* micro seconds */

#define TUNEFAILUREMAX     3 /* # of server timeouts before stb is failed */

#define SECOND_UTIME 1000000    /* 1000000 usec in 1 sec */
#define NEVEREXPIRE 0xFFFFFFFF  /* end of time */


enum e_state
{
    e_state_next,
    e_state_wait,
    e_state_tx,
    e_state_done
};

typedef enum e_state estate;

struct st_settop_box
{
    estate  state, prevstate;
    guint   flags;
    struct timeval dwell_time_period;
    struct timeval dwell_time;
    struct timeval time_out;
    guint  tunefailcnt;
    
    /* stb objects */
    guint8  macaddr[10];         /* mac addr 6 + tuner# 1 + reseved 3 */
    guint   sourceId;
    guint   frequency;
    guint   modulation;
    guint   mpegnumber;
    
    /* SDV objects */
    server_t *srvrptr;
    guint    servicegroup;
    guint32  transxId;
    guint32  msgId, prevmsgId;
    
    guint    dsmcc_len;
    dsmcc_t  dsmcc;
};

typedef struct st_settop_box stb_t;

gchar* sessionId_to_string( guint8 *sessionId );

void stb_init( stb_t *stbptr, guint sgnumber, server_t *srvrptr,
       guint stb_base, guint stb_number, guint flags, struct timeval dwell_time_period );

gboolean stb_run( stb_t *stbptr,
                  gint *srcidptr, gint srcid_min, gint srcid_max,
                  gboolean b_txgatedmsg );


gboolean stb_dsmcc_out( stb_t *stbptr );
gboolean stb_dsmcc_in( stb_t *stbptr );

void stb_FSM( stb_t *stbptr, gint *srcidptr, gint srcid_min, gint srcid_max );

void print_stb( stb_t *stbptr );
void dbg_print_stb( gchar *str, stb_t *stbptr );

gint stbcmp( gchar *macptr1, gchar *macptr2 );

#endif
