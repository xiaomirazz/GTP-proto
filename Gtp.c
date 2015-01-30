/*- INCLUDES -------------------------------------------------------------------------------------*/
#include "enb_common_includes.h"
#include "data_common.h"
#include "gtp_header.h"
#include "data_statistics.h"
#include "gtp.h"

#include <netinet/in.h>
#include "gtp_al.h"


#include "wrp_timer.h" /*../../../../../sds_os/_output/include/*/
/*- MACROS ---------------------------------------------------------------------------------------*/
#define ENB_COMPONENT_ID        COMPONENT_ID_GTP_U
#define COMPONENT_ID()          (GTP_LOG_ID)
#define COMPONENT_MASK          COMPONENT_MASK_GTP_U

#define GTP_DEBUG

#define POLICING_WINDOW_IN_SUBFRAMES       250/*0.25 sec*/

#ifdef T1_TEST
SDS_UINT32 t3_scale_factor;
#endif
/*------------------------------------------------------------------------------------------------*/
/*!
  \macro          FILL_CONSTANT_PART_IN_GTP_PACKET(gtpPacket)

  \param[in]      gtpPacket : GTP header.

  \brief          Fills constant parts in GTP packet as Cersion,Protocol type,
                  Extenstension Header Flag, SequenceNumber Flag

  \return

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
#define FILL_CONSTANT_PART_IN_GTP_PACKET(gtpPacket)     \
{                                                       \
    gtpPacket.version = GTP_VERSION;                    \
    gtpPacket.protocolType = GTP_PROTOCOL_TYPE_GTP;     \
    gtpPacket.extensionFlag = FALSE;                    \
    gtpPacket.sequenceNumberFlag = TRUE;                \
    gtpPacket.pnFlag = FALSE;                           \
    gtpPacket.msgType = GTP_MSG_TYPE_GPDU;              \
}

/*------------------------------------------------------------------------------------------------*/
/*!
  \macro          GTP_TX_NWK(a_buff,a_len ,a_destAddress,a_MSG_MORE_Flag)

  \param[in]      a_buff : pointer to the encoded buffer.
  \param[in]      a_len  : Buffer length.
  \param[in]      a_destAddress : IP address of the destination.
  \param[in]      a_MSG_MORE_Flag : Flag indicates if this packet is complete or its remaining part
                                    will be sent next time.

  \brief          Send the encoded GTP packet on the network.

  \return         <Return Type>

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
#define GTP_TX_NWK(a_buff,a_len ,a_destAddress,a_MSG_MORE_Flag,a_status_Ptr)                       \
{                                                                                                  \
    SDS_UINT32      peerHostIp;                                                                    \
                                                                                                   \
    peerHostIp = a_destAddress->addrV4.addr;                                                       \
                                                                                                   \
    LOG_TRACE_ARG("Sending Data Packet with length=%d", a_len, ZERO, ZERO);                        \
    *a_status_Ptr = GTP_AL_sendGTPpacket(a_buff,a_len,peerHostIp,a_MSG_MORE_Flag);                 \
                                                                                                   \
}
