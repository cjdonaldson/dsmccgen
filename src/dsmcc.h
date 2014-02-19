#ifndef __DSMCC_H__
#define __DSMCC_H__

#include "glib-min.h"
#include <string.h>
#include <stdio.h>

#define MYPORT 13822 //13822 /* 0 will pull from port pool */
#define MYADDR "0.0.0.0"

//gchar *DSTADDR;         /* INADDR_ANY allows socket to receive anybody; "192.168.9.252" */
#define DSTPORT             13822
#define DSMCC_DESCRIMINATOR 0x11
#define DSMCC_TYPE_SDV      0x04

#define DSMCC_PAYLOAD_OFFSET sizeof( struct st_dsmcc_hdr )

struct st_dsmcc_hdr {
    /* hdr */
    guint8  descriminator;
    guint8  type;
    guint16 msgId;
    guint32 transxId;
    guint8  res;
    guint8  adaptLen;
    guint16 msgLen;
};

struct st_dsmcc_sdb_init_request {
    /* payload for init v2.12 */
    guint8  sessId[10];
    guint16 res1;
    guint32 serviceGroupId;
    guint8  version1;        /* set to 0 */
    guint8  res2;
    guint8  numDesc;         /* set to 0 */
    /*    guint16 version2; */
};

struct st_dsmcc_sdb_init_confirm {
    /* payload for init confirm */
    guint8  sessId[10];
    guint16 response;        /* dsmcc_selectresponse_names */
};

struct st_dsmcc_sdb_select_request {
    /* payload for select request */
    guint8  sessId[10];
    guint8  retryCount;      /* v2.12 this field is reserved; set to 0xFF */
    guint8  res1;            /* set to 0xFF */
    guint32 sourceId;
    guint16 dataLen;         /* set to 6 */
    guint8  tunerUse;
    guint8  res2;            /* set to 0xFF */
    guint32 serviceGroupId;
};

struct st_dsmcc_sdb_select_confirm {
    /* payload for select confirm */
    guint8  sessId[10];
    guint16 response;        /* dsmcc_selectresponse_names */
    guint32 sourceId;
    guint16 dataLen;         /* set to 10 */
    guint8  res1;            /* set to 0xFF */
    guint8  modulation;      /* use dsmcc_modfmt_names */
    guint32 frequency;
    guint16 programNumber;
    guint16 res2;
};

struct st_dsmcc_sdb_select_indication {
    /* payload for select indications */
    guint8  sessId[10];
    guint16 reason;          /* dsmcc_selectreason_names */
    guint32 sourceId;
    guint16 dataLen;         /* set to 14 */
    guint8  res1;
    guint8  modulation;      /* use dsmcc_modfmt_names */
    guint32 frequency;
    guint16 programNumber;
    guint16 res2;
    guint32 sourceId2;
};

struct st_dsmcc_sdb_select_response {
    /* payload for select response */
    guint8  sessId[10];
    guint16 response;        /* dsmcc_selectresponse_names */
    guint8  dataLen;         /* set to 0 */
};

struct st_dsmcc_sdb_query_request {
    guint8  sessId[10];
    guint16 res;
};

struct st_dsmcc_sdb_query_confirm {
    guint8  sessId[10];
    guint16 response;        /* dsmcc_selectresponse_names */
    guint32 sourceId;
    guint16 dataLen;         /* set to 10 */
    guint8  tunerUse;
    guint8  res2;            /* set to 0xFF */
    guint32 serviceGroupId;
    guint32 lastUserActivity;
};

struct st_dsmcc_sdb_activity_report {
    guint8  sessId[10];
    guint16 res;            /* set to 0xFFFF */
    guint32 sourceId;
    guint32 lastUserActivity;
};

/* not used
struct st_dsmcc_sdb_event_indication {
    guint8  sessId[10];
    guint16 res1;            / * set to 0xFF * /
    guint16 dataLen;         / * set to var * /
    guint16 res2;            / * set to 0xFF * /
    guint32 sourceId;
    guint8  tag;
    guint8  len;
    guint32 data;
};

struct st_dsmcc_sdb_event_response {
    guint8  sessId[10];
    guint16 response;        / * dsmcc_selectresponse_names * /
};
*/

struct st_dsmcc {
    struct st_dsmcc_hdr hdr;
    union {
        struct st_dsmcc_sdb_init_request      sdb_init_request;
        struct st_dsmcc_sdb_init_confirm      sdb_init_confirm;
        struct st_dsmcc_sdb_select_request    sdb_select_request;
        struct st_dsmcc_sdb_select_confirm    sdb_select_confirm;
        struct st_dsmcc_sdb_select_indication sdb_select_indication;
        struct st_dsmcc_sdb_select_response   sdb_select_response;
        struct st_dsmcc_sdb_query_request     sdb_query_request;
        struct st_dsmcc_sdb_query_confirm     sdb_query_confirm;
        struct st_dsmcc_sdb_activity_report   sdb_activity_report;
    };
};

typedef struct st_dsmcc dsmcc_t;

typedef struct _value_string {
    guint32  value;
    gchar*    strptr;
} value_string;

