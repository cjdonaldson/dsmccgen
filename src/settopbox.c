#include <sys/timeb.h>    /* for timeb to get millitime */
#include <time.h>

#include "settopbox.h"

const value_string debug_stbstate_names[] = {
    { e_state_next, "next" },
    { e_state_wait, "wait" },
    { e_state_tx,   "send" },
    { e_state_done, "done" },
    { 0, NULL }
};

/* Verbose diagnostic options
 *
 * Message                  VERBOSEIN  VERBOSEOUT VERBOSEERROR VERBOSEFAIL IGNOREERROR DBGFSMFULL DBGFSMABN
 *                                                   err!=ok
 *                             -vi        -vo        -ve          -vf                     -vs        -vx
 *
 * ..._INIT_REQUEST        |               X     |                        |           |    X               |
 * ..._INIT_CONFIRM        |    X                |    X                   |           |    X               |
 *
 * ..._SELECT_REQUEST      |               X     |                        |           |    X               |
 * ..._SELECT_CONFIRM      |    X                |    X                   |           |    X               |
 *
 * ..._SELECT_INDICATION   |    X                |    X            X      |           |    X          X    |
 * ..._SELECT_RESPONSE     |               X     |    X            X      |           |    X          X    |
 *
 * ..._ACTIVITY_REPORT     |               X     |                        |           |    X          X    |
 *
 * ..._QUERY_REQUEST       |    X                |    X                   |           |    X          X    |
 * ..._QUERY_CONFIRM       |               X     |                        |           |    X          X    |
 *
 * ..._EVENT_INDICATION    |    X                |                 X      |           |    X          X    |
 * ..._EVENT_RESPONSE      |               X     |                 X      |           |    X          X    |
 *
 * ... = DSMCC_MSGID_SDV
 *
 */



gchar vsbuff[500];
//static pthread_mutex_t stb_mutex  = PTHREAD_MUTEX_INITIALIZER;

void encode_macaddr( guint8* macaddr, guint stb_base, guint stb_number ) {
    macaddr[1]  = ( stb_base >> 8 & 0xFF );
    macaddr[2]  = ( stb_base & 0xFF );
    macaddr[4]  = ( stb_number >> 8 & 0xFF );
    macaddr[5]  = ( stb_number & 0xFF );
}

void decode_macaddr( guint8* macaddr, guint* stb_base, guint* stb_number ) {
    *stb_base   = macaddr[1] << 8 | macaddr[2];
    *stb_number = macaddr[4] << 8 | macaddr[5];
}

void
stb_init( stb_t* stbptr, guint sgnumber, server_t* srvrptr, guint stb_base, guint stb_number, guint flags, struct timeval dwell_time_period ) {
    stbptr->msgId = stbptr->prevmsgId = DSMCC_MSGID_SDV_INIT_REQUEST;
    stbptr->state = stbptr->prevstate = e_state_next;

    stbptr->flags                     = flags;

    /* mac addr 6 + tuner# 1 + reseved 3 */
    memset( stbptr->macaddr, '\0', sizeof stbptr->macaddr );
    encode_macaddr( stbptr->macaddr, stb_base, stb_number );

    stbptr->transxId          = 1;

    stbptr->srvrptr           = srvrptr;
    stbptr->servicegroup      = sgnumber;

    stbptr->dwell_time_period = dwell_time_period;

    /* nothing sent, so nothing to time out on */
    stbptr->time_out.tv_sec   = NEVEREXPIRE;
    stbptr->time_out.tv_usec  = NEVEREXPIRE;

    // do not use 0 as that will signal a signoff, this value will be overwritten with the next channel in the group
    stbptr->sourceId          = 1;

    stbptr->frequency         = 0;
    stbptr->modulation        = 0;
    stbptr->mpegnumber        = 0;

    stbptr->tunefailcnt       = 0;
}

