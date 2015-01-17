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
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/*!
\macro          GTP_GET_TRANSPORT_ADRESS(newTunnelContex_Ptr,a_msg_Ptr)

\param[in]      newTunnelContex_Ptr,a_msg_Ptr

\brief          Gets the Transport address from the a_msg_Ptr and set it in the tunnel newTunnelContext

\return         <Return Type>

\b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
#define COPY_TRANSPORT_ADDRESS( dst , src )         \
{                                                   \
        dst.addrV4.addr = src.addrV4.addr;          \
}                                                   \


#define GET_TUNNEL_TYPE_INDEX_FROM_TEID(TEID)                   \
     (SDS_UINT16)(TEID >> GTP_TUNNEL_INDEX_BIT_COUNT)           \
/*- TYPES ----------------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/*!
\enum  GTP_tunnelStatus

\breif The GTP tunnel status
*/
/*------------------------------------------------------------------------------------------------*/
typedef enum _GTP_tunnelStatus
{
    /*! The tunnel is open and a connection is established*/
    GTP_TUNNEL_STATUS_OPEN = 0,
    /*! The tunnel is closed and the connection is terminated*/
    GTP_TUNNEL_STATUS_CLOSED = 1
} GTP_tunnelStatus;

/*------------------------------------------------------------------------------------------------*/
/*!
\enum  GTP_TunnelMode

\breif Holds the Mode of the established tunnel , either Uplink or Downlik
*/
/*------------------------------------------------------------------------------------------------*/
typedef enum _GTP_TunnelMode
{
    /*! Established tunnel direction is Uplink*/
    GTP_TUNNEL_UPLINK = 0,
    /*! Established tunnel direction is Downlink*/
    GTP_TUNNEL_DOWNLINK = 1
} GTP_TunnelMode;