typedef enum {
    DSMCC_MSGID_SDV_SELECT_REQUEST       = 0x0001,  /* message from stb */
    DSMCC_MSGID_SDV_SELECT_CONFIRM       = 0x0002,  /* message from svr */
    DSMCC_MSGID_SDV_SELECT_INDICATION    = 0x0003,  /* message from svr */
    DSMCC_MSGID_SDV_SELECT_RESPONSE      = 0x0004,  /* message from stb */
    DSMCC_MSGID_SDV_ACTIVITY_REPORT      = 0x8000,  /* message from stb */

    DSMCC_MSGID_SDV_INIT_REQUEST         = 0x8001,  /* message from stb */
    DSMCC_MSGID_SDV_INIT_CONFIRM         = 0x8002,  /* message from svr */
    DSMCC_MSGID_SDV_QUERY_REQUEST        = 0x8003,  /* message from svr */
    DSMCC_MSGID_SDV_QUERY_CONFIRM        = 0x8004,  /* message from stb */
    DSMCC_MSGID_SDV_EVENT_INDICATION     = 0x8005,  /* message from svr */
    DSMCC_MSGID_SDV_EVENT_RESPONSE       = 0x8006,  /* message from stb */

    DSMCC_MSGID_SDV_FSM_DEBUG_FLAGS      = DSMCC_MSGID_SDV_SELECT_INDICATION ||
                                           DSMCC_MSGID_SDV_SELECT_RESPONSE ||
                                           DSMCC_MSGID_SDV_EVENT_INDICATION ||
                                           DSMCC_MSGID_SDV_EVENT_RESPONSE
} DSMCC_MSGID_SDV;

static const value_string dsmcc_msgid_names[] = {
    { DSMCC_MSGID_SDV_SELECT_REQUEST,    "SlctRqst" },
    { DSMCC_MSGID_SDV_SELECT_CONFIRM,    "SlctCnfm" },
    { DSMCC_MSGID_SDV_SELECT_INDICATION, "SlctIndn" },
    { DSMCC_MSGID_SDV_SELECT_RESPONSE,   "SlctRspn" },
    { DSMCC_MSGID_SDV_ACTIVITY_REPORT,   "ActRprt " },
    { DSMCC_MSGID_SDV_INIT_REQUEST,      "InitRqst" },
    { DSMCC_MSGID_SDV_INIT_CONFIRM,      "InitCnfm" },
    { DSMCC_MSGID_SDV_QUERY_REQUEST,     "QryRqst " },
    { DSMCC_MSGID_SDV_QUERY_CONFIRM,     "QryCnfm " },
    { DSMCC_MSGID_SDV_EVENT_INDICATION,  "EvntIndn" },
    { DSMCC_MSGID_SDV_EVENT_RESPONSE,    "EvntRspn" },
    { 0, NULL }
};

static const value_string dsmcc_modfmt_names[] = {
    { 0x00, "none  " },
    { 0x06, "QAM16 " },
    { 0x07, "QAM32 " },
    { 0x08, "QAM64 " },
    { 0x0C, "QAM128" },
    { 0x10, "QAM256" },
    { 0, NULL }
};

/* b             b   bit mask for tuner use = 0x08
   7 6 5 4 3 2 1 0
   ___ _____ _ _ _
    ^    ^   ^ ^ ^ tuner is recording
    |    |   | |__ reserved = 0
    |    |   |____ tuner is PPV
    |    |________ Display
    |_____________ reserved = 0
   */

static const value_string dsmcc_tuneruse_record_names[] = {
    { 0, "NotRecording" },
    { 1, "Recording   " },
    { 0, NULL }
};

static const value_string dsmcc_tuneruse_ppv_names[] = {
    { 0, "not in PPV" },
    { 1, "in PPV    " },
    { 0, NULL }
};

static const value_string dsmcc_tuneruse_display_names[] = {
    { 0, "HD only / No Display" },
    { 1, "Main" },
    { 2, "PIP" },
    { 0, NULL }
};

static const value_string dsmcc_tuneruse_fail_names[] = {
    { 0, "Tuned" },
    { 1, "Tune Failed" },
    { 0, NULL }
};

enum {
    rsnOk                   = 0x0000,
    rsnNormal               = 0x0001,
    rsnSeEntitlementFailure = 0x0002,
    rsnForceTune            = 0x8000,
    rsnProgNotAvailable     = 0x8001
};

static const value_string dsmcc_selectreason_names[] = {
    { rsnOk,                   "rsnOK       " },
    { rsnNormal,               "rsnNormal   " },
    { rsnSeEntitlementFailure, "rsnSeEntitlementFailure" },
    { rsnForceTune,            "rsnFrcTune  " },
    { rsnProgNotAvailable,     "rsnPrgNotAvl" },
    { 0, NULL }
};

enum {
    rspOk                  = 0x0000,
    rspFomratError         = 0x0001,
    rspNoSession           = 0x0002,
    rspProgOutOfService    = 0x0006,
    rspRedirect            = 0x0007,
    rspInvalidSG           = 0x8001,
    rspUnknownClient       = 0x8002,
    rspSeNoResource        = 0x8003,
    rspNetBwNotAvail       = 0x8004,
    rspSrvCapacityExceeded = 0x8005,
    rspVerNotSuport        = 0x8006,
    rspUnknownError        = 0x8FFF
};

static const value_string dsmcc_selectresponse_names[] = {
    { rspOk,                  "rspOK" },
    { rspFomratError,         "rspFormatError" },
    { rspFomratError,         "rspNoSession" },
    { rspProgOutOfService,    "rspPrgOutOfSrvc" },
    { rspProgOutOfService,    "rspRedirct" },
    { rspInvalidSG,           "rspInvldSG" },
    { rspUnknownClient,       "rspunkClnt" },
    { rspSeNoResource,        "rspSeNoRsrc" },
    { rspNetBwNotAvail,       "rspNetBwUnAvail" },
    { rspSrvCapacityExceeded, "rspSrvCapctyExcdd" },
    { rspVerNotSuport,        "rspVerNotSupported" },
    { rspUnknownError,        "rspUnknownError" },
    { 0, NULL }
};

gchar* val_to_string( guint32 value, const value_string vs[] );
void print_dsmcc( gchar dsmccptr[], gint len );

#endif