guint
snprint_log( char* cbuffptr, int cbufflen, struct timeval* tmvalptr, stb_t* stbptr ) {
    guint i;

    i  = strftime( cbuffptr, cbufflen, "%a %d%b%Y %H:%M:%S", localtime( &tmvalptr->tv_sec ) );
    i += snprintf( cbuffptr + i, cbufflen - i, ".%03i ", tmvalptr->tv_usec / 1000 );

    return i;
}


gboolean
stb_dsmcc_out( stb_t* stbptr ) {
    dsmcc_t*      dsmccptr = &stbptr->dsmcc;

    /* static variable only because we will be here often */
    static gint    msglen;
    static gchar*  buff;
#   define TXFLAGS 0


    struct timeval tmval;

    /* get seconds and useconds for timing */
    if ( gettimeofday( &tmval, NULL ) != 0 ) {
        perror( "stb_dsmcc_out gettimeofday: " );
    }

    static char    cbuff[500];
    snprint_log( cbuff, sizeof cbuff, &tmval, stbptr );

    struct st_dsmcc_hdr* hdr = &dsmccptr->hdr;
    if ( hdr->descriminator != DSMCC_DESCRIMINATOR ) {
        /* here if first time */
        /* set the default values for root structure */
        hdr->descriminator  = DSMCC_DESCRIMINATOR;
        hdr->type           = DSMCC_TYPE_SDV;
        hdr->res            = 0xFF;
        hdr->adaptLen       = 0x00;

        /* not part of hdr but common to all payloads */
        memcpy( dsmccptr->sdb_init_request.sessId, stbptr->macaddr, sizeof stbptr->macaddr );
    }

    if ( stbptr->msgId == DSMCC_MSGID_SDV_INIT_REQUEST ) {
        /* take care of header */
        hdr->msgLen = htons( sizeof( struct st_dsmcc_sdb_init_request ) );

        /* payload for init v2.12 */
        struct st_dsmcc_sdb_init_request* sdb_init_request = &dsmccptr->sdb_init_request;
        sdb_init_request->res1           = 0xFFFF;
        sdb_init_request->serviceGroupId = htonl( stbptr->servicegroup );
        sdb_init_request->version1       = 0;
        sdb_init_request->res2           = 0xFF;
        sdb_init_request->numDesc        = 0;
        /*dsmccptr->sdb_init_request.version2      = 0; */

        if ( stbptr->flags & VERBOSEOUT ) {
            /* display some stuff */
            printf( "%s  i>: %s  %s  sg:%i\n",
                    cbuff,
                    sessionId_to_string( stbptr->macaddr ),
                    val_to_string( stbptr->msgId, dsmcc_msgid_names ),
                    stbptr->servicegroup );
        }
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_SELECT_REQUEST ) {
        /* take care of header */
        hdr->msgLen = htons( sizeof( struct st_dsmcc_sdb_select_request ) );

        /* payload for select request */
        struct st_dsmcc_sdb_select_request* sdb_select_request = &dsmccptr->sdb_select_request;

        sdb_select_request->retryCount     = 0xFF;
        sdb_select_request->res1           = 0xFF;
        sdb_select_request->sourceId       = htonl( stbptr->sourceId );
        sdb_select_request->dataLen        = htons( ( guint16 )0x0006 );
        sdb_select_request->tunerUse       = 0;
        sdb_select_request->res2           = 0xFF;
        sdb_select_request->serviceGroupId = htonl( stbptr->servicegroup );

        if ( stbptr->flags & VERBOSEOUT ) {
            /* display some stuff */
            printf( "%s  s>: %s  %s  src:%i  sg:%i\n",
                    cbuff,
                    sessionId_to_string( stbptr->macaddr ),
                    val_to_string( stbptr->msgId, dsmcc_msgid_names ),
                    stbptr->sourceId, stbptr->servicegroup );
        }
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_SELECT_RESPONSE ) {
        /* take care of header */
        hdr->msgLen = htons( sizeof( struct st_dsmcc_sdb_select_response ) );

        /* payload for select response */
        dsmccptr->sdb_select_response.response = htons( rspOk );
        dsmccptr->sdb_select_response.dataLen  = 0;  /* set to 0 */
        gint i = ntohs( dsmccptr->sdb_select_response.response );

        gboolean b_display  = ( stbptr->flags & ( VERBOSEOUT || VERBOSEFAIL ) )
                              || ( ( i > 0 ) && ( stbptr->flags & VERBOSEERROR ) );

        if ( b_display ) {
            /* display some stuff */

            printf( "%s  r>: %s  %s  %s\n",
                    cbuff,
                    sessionId_to_string( dsmccptr->sdb_init_request.sessId ),
                    val_to_string( stbptr->msgId, dsmcc_msgid_names ),
                    val_to_string( i, dsmcc_selectresponse_names ) );
        }
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_ACTIVITY_REPORT ) {
        /* take care of header */
        hdr->msgLen = htons( sizeof( struct st_dsmcc_sdb_activity_report ) );

        /* payload for select response */
        dsmccptr->sdb_activity_report.res = 0xFFFF;
        /* dsmccptr->sdb_activity_report.sourceId; do not change */
        dsmccptr->sdb_activity_report.lastUserActivity = htonl( tmval.tv_sec );

        if ( stbptr->flags & VERBOSEIN ) {
            /* display some stuff */
            printf( "%s  a<: %s %s\n",
                    cbuff,
                    sessionId_to_string( dsmccptr->sdb_init_request.sessId ),
                    val_to_string( stbptr->msgId, dsmcc_msgid_names ) );
        }
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_QUERY_CONFIRM ) {
        /* take care of header */
        hdr->msgLen = htons( sizeof( struct st_dsmcc_sdb_query_confirm ) );

        struct st_dsmcc_sdb_query_confirm* sdb_query_confirm = &dsmccptr->sdb_query_confirm;

        /* payload for query confirm */
        sdb_query_confirm->response         = htons( rspOk );
        //sdb_query_confirm->sourceId       = ; /* do not change */
        sdb_query_confirm->dataLen          = 10;
        sdb_query_confirm->tunerUse         = 0x08;
        sdb_query_confirm->res2             = 0xFF;
        sdb_query_confirm->serviceGroupId   = htonl( stbptr->servicegroup );
        sdb_query_confirm->lastUserActivity = htonl( tmval.tv_sec );

        if ( stbptr->flags & ( VERBOSEOUT | VERBOSEERROR ) ) {
            /* display some stuff */
            printf( "%s  q<: %s %s\n",
                    cbuff,
                    sessionId_to_string( dsmccptr->sdb_init_request.sessId ),
                    val_to_string( stbptr->msgId, dsmcc_msgid_names ) );
        }
    }
    else {
        /* display some stuff */
        printf( "\n\n\n\n ***>: %s  sending nothing. Unknown MsgId: %04X\n",
                sessionId_to_string( stbptr->macaddr ),
                ntohs( stbptr->msgId ) );

        printf( "    " );
        print_dsmcc( ( gchar* )dsmccptr, sizeof * dsmccptr );

        printf( "\n\n\n\n" );

        return FALSE;
    }

    /* made it here; set the common data */
    /* set the dynamic parameters of the root structure */
    hdr->msgId     = htons( stbptr->msgId );
    hdr->transxId  = htonl( ( stbptr->transxId )++ );

    /* calculate message length*/
    stbptr->dsmcc_len = sizeof( struct st_dsmcc_hdr ) + ntohs( hdr->msgLen );

    /* send the message and then wait for response */
    gint i = send_data( stbptr->srvrptr, ( gchar* ) &stbptr->dsmcc, stbptr->dsmcc_len );

    if ( i == stbptr->dsmcc_len ) {
        /* message sent in its entirety
        ** set the state to wait, set the timeout timer, set the dwell timer */
        stbptr->state = e_state_wait;

        //stbptr->time_out.tv_sec  = tmval.tv_sec + TIMEOUTPERIOD_SEC;
        //stbptr->time_out.tv_usec = tmval.tv_usec + TIMEOUTPERIOD_USEC;
        // set timeout to just past dwell time
        stbptr->time_out.tv_sec  = tmval.tv_sec + stbptr->dwell_time_period.tv_sec;
        stbptr->time_out.tv_usec = tmval.tv_usec + stbptr->dwell_time_period.tv_usec + 1000;
        while ( stbptr->time_out.tv_usec >= SECOND_UTIME ) {
            ++stbptr->time_out.tv_sec;
            stbptr->time_out.tv_usec -= SECOND_UTIME;
        }

        if ( stbptr->msgId == DSMCC_MSGID_SDV_SELECT_REQUEST ) {
            /* only dwell on channel requests */
            stbptr->dwell_time.tv_sec  = tmval.tv_sec + stbptr->dwell_time_period.tv_sec;
            stbptr->dwell_time.tv_usec = tmval.tv_usec + stbptr->dwell_time_period.tv_usec;
            while ( stbptr->dwell_time.tv_usec >= SECOND_UTIME ) {
                ++stbptr->dwell_time.tv_sec;
                stbptr->dwell_time.tv_usec -= SECOND_UTIME;
            }
        }
    }
    else {
        puts( "stb_dsmcc_out tx error\n" );
        return FALSE;
    }

    return TRUE;
} /* stb_dsmcc_out */

