#ifndef _GTP_H_
#define _GTP_H_

/*!
 \defgroup GTP_H_       GTP
 \ingroup  DATA_COMMON_H_
 \brief     GPRS Tunnel Protocol (GTP) -- RX and TX Path
 \{
 */


/*- INCLUDES -------------------------------------------------------------------------------------*/

/*- MACROS ---------------------------------------------------------------------------------------*/

#define NUMBER_OF_TUNNEL_CONTEXTS_PER_RECORD        4

#define GTP_EXTENSION_HEADER_TYPE_LIST_LENGTH       2

#define GTP_SUPPORTED_EXTENSION_HEADERS_NOTIFICATION_MSG_LENGTH     GTP_EXTENSION_HEADER_TYPE_LIST_LENGTH + NUMBER_2/* TYPE+LENGTH feilds*/

#define GTP_PDCP_EXTENSION_HEADER_LENGTH            1

#ifdef MAIN_TEST
#define T3_5_SEC_TICK                          500000
#else
/* Maximum wait time for a response of a requested message */
#define T3_5_SEC_TICK                          5000
#endif
#define ECHO_TIMER_STEP                         5
/* No of ticks to make 60 seconds where the tick represents 5 seconds*/
#define T3_60_SECS_TICKS                            12
/* Maximum number of requests to be sent before receiving a response */
#define MAX_NO_OF_ECHO_REQUESTS_WITHOUT_RESPONSE    3


#define GTP_DL_TUNNEL_TEID_INDEX                    0

#define GTP_FWD_UL_TUNNEL_TEID_INDEX                1

#define GTP_FWD_DL_TUNNEL_TEID_INDEX                2

#define GTP_TUNNEL_INDEX_BIT_COUNT                  16
