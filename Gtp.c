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
/*------------------------------------------------------------------------------------------------*/
/*!
  \macro          GTP_CONVERT_RATE_TO_NUMBER_OF_BYTES(a_bitRate)

  \param[in]      a_bitRate : bits/sec that can be received for this RB.

  \brief          calculate no of bytes that can be received on RB within a certain window,

  \return         <Return Type>

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
#define GTP_CONVERT_RATE_TO_NUMBER_OF_BYTES(a_bitRate)                                             \
    (a_bitRate * POLICING_WINDOW_IN_SUBFRAMES)/(1000 * 8)

/*- TYPES ----------------------------------------------------------------------------------------*/

/*- GLOBAL VARIABLES -----------------------------------------------------------------------------*/


/*- LOCAL VARIABLES ------------------------------------------------------------------------------*/
GTP_Context* g_GTP_Ptr;
/*- FUNCTION DECLARATIONS ------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             SDS_STATIC SDS_BOOL GTP_addEchoPath(eNB_TransportAddress* pathAddress, GTP_TunnelContext* a_tunnelRecord_Ptr)

  \param[in]      eNB_TransportAddress* pathAddress: Peer IP.
  \param[in]      GTP_TunnelContext* a_tunnelRecord_Ptr :  pointer to tunnel record.

  \brief          Adds the eNB_TransportAddress in echoPath_List if not already found.

  \return         SDS_BOOL : addition status.

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_STATIC SDS_BOOL GTP_addEchoPath(eNB_TransportAddress* pathAddress,GTP_TunnelContext* a_tunnelRecord_Ptr);
/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             SDS_STATIC void GTP_removeEchoPath(GTP_TunnelContext* a_tunnelRecord_Ptr)

  \param[in]      GTP_TunnelContext* a_tunnelRecord_Ptr : Pointer to tunnel record.

  \brief          Removes the eNB_TransportAddress in echoPath_List.

  \return         SDS_STATIC void

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_STATIC void GTP_removeEchoPath(GTP_TunnelContext* a_tunnelRecord_Ptr);

/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             SDS_STATIC void GTP_updateEchoPath(eNB_TransportAddress* pathAddress, SDS_UINT32 a_bearerIndex, GTP_TunnelContext* a_tunnelRecord_Ptr)

  \param[in]      eNB_TransportAddress* pathAddress : pointer to new IP.
  \param[in]      SDS_UINT32 a_bearerIndex : bearer index that corresponds to this tunnel.
  \param[in]      GTP_TunnelContext* a_tunnelRecord_Ptr : pointer to tunnel record.

  \brief          Updates the eNB_TransportAddress in echoPath_List.

  \return         SDS_BOOL : update status.

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_STATIC SDS_BOOL GTP_updateEchoPath(eNB_TransportAddress* a_pathAdress_Ptr, SDS_UINT32 a_bearerIndex, GTP_TunnelContext* a_tunnelRecord_Ptr);

/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             SDS_STATIC void GTP_handleEchoResponse(SDS_UINT16 receivedEchoSN,eNB_TransportAddress* srcAdrress)

  \param[in]      SDS_UINT16 receivedEchoSN : sequence number received in the response.
  \param[in]      eNB_TransportAddress* srcAdrress : IP of the sending peer.

  \brief          Handles the received Echo response by updating the Echo path list with the received responses.

  \return         SDS_STATIC void

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_STATIC void GTP_handleEchoResponse(SDS_UINT16 receivedEchoSN,eNB_TransportAddress* srcAddress);
/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             SDS_STATIC void  GTP_processPacket(GTP_PACKET* gtpPacket ,SDS_UINT8 *payloadStart_Ptr,SDS_UINT32 payloadOffset, GTP_SDU* receivedGTP_desc_Ptr)

  \param[in]      GTP_PACKET* gtpPacket : pointer to decoded header.
  \param[in]      SDS_UINT8 *payloadStart_Ptr : pointer to the reception buffer.
  \param[in]      SDS_UINT32 payloadOffset : offset to get message start.
  \param[in]      GTP_SDU* receivedGTP_desc_Ptr : pointer to GTP packet descriptor.

  \brief          Processes a GTP packet and perform the suitable action.

  \return         SDS_STATIC void

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_STATIC void  GTP_processPacket(GTP_HEADER* gtpPacket ,SDS_UINT8 *packetStart_Ptr,SDS_UINT32 payloadOffset, GTP_SDU* receivedGTP_desc_Ptr);


/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             SDS_STATIC void  GTP_sendSupportedExtensionHeaderNotification(eNB_TransportAddress* srcAddress)

  \param[in]      eNB_TransportAddress* srcAddress : IP address of the destination peer.

  \brief          Handles sending the ExtensionHeaderNotification.

  \return         void

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_STATIC void  GTP_sendSupportedExtensionHeaderNotification(eNB_TransportAddress* srcAddress);

/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             SDS_STATIC void GTP_sendEchoReq(void)

  \param[in]      eNB_TransportAddress* pathAddress : Path IP that echo request will be sent to.
  \param[in]      SDS_UINT16 EchoReqSN : sequence number that will be sent in the request.

  \brief          Handles sending Echo request message to pathAdress with EchoReqSN in the packet.

  \return         void

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_STATIC void GTP_sendEchoReq(eNB_TransportAddress* pathAddress , SDS_UINT16 EchoReqSN);