gint
stbcmp( gchar* macptr1, gchar* macptr2 ) {
    /* really only need look at macaddr[1:5] bytes */
    return memcmp( macptr1 + 1, macptr2 + 1, 5 );
}

gboolean
stb_dsmcc_in( stb_t* stbptr ) {
    dsmcc_t*       dsmccptr;
    guint          msglen;

    static gint    i;
    static gchar   cbuff[500];

    gboolean       b_display;

    struct timeval tmval;

    /* setup a server_t ptr to make coding readable */
    server_t*      srvrptr = stbptr->srvrptr;

    /* check socket for data; done if none */
    if ( !is_data( srvrptr ) ) {
        return FALSE;
    }

    /* check to see if it is one of our stbs */
    peek_data ( srvrptr, ( gchar* )&stbptr->dsmcc , sizeof stbptr->dsmcc );

    if ( stbcmp( stbptr->macaddr, stbptr->dsmcc.sdb_init_request.sessId ) != 0 ) {
        /* not in the group, so go no farther */
        return FALSE;
    }

    recv_data( srvrptr, ( gchar* ) &stbptr->dsmcc , sizeof stbptr->dsmcc );

    /* we received a message so stop the time out timer */
    stbptr->time_out.tv_sec  = NEVEREXPIRE;
    stbptr->time_out.tv_usec = NEVEREXPIRE;

    /* get seconds and useconds for timing */
    if ( gettimeofday( &tmval, NULL ) != 0 ) {
        perror( "stb_dsmcc_in gettimeofday: " );
    }

    snprint_log( cbuff, sizeof cbuff, &tmval, stbptr );

    dsmccptr      = &stbptr->dsmcc;
    stbptr->msgId = ntohs( dsmccptr->hdr.msgId );
    stbptr->state = e_state_next;

    if ( stbptr->msgId == DSMCC_MSGID_SDV_INIT_CONFIRM ) {
        i         = ntohs( dsmccptr->sdb_init_confirm.response );

        b_display  = ( stbptr->flags & VERBOSEIN );
        b_display |=  ( ( i > 0 ) && ( stbptr->flags & VERBOSEERROR ) );

        if ( b_display ) {
            /* display some stuff */
            if ( i == 0x0000 ) {
                printf( "%s  ", cbuff );
            }
            else {
                printf( "%sE ", cbuff );
            }

            printf( "i<: %s  %s  %s\n" ,
                    sessionId_to_string( dsmccptr->sdb_init_confirm.sessId ),
                    val_to_string( stbptr->msgId, dsmcc_msgid_names ),
                    val_to_string( i, dsmcc_selectresponse_names ) );

            /*
                    if ( msglen < hdr->msgLen )
                        printf( " << header message length too long >>" );
                    else if ( msglen > hdr->msgLen )
                        printf( " << header message length too short >>" );

                        printf( "\n" );
            */
        }
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_SELECT_CONFIRM ) {
        stbptr->sourceId   = ntohl( dsmccptr->sdb_select_confirm.sourceId );
        stbptr->frequency  = ntohl( dsmccptr->sdb_select_confirm.frequency );
        stbptr->modulation = dsmccptr->sdb_select_confirm.modulation;
        stbptr->mpegnumber = ntohs( dsmccptr->sdb_select_confirm.programNumber );
        i                  = ntohs( dsmccptr->sdb_select_confirm.response );

        b_display  = stbptr->flags & VERBOSEIN;
        b_display |=  ( ( i > 0 ) && ( stbptr->flags & VERBOSEERROR ) );

        if ( b_display ) {
            /* display some stuff */
            if ( i == 0x0000 ) {
                printf( "%s  ", cbuff );
            }
            else {
                printf( "%sE ", cbuff );
            }

            printf( "s<: %s  %s  %s  src:%i  tune:%i-%s-%i\n",
                    sessionId_to_string( dsmccptr->sdb_select_confirm.sessId ),
                    val_to_string( stbptr->msgId, dsmcc_msgid_names ),
                    val_to_string( i, dsmcc_selectresponse_names ),
                    stbptr->sourceId,
                    stbptr->frequency,
                    val_to_string( stbptr->modulation, dsmcc_modfmt_names ),
                    stbptr->mpegnumber );

            /*
                    if ( sizeof dsmccptr->sdb_select_confirm < hdr->msgLen )
                        printf( " << header message length too long >>" );
                    else if ( sizeof dsmccptr->sdb_select_confirm > hdr->msgLen )
                        printf( " << header message length too short >>" );

                    if ( 14 < dsmccptr->sdb_select_confirm.dataLen )
                        printf( " << private data length too long >>" );
                    else if ( 14 > dsmccptr->sdb_select_confirm.dataLen )
                        printf( " << private data length too short >>" );

                        printf( "\n" );
            */
        }
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_SELECT_INDICATION ) {
        stbptr->sourceId   = ntohl( dsmccptr->sdb_select_indication.sourceId );
        stbptr->frequency  = ntohl( dsmccptr->sdb_select_indication.frequency );
        stbptr->modulation = dsmccptr->sdb_select_indication.modulation;
        stbptr->mpegnumber = ntohs( dsmccptr->sdb_select_indication.programNumber );

        i                  = ntohs( dsmccptr->sdb_select_indication.reason );

        b_display          = stbptr->flags & ( VERBOSEIN || VERBOSEFAIL );
        b_display         |= ( ( i > 0 ) && ( stbptr->flags & VERBOSEERROR ) );

        if ( b_display ) {
            /* display some stuff */
            if ( i == 0x0000 ) {
                printf( "%s  ", cbuff );
            }
            else {
                printf( "%sE ", cbuff );
            }

            printf( "n<: %s  %s  %s  src:%i  tune:%i-%s-%i\n",
                    sessionId_to_string( dsmccptr->sdb_select_indication.sessId ),
                    val_to_string( stbptr->msgId, dsmcc_msgid_names ),
                    val_to_string( i, dsmcc_selectreason_names ),
                    stbptr->sourceId,
                    stbptr->frequency,
                    val_to_string( stbptr->modulation, dsmcc_modfmt_names ),
                    stbptr->mpegnumber );

            /*
                    if ( msglen < hdr->msgLen )
                        printf( " << header message length too long >>" );
                    else if ( msglen > hdr->msgLen )
                        printf( " << header message length too short >>" );

                    if ( sizeof dsmccptr->sdb_select_indication < dsmccptr->sdb_select_indication.dataLen )
                        printf( " << private data length too long >>" );
                    else if ( sizeof dsmccptr->sdb_select_indication > dsmccptr->sdb_select_indication.dataLen )
                        printf( " << private data length too short >>" );

                    printf( "\n" );
            */
        }
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_QUERY_REQUEST ) {
        if ( stbptr->flags & VERBOSEIN ) {
            /* display some stuff */
            printf( "%s  q<: %s %s  query request\n", cbuff,
                    val_to_string( stbptr->msgId, dsmcc_msgid_names ),
                    sessionId_to_string( dsmccptr->sdb_query_request.sessId ) );
        }
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_EVENT_INDICATION ) {
        if ( stbptr->flags & ( VERBOSEIN | VERBOSEFAIL ) ) {
            /* display some stuff */
            printf( "%s  e<: %s %s  event indication\n", cbuff,
                    val_to_string( stbptr->msgId, dsmcc_msgid_names ),
                    sessionId_to_string( dsmccptr->sdb_init_request.sessId ) );
        }
    }
    else {
        printf( "\n\n\nrecived unknown id %04X\n\n\n", stbptr->msgId );
    }

    return TRUE;
} /* stb_dsmcc_in */

