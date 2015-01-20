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

/*------------------------------------------------------------------------------------------------*/
/*!
\enum  GTP_Tunnel_QCI_Type

\breif Tunnel type GBR or Non GBR.
*/
/*------------------------------------------------------------------------------------------------*/
typedef enum _GTP_Tunnel_QCI_Type
{
    /*! Guaranteed bit rate*/
    GBR = 0,
    /*! Non guaranteed bit rate*/
    NON_GBR = 1
} GTP_Tunnel_QCI_Type;

/*------------------------------------------------------------------------------------------------*/
/*!
\enum  GTP_PacketStatus

\breif Indicates whether this packet will be processed or discarded
*/
/*------------------------------------------------------------------------------------------------*/
typedef enum _GTP_PacketStatus
{
    /*! Process*/
    GTP_Process = 0,
    /*! Discard*/
    GTP_Discard = 1
} GTP_PacketStatus;

/*------------------------------------------------------------------------------------------------*/
/*!
\struct   GTP_pathEchoRecord

\brief    Holds the GTP path ECHO status which shows
*/
/*------------------------------------------------------------------------------------------------*/
typedef struct _GTP_pathEchoRecord
{
    /*! Node to be inserted in Echo Path List*/
    ListNode               listNode;

    /*! Path Transport address (port + IP ) */
    eNB_TransportAddress    pathAddress;

    /*! Number of elapsed ticks where each tick represent 5 seconds , used to calculate 60 seconds*/
    SDS_UINT32              elapsedTicksCount;

    /*! Number of sent Echo Requests without receiving corresponding response */
    SDS_UINT32              EchoREQCount;

    /*! Flag indicates if an Echo request was sent */
    SDS_BOOL32              isEchoReqSent;

    /*! Number of tunnels related to this path */
    SDS_UINT32              relatedTunnelsCount;

    /*! Sequence number of ECHO request */
    SDS_UINT16              EchoReqSN;

    /*! List of tunnels that belong to this path*/
    UTILS_LinkedList*       tunnel_List_Ptr;

} GTP_pathEchoRecord;
/*------------------------------------------------------------------------------------------------*/
/*!
\struct   GTP_TunnelContext

\brief    Holds UL GTP tunnel info
*/
/*------------------------------------------------------------------------------------------------*/
typedef struct _GTP_TunnelContext
{
    /*! Node to be inserted in Echo Path List*/
    ListNode            listNode;

    /*! Flag to indicate if the End Marker was received for this tunnel.
     * If TRUE ,any packet received on this tunnel will be discarded silently*/
    SDS_BOOL32          isEndMarkerReceived;

    /*! Tunnel End-point ID*/
    SDS_UINT32          TEID;

    /*! Sub frame number that refers to the start of the window*/
    SDS_UINT32          windowStartSubframe;

    /*! Bearer index of this tunnel*/
    SDS_UINT16          bearerIndex;

    /*! Expected Sequence number for Uplink, equals the latest received sequence number + 1*/
    SDS_UINT16          expectedSeqNumber;

    /*! Pointer to path record  */
    GTP_pathEchoRecord* pathRecord_Ptr;

    /*! GBR/Non-GBR */
    GTP_Tunnel_QCI_Type QCI_Type;

    /*! Maximum number of bytes allowed for this tunnel*/
    SDS_UINT32          GBR_MaxAllowedBytes;

    /*! Total received bytes during window on this tunnel*/
    SDS_UINT32          GBR_TotalReceivedBytes;

} GTP_TunnelContext;


/*------------------------------------------------------------------------------------------------*/
/*!
\struct   GTP_TunnelRecord

\brief 	  Contains pointers to GTP contexts for Uplink , Downlink and forwarding tunnels related
          to a certain Radio Bearer. Not all Radio bearers are bi-directional,therefore someof the
          of the elements in the structure may not be significant.
*/
/*------------------------------------------------------------------------------------------------*/
typedef struct _GTP_TunnelRecord
{
    /*! Uplink Tunnel */
    GTP_TunnelContext*           UL_Tunnel_Ptr;

    /*! Downlink Tunnel */
    GTP_TunnelContext*           DL_Tunnel_Ptr;

    /*! Uplink Forwarding Tunnel */
    GTP_TunnelContext*           FWD_UL_Tunnel_Ptr;

    /*! Downlink Forwarding Tunnel */
    GTP_TunnelContext*           FWD_DL_Tunnel_Ptr;

    /*! UE Index*/
    SDS_UINT16                   UE_Index;

} GTP_TunnelRecord;