void
stb_FSM( stb_t* stbptr, gint* sourceidptr, gint sourceid_min, gint sourceid_max ) {
    /*  stb state machine
     *
     *                               -( w )-
     *                     ( Tx )    |     |
     * (st)     InitRq    -------->   InitC
     *                                 |
     *                                 | ( Rx )
     *                    ( Tx )       |
     *                   <--------     V
     *  ( w ) = SelectC             SelectRq
     *                   -------->
     *                    ( Rx )       ^
     *                                 | ( Tx )
     *                    ( Rx )       |
     *          SelectI  -------->  SelectRsp
     *          QueryR   -------->  QueryC
     *          EventI   -------->  EventRsp
     *                    (intrnl)  ActRpt
     */


    /* get seconds and useconds for timing */
    struct timeval tmval;
    if ( gettimeofday( &tmval, NULL ) != 0 ) {
        perror( "stb_fsm gettimeofday: " );
    }

    gboolean       b_dwelling;
    b_dwelling  = ( stbptr->state == e_state_next );
    b_dwelling &= ( tmval.tv_sec < stbptr->dwell_time.tv_sec ) ||
                  ( tmval.tv_sec == stbptr->dwell_time.tv_sec &&
                    tmval.tv_usec < stbptr->dwell_time.tv_usec );

    gboolean b_timeout;
    b_timeout  = ( stbptr->state == e_state_wait );
    b_timeout &= ( tmval.tv_sec > stbptr->time_out.tv_sec ) ||
                 ( tmval.tv_sec == stbptr->time_out.tv_sec &&
                   tmval.tv_usec > stbptr->time_out.tv_usec );

    gboolean         b_dbgabnevnt = FALSE;
    gchar*           fsmdbgtxtptr = " FSM ";

    estate           oldstate     = stbptr->state;
    DSMCC_MSGID_SDV  oldmsg       = stbptr->msgId;

    if ( stbptr->state == e_state_done ) {
        ;    /* here if this stb is done; do nothing */
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_SELECT_INDICATION ) {
        /* here if server sent revised tune info */
        stbptr->msgId       = DSMCC_MSGID_SDV_SELECT_RESPONSE;
        stbptr->tunefailcnt = 0;
        stbptr->state       = e_state_tx;
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_SELECT_RESPONSE ) {
        /* here if stb responded revised tune info */
        stbptr->msgId = DSMCC_MSGID_SDV_SELECT_REQUEST;
        if ( stbptr->flags & RETUNEFAILURES ) {
            stbptr->state = e_state_tx;
        }
        else {
            stbptr->tunefailcnt = 0;
            stbptr->state       = e_state_next;
        }
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_QUERY_REQUEST ) {
        /* here if server needs tuned src id */
        stbptr->msgId = DSMCC_MSGID_SDV_QUERY_CONFIRM;
        stbptr->state = e_state_tx;
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_QUERY_CONFIRM ) {
        /* here if server responded to QUERY_REQUEST */
        stbptr->msgId = DSMCC_MSGID_SDV_SELECT_REQUEST;
        stbptr->state = e_state_next;
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_EVENT_INDICATION ) {
        /* here if server sent ???? */
        stbptr->msgId = DSMCC_MSGID_SDV_EVENT_RESPONSE;
        stbptr->state = e_state_tx;
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_EVENT_RESPONSE ) {
        /* here if stb EvntRspn EVENT RESPONSE */
        stbptr->msgId = DSMCC_MSGID_SDV_SELECT_REQUEST;
        stbptr->state = e_state_next;
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_ACTIVITY_REPORT ) {
        /* here if stb sent ACTIVITY REPORT */
        stbptr->msgId = DSMCC_MSGID_SDV_SELECT_REQUEST;
        stbptr->state = e_state_next;
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_INIT_CONFIRM ) {
        /* here if stb just init'd, so go right to program select */
        stbptr->msgId    = DSMCC_MSGID_SDV_SELECT_REQUEST;
        stbptr->state    = e_state_next;
        stbptr->sourceId = 0;
    }
    else if ( stbptr->msgId == DSMCC_MSGID_SDV_SELECT_CONFIRM  ) {
        /* here if stb has tuned a channel */
        stbptr->tunefailcnt = 0;

        if ( stbptr->flags & LOOPFLG ) {
            /* here if stb need to select another channel */
            stbptr->msgId = DSMCC_MSGID_SDV_SELECT_REQUEST;
            stbptr->state = e_state_next;
        }
        else {
            /* here if stb is done tuning channels */
            stbptr->state = e_state_done;
        }
    }
    else if ( stbptr->state == e_state_wait  ) {
        if ( stbptr->sourceId == 0 ) {
            /* here if stb sent a sign off msg; srcid=0 and waiting */
            fsmdbgtxtptr  = " FSM signoff ";

            stbptr->state = e_state_done;
            b_dbgabnevnt  = TRUE;
        }
        else if ( b_timeout ) {
            /* here if stb spent too much time NOT tx'g */
            fsmdbgtxtptr  = " FSM timeout ";
            b_dbgabnevnt  = TRUE;
            stbptr->state = e_state_next;
        }
    }
    else if ( b_timeout ) {
        /* here if stb spent too much time NOT tx'g */
        fsmdbgtxtptr  = " FSM timeout ";
        b_dbgabnevnt  = TRUE;
        stbptr->state = e_state_next;
    }
    else if ( stbptr->state == e_state_tx ) {
        ; // waiting for service group  to send this stb message
    }
    else if ( stbptr->state == e_state_next ) {
        if ( stbptr->msgId == DSMCC_MSGID_SDV_INIT_REQUEST ) { /* do not change this */
            /* here if stb timed out and did not init, so try again */
            if ( stbptr->flags & NOINITFLG ) {
                /* here if skipping the init sequence and going straight to channel */
                stbptr->msgId = DSMCC_MSGID_SDV_SELECT_REQUEST;
            }
            else {
                /* here if stb is to send init */
                stbptr->state = e_state_tx;
            }
        }
        else if ( stbptr->msgId == DSMCC_MSGID_SDV_SELECT_REQUEST ) {
            /* here if stb is ready for a new channel */
            /*      or looping to another channel */
            if ( b_dwelling ) {
                /* wait for dwell time to expire */
                ;
            }
            else {
                /* ready to move on */
                stbptr->sourceId = *sourceidptr;
                stbptr->state    = e_state_tx;

                /* set next channel for next stb */
                if ( ++( *sourceidptr ) > sourceid_max ) {
                    *sourceidptr = sourceid_min;
                }
            }
        }
    }
    else {
        /* here if no other events triggered */
        fsmdbgtxtptr = "FSM invalid msgid";
        printf( " \n\n\n\n STB %s is invalid msgid = %04X; forcing to DSMCC_MSGID_SDV_SELECT_REQUEST\n\n\n",
                sessionId_to_string( stbptr->macaddr ), stbptr->msgId );

        stbptr->state = e_state_done;
    }

    // log message event type  and abnormal events if dbg'ing fsm abnormals
    // log all if dbg'ing fsm full
    gboolean  b_display;
    //b_display = stbptr->msgId & DSMCC_MSGID_SDV_FSM_DEBUG_FLAGS;
    //b_display |= b_dbgabnevnt;
    b_display  = b_dbgabnevnt;
    b_display &= stbptr->flags & DBGFSMABN;

    b_display |= stbptr->flags & DBGFSMFULL;

    if ( b_display && ( stbptr->state != oldstate || stbptr->msgId != oldmsg ) ) {
        if ( stbptr->flags & DBGFSMFULL ) {
            printf( "fsm %s %s --> %s  %s \n",
                    val_to_string( oldstate, debug_stbstate_names ),
                    val_to_string( oldmsg, dsmcc_msgid_names ),
                    val_to_string( stbptr->state, debug_stbstate_names ),
                    val_to_string( stbptr->msgId, dsmcc_msgid_names )
                  );
        }
        dbg_print_stb( fsmdbgtxtptr, stbptr );
    }

    /* reset timeout period for stbs ready to tx */
//    if ( stbptr->state != e_state_wait )
//    {   /* here to reset timeout */
//        stb_set_timeout( stbptr, &tmval );
//    }
} /* stb_FSM */

gchar*
sessionId_to_string( guint8 sessionId[] ) {
    guint i, j;
    for ( i = 0, j = 0; i < 10; i++, j += 2 ) {
        snprintf( vsbuff + j, 40 - j, "%02X", sessionId[i] );
    }

    return vsbuff;
}

void
dbg_print_stb( gchar* str, stb_t* stbptr ) {
    if ( str != NULL ) {
        printf( str );
    }
    print_stb( stbptr );
}

void
print_stb( stb_t* stbptr ) {
    //gint           i;

    /* get seconds and useconds for timing */
    struct timeval tmval;
    if ( gettimeofday( &tmval, NULL ) != 0 ) {
        perror( "print_stb gettimeofday: " );
    }

    /*
        time = tmval.tv_sec * SECOND_UTIME + tmval.tv_usec;

        printf( "to-now:%u to-stb:%u  %s sg%i tx:%i delta:%+i state:%s %s",
                    time, stbptr->time_out,
                    sessionId_to_string( stbptr->macaddr ),
                    stbptr->servicegroup, stbptr->transxId,
                    stbptr->time_out - time,
                    val_to_string( stbptr->state, debug_stbstate_names ),
                    val_to_string( stbptr->msgId, dsmcc_msgid_names ) );
    */
    guint time;
    time = ( stbptr->time_out.tv_sec - tmval.tv_sec ) * SECOND_UTIME +
           ( stbptr->time_out.tv_usec - tmval.tv_usec );

    printf( "%s sg%i tx:%i delta:%+i state:%s %s srcid:%i\n",
            sessionId_to_string( stbptr->macaddr ),
            stbptr->servicegroup, stbptr->transxId,
            time,
            val_to_string( stbptr->state, debug_stbstate_names ),
            val_to_string( stbptr->msgId, dsmcc_msgid_names ),
            stbptr->sourceId );

} /* print_stb */
