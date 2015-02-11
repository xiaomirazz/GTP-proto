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
/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             SDS_STATIC void GTP_sendEchoRsp(SDS_UINT16 a_SN)

  \param[in]      SDS_UINT16 a_SN : sequence number that will be sent in the response.

  \brief          Handles sending Echo response with a given Sequence number.

  \return         void

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_STATIC void GTP_sendEchoRsp(SDS_UINT16 a_SN , eNB_TransportAddress* srcAddress);

/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             void GTP_sendErorrIndication(eNB_TransportAddress* srcAddress, SDS_UINT16 a_SenderPortNum, SDS_UINT32 a_TEID)

  \param[in]      eNB_TransportAddress* srcAddress : IP of the sender peer.
  \param[in]      SDS_UINT16 a_SenderPortNum : sender port number.
  \param[in]      SDS_UINT32 a_TEID : tunnel ID received in the message.

  \brief          Sends error indication message when the received message has no context.

  \return         void

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_STATIC void GTP_sendErorrIndication(eNB_TransportAddress* srcAddress, SDS_UINT16 a_SenderPortNum, SDS_UINT32 a_TEID);

/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             void GTP_HandleErorrIndication(eNB_TransportAddress* srcAddress, SDS_UINT32 a_TEID)

  \param[in]      eNB_TransportAddress* srcAddress : IP of the sender peer.
  \param[in]      SDS_UINT32 a_TEID : tunnel ID that this meesage belongs to.

  \brief          Handle error indication message reception be requesting RB release from RRC.

  \return         void

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_STATIC void GTP_HandleErorrIndication(eNB_TransportAddress* srcAddress, SDS_UINT32 a_TEID);
/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             GTP_PacketStatus GTP_PolicingAlgorithm(GTP_TunnelRecord* a_tunnelRecord_Ptr,GTP_TunnelContext* a_tunnelContext_Ptr,SDS_UINT16 a_receivedLength)

  \param[in]      GTP_TunnelRecord* a_tunnelRecord_Ptr : Pointer to tunnel record.
  \param[in]      GTP_TunnelContext* a_tunnelContext_Ptr : Pointer to tunnel context.
  \param[in]      SDS_UINT16 a_receivedLength : received packet length.

  \brief          Decides whether this packet will be discarded or processed according to the bit rate
                  allowed to this RB.

  \return         GTP_PacketStatus

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_STATIC GTP_PacketStatus GTP_PolicingAlgorithm(GTP_TunnelRecord* a_tunnelRecord_Ptr,GTP_TunnelContext* a_tunnelContext_Ptr,SDS_UINT16 a_receivedLength);

/*- FUNCTION DEFINITIONS -------------------------------------------------------------------------*/

void GTP_init_Handler(GTP_U_init* a_msg_Ptr)
{
    SDS_UINT32                 i;
    GTP_pathEchoRecord*        pathRecord_Ptr = NULL;
    /*CROSS_REVIEW_habdallah_DONE use SDS_CHAR*/
    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/
    GTP_LOG_ID = LOGGER_REGISTER("GTP");
    LOG_ENTER("GTP_init");


    SKL_MESSAGE_HANDLER_REGISTER(INTERCOMP_MSG_ID_GTP_OPEN_TUNNEL_REQ,
                                 GTP_openTunnel_Req_Handler,
                                 NULL);

    SKL_MESSAGE_HANDLER_REGISTER(INTERCOMP_MSG_ID_GTP_UPDATE_TUNNEL_REQ,
                                 GTP_updateTunnel_Req_Handler,
                                 NULL);

    SKL_MESSAGE_HANDLER_REGISTER(INTERCOMP_MSG_ID_GTP_CLOSE_TUNNEL_REQ,
                                 GTP_closeTunnel_Req_Handler,
                                 NULL);

    /* Register reset handler */
    SKL_MESSAGE_HANDLER_REGISTER(INTERCOMP_MSG_ID_GTP_U_RESET,
                                 GTP_reset_Handler,
                                 NULL);

    /* Register destroy handler */
    SKL_MESSAGE_HANDLER_REGISTER(INTERCOMP_MSG_ID_GTP_U_DESTROY,
                                 GTP_destroy_Handler,
                                 NULL);

    /* Register destroy handler */
    SKL_MESSAGE_HANDLER_REGISTER(INTERCOMP_MSG_ID_GTP_T3_TIMER_EXPIRED,
                                 GTP_T3_TimerExpRoutine,
                                 NULL);


    /* Register skeleton message handler */
    QUEUE_MANAGER_REGISTER_HANDLER(QUEUE_ID_RX_PDCP_GTP,
                                 GTP_Rx_PDCP_data_Handler);

    /* Register skeleton message handler */
    QUEUE_MANAGER_REGISTER_HANDLER(QUEUE_ID_TX_GTP_AL_TO_GTP,
                                 GTP_Rx_NWK_data_Handler);

    /*Cross_Review_R2.0_vsafwat: I renamed this queue ID to become:QUEUE_ID_RX_PDCP_GTP_S_ENB_FWD
     * notice that GTP also will have another queue that needs to be registered in which
     * PDCP_TX posts forwarded TX_SDUs (its name in queue manager is QUEUE_ID_TX_PDCP_GTP_S_ENB_FWD
     */
    /* Register skeleton message handler */
    QUEUE_MANAGER_REGISTER_HANDLER(QUEUE_ID_TX_PDCP_GTP_S_ENB_FWD,
                                   GTP_Rx_PDCP_TX_FWD_data_Handler);

    /* Register skeleton message handler */
    QUEUE_MANAGER_REGISTER_HANDLER(QUEUE_ID_RX_PDCP_GTP_S_ENB_FWD,
                                   GTP_Rx_PDCP_RX_FWD_data_Handler);

    /* DONE_WALK_THROUGH_R2.0_ratef_Oct 17, 2010: allocate g_GTP_Ptr in global to be
     * allocated in init
     */
    /* Initialize private data*/
    SKL_MEMORY_ALLOC(SKL_GET_MEMORY_SEGMENT_FOR_TASK(),  sizeof(*g_GTP_Ptr) ,g_GTP_Ptr);

    /* Setting the maximum number of ERABS*/
    g_GTP_Ptr->tunnelRecordsNumber = a_msg_Ptr->maxBearerIndex/*maximumNumberOf_RBs*/;

    /* Allocate GTP_TunnelsRecord with the maximum number ERABS sent in the configurations*/
    /*CROSS_REVIEW_habdallah_DONE, we need to allocate array of pointers and pool, this array should
     * be allocated with max bearer index which should passed by OCM, check RLC_TX as an example*/

    /* DONE_WALK_THROUGH_R2.0_ratef_Oct 17, 2010: it is not correct to allocate the
     * pool with a_msg_Ptr->maxBearerIndex. it has to be allocated by  maximumNumberOf_RBs
     * */
    SKL_FIXED_SIZE_POOL_INIT(g_GTP_Ptr->tunnelRecordsPool_Ptr,
                                 DEDICATED,
                                 SKL_GET_MEMORY_SEGMENT_FOR_TASK(),
                                 (a_msg_Ptr->maximumNumberOf_RBs),
                                 sizeof(GTP_TunnelRecord),
                                 "GTP Tunnel records pool");

    /* DONE_WALK_THROUGH_R2.0_ratef_Oct 17, 2010: size of pool = (maximumNumberOf_RBs * 2 to account for bi-dir RBs)+
     *  (new config:concurrent forwarding bearers *2)
     */
    SKL_FIXED_SIZE_POOL_INIT(g_GTP_Ptr->tunnelContextPool_Ptr,
                                 DEDICATED,
                                 SKL_GET_MEMORY_SEGMENT_FOR_TASK(),
                                 NUMBER_2 * ( a_msg_Ptr->maximumNumberOf_RBs +
                                                 a_msg_Ptr->maximumNumberOfConcurrentForwardingBearers ),
                                 sizeof(GTP_TunnelContext),
                                 "GTP Tunnel contexts pool");

    /*UE records Pool*/
    SKL_FIXED_SIZE_POOL_INIT(g_GTP_Ptr->UERecordsPool_Ptr,
                                 DEDICATED,
                                 SKL_GET_MEMORY_SEGMENT_FOR_TASK(),
                                 a_msg_Ptr->maxNumberOfUEs,
                                 sizeof(GTP_UE_Record),
                                 "GTP UE records pool");

    g_GTP_Ptr->numberOfUEs = a_msg_Ptr->maxNumberOfUEs;

    /* Allocate TunnelRecord array with maximum number of bearer index */
    SKL_MEMORY_ALLOC(SKL_GET_MEMORY_SEGMENT_FOR_TASK(),
                         (a_msg_Ptr->maxBearerIndex)*(sizeof(TunnelRecord*)),
                         g_GTP_Ptr->tunnelsRecord);

    /*allocate UE records array*/
    SKL_MEMORY_ALLOC(SKL_GET_MEMORY_SEGMENT_FOR_TASK(),
                         (a_msg_Ptr->maxNumberOfUEs)*(sizeof(GTP_UE_Record*)),
                         g_GTP_Ptr->UEsRecordsArray_Ptr);

    /* Initialize timer T3_RESPONSE as a periodic timer with period T3_RESPONSE_PERIOD*/

    SKL_TIMER_INIT(g_GTP_Ptr->T3_timer_Ptr, WRP_TT_PERIODIC, "T3_Timer");

    /* Initialize the Echo requests and response handling list*/
    SKL_LINKED_LIST_INIT(g_GTP_Ptr->echoPath_List_Ptr,"GTP path Echo list");

    /* DONE_WALK_THROUGH_R2.0_ratef_Oct 17, 2010: new config: maxNmberOfActiveGTP_Paths to allocate this pool*/
    SKL_FIXED_SIZE_POOL_INIT(g_GTP_Ptr->pathEchoRecordsPool_Ptr,
                                 DEDICATED,
                                 SKL_GET_MEMORY_SEGMENT_FOR_TASK(),
                                 a_msg_Ptr->maxNumberOfActiveGTP_Paths,
                                 sizeof(GTP_pathEchoRecord),
                                 "GTP Echo path records");

    g_GTP_Ptr->numberOfPaths = a_msg_Ptr->maxNumberOfActiveGTP_Paths;

    for (i = ZERO; i < a_msg_Ptr->maxNumberOfActiveGTP_Paths; i++)
    {

        SKL_FIXED_SIZE_BUFF_ALLOC(g_GTP_Ptr->pathEchoRecordsPool_Ptr,
                                  DEDICATED,
                                  sizeof(*pathRecord_Ptr),
                                  pathRecord_Ptr);

        SKL_LINKED_LIST_INIT(pathRecord_Ptr->tunnel_List_Ptr,"GTP path Echo list");

        SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->pathEchoRecordsPool_Ptr, DEDICATED, pathRecord_Ptr);

    } /*for*/

    /* Set the eNB own IP address */
    g_GTP_Ptr->eNB_IpAddress = a_msg_Ptr->GTP_ENB_TransportAddress;

    /* Set the T3_Response time*/
    g_GTP_Ptr->T3_RESPONSE_numberOfTicks = a_msg_Ptr->T3_RESPONSE_waitTime / ECHO_TIMER_STEP ;

    /*Set the ECHO request period equivalent ticks */
    g_GTP_Ptr->ECHO_requestPeriod_numberOfTicks = a_msg_Ptr->ECHOrequestPeriod / ECHO_TIMER_STEP;

#ifdef T1_TEST
    g_GTP_Ptr->ECHO_requestPeriod_numberOfTicks = t3_scale_factor;
#endif

    /* Set N3_Requests */
    g_GTP_Ptr->N3_Requests = a_msg_Ptr->N3_Requests;

/*function_exit:*/
    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/
    LOG_EXIT("GTP_init");
}
void GTP_destroy_Handler(GTP_U_destroy *a_msg_Ptr)
{
    SDS_UINT32              i;
    GTP_pathEchoRecord*     removedPathEchoRecord_ptr;
    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/
    LOG_ENTER("GTP_destroy_Handler");

    /* DONE_WALK_THROUGH_R2.0_ratef_Oct 19, 2010: move this cleanup part before
     * calling de-register queues in order t be able to use same handlers
     * to discard the packets instead of serving them
     */
    for (i = 0; i < g_GTP_Ptr->tunnelRecordsNumber; i++)
    {
        if(NULL != g_GTP_Ptr->tunnelsRecord[i])
        {
            if(NULL != g_GTP_Ptr->tunnelsRecord[i]->DL_Tunnel_Ptr)
            {
                /* Release the context*/
                SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->tunnelContextPool_Ptr,
                    DEDICATED,
                    g_GTP_Ptr->tunnelsRecord[i]->DL_Tunnel_Ptr);
            }
            if(NULL != g_GTP_Ptr->tunnelsRecord[i]->FWD_DL_Tunnel_Ptr)
            {
                /* Release the context*/
                SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->tunnelContextPool_Ptr,
                    DEDICATED,
                    g_GTP_Ptr->tunnelsRecord[i]->FWD_DL_Tunnel_Ptr);
            }
            if(NULL != g_GTP_Ptr->tunnelsRecord[i]->UL_Tunnel_Ptr)
            {
                /* Release the context*/
                SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->tunnelContextPool_Ptr,
                    DEDICATED,
                    g_GTP_Ptr->tunnelsRecord[i]->UL_Tunnel_Ptr);
            }
            if(NULL != g_GTP_Ptr->tunnelsRecord[i]->FWD_UL_Tunnel_Ptr)
            {
                /* Release the context*/
                SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->tunnelContextPool_Ptr,
                    DEDICATED,
                    g_GTP_Ptr->tunnelsRecord[i]->FWD_UL_Tunnel_Ptr);
            }

            LOG_BRANCH("Remove the record");
            /* Release the context*/
            SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->tunnelRecordsPool_Ptr,
                DEDICATED,
                g_GTP_Ptr->tunnelsRecord[i]);
        } /*if(Remove the record)*/
    } /*for*/

    for (i = ZERO; i < g_GTP_Ptr->numberOfUEs; i++)
    {
        if(NULL != g_GTP_Ptr->UEsRecordsArray_Ptr[i])
        {
            LOG_BRANCH("UE Record exist");
            SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->UERecordsPool_Ptr,
                                        DEDICATED,
                                        g_GTP_Ptr->UEsRecordsArray_Ptr[i]);
        } /*if(UE Record exist)*/
    } /*for*/

    /* DONE_WALK_THROUGH_R2.0_ratef_Oct 19, 2010: move queue de-registration here */

    /* De-register skeleton message handler */
    QUEUE_MANAGER_DEREGISTER_HANDLER(QUEUE_ID_RX_PDCP_GTP,
                    GTP_Rx_PDCP_data_Handler);

    /* De-register skeleton message handler */
    QUEUE_MANAGER_DEREGISTER_HANDLER(QUEUE_ID_TX_GTP_AL_TO_GTP,
                    GTP_Rx_NWK_data_Handler);


    /* De-register skeleton message handler */
    QUEUE_MANAGER_DEREGISTER_HANDLER(QUEUE_ID_TX_PDCP_GTP_S_ENB_FWD,
                                   GTP_Rx_PDCP_TX_FWD_data_Handler);

    /* De-register skeleton message handler */
    QUEUE_MANAGER_DEREGISTER_HANDLER(QUEUE_ID_RX_PDCP_GTP_S_ENB_FWD,
                                   GTP_Rx_PDCP_RX_FWD_data_Handler);

    /* Destroy Tunnel Context Pool */
    SKL_FIXED_SIZE_POOL_DESTROY(g_GTP_Ptr->tunnelContextPool_Ptr,DEDICATED);

    /* Destroy Tunnel Record Pool */
    SKL_FIXED_SIZE_POOL_DESTROY(g_GTP_Ptr->tunnelRecordsPool_Ptr,DEDICATED);

    /*UE records pool*/
    SKL_FIXED_SIZE_POOL_DESTROY(g_GTP_Ptr->UERecordsPool_Ptr,DEDICATED);

    /* De-allocate GTP_TunnelsRecord */
    SKL_MEMORY_FREE(g_GTP_Ptr->tunnelsRecord);

    /*Deallocate UE records array*/
    SKL_MEMORY_FREE(g_GTP_Ptr->UEsRecordsArray_Ptr);

    /* DONE_WALK_THROUGH_R2.0_ratef_Oct 19, 2010: move the destroy of echoPath_List_Ptr after
     * releasing all elements
     */

    /* Releasing all allocated echo path records buffers */
    SKL_LINKED_LIST_GET_AND_REMOVE_FIRST(g_GTP_Ptr->echoPath_List_Ptr,removedPathEchoRecord_ptr);
    while(NULL != removedPathEchoRecord_ptr)
    {
        /* Release the allocated structure*/
        SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->pathEchoRecordsPool_Ptr, DEDICATED, removedPathEchoRecord_ptr);

        SKL_LINKED_LIST_GET_AND_REMOVE_FIRST(g_GTP_Ptr->echoPath_List_Ptr,removedPathEchoRecord_ptr);

    } /*while(NULL != removedPathEchoRecord_ptr)*/

    for (i = ZERO; i < g_GTP_Ptr->numberOfPaths; i++)
    {
        SKL_FIXED_SIZE_BUFF_ALLOC(g_GTP_Ptr->pathEchoRecordsPool_Ptr,
                                  DEDICATED,
                                  sizeof(*removedPathEchoRecord_ptr),
                                  removedPathEchoRecord_ptr);

        SKL_LINKED_LIST_DESTROY(removedPathEchoRecord_ptr->tunnel_List_Ptr);

        SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->pathEchoRecordsPool_Ptr, DEDICATED, removedPathEchoRecord_ptr);
    } /*for*/

    /* Destroy Echo path Records pool */
    SKL_FIXED_SIZE_POOL_DESTROY(g_GTP_Ptr->pathEchoRecordsPool_Ptr,DEDICATED);

    /* Destroy the ECHO paths linked list */
    SKL_LINKED_LIST_DESTROY(g_GTP_Ptr->echoPath_List_Ptr);

    /*DONE_walkthrough_R2.0_Oct 31, 2010_root: stop T3_timer_Ptr before destroy */
    /*Stop the timer*/
    SKL_TIMER_STOP(g_GTP_Ptr->T3_timer_Ptr);

    /*Destroy the timer*/
    SKL_TIMER_DESTROY(g_GTP_Ptr->T3_timer_Ptr);

    /* DONE_WALK_THROUGH_R2.0_ratef_Oct 19, 2010: destroy the GTP global structure*/
    /* Free the component structure */
    SKL_MEMORY_FREE(g_GTP_Ptr);

    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/
    LOG_EXIT("GTP_destroy_Handler");
}

void GTP_reset_Handler(GTP_U_reset *a_msg_Ptr)
{
    SDS_UINT32              i;
    GTP_pathEchoRecord*     removedPathEchoRecord_ptr;
    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/

    /* DONE_WALK_THROUGH_R2.0_ratef_Oct 19, 2010: in case of reset , same as destroy
     * except no need to destroy the lists and the records' pool or the global structure/timer
     */

    LOG_ENTER("GTP_reset_Handler");

    for (i = 0; i < g_GTP_Ptr->tunnelRecordsNumber; i++)
    {
        if(NULL != g_GTP_Ptr->tunnelsRecord[i])
        {
            if(NULL != g_GTP_Ptr->tunnelsRecord[i]->DL_Tunnel_Ptr)
            {
                /* Release the context*/
                SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->tunnelContextPool_Ptr,
                    DEDICATED,
                    g_GTP_Ptr->tunnelsRecord[i]->DL_Tunnel_Ptr);
            }
            if(NULL != g_GTP_Ptr->tunnelsRecord[i]->FWD_DL_Tunnel_Ptr)
            {
                /* Release the context*/
                SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->tunnelContextPool_Ptr,
                    DEDICATED,
                    g_GTP_Ptr->tunnelsRecord[i]->FWD_DL_Tunnel_Ptr);
            }
            if(NULL != g_GTP_Ptr->tunnelsRecord[i]->UL_Tunnel_Ptr)
            {
                /* Release the context*/
                SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->tunnelContextPool_Ptr,
                    DEDICATED,
                    g_GTP_Ptr->tunnelsRecord[i]->UL_Tunnel_Ptr);
            }
            if(NULL != g_GTP_Ptr->tunnelsRecord[i]->FWD_UL_Tunnel_Ptr)
            {
                /* Release the context*/
                SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->tunnelContextPool_Ptr,
                    DEDICATED,
                    g_GTP_Ptr->tunnelsRecord[i]->FWD_UL_Tunnel_Ptr);
            }

            LOG_BRANCH("Remove the record");
            /* Release the context*/
            SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->tunnelRecordsPool_Ptr,
                DEDICATED,
                g_GTP_Ptr->tunnelsRecord[i]);
        } /*if(Remove the record)*/
    } /*for*/

    for (i = ZERO; i < g_GTP_Ptr->numberOfUEs; i++)
    {
        if(NULL != g_GTP_Ptr->UEsRecordsArray_Ptr[i])
        {
            LOG_BRANCH("UE Record exist");
            SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->UERecordsPool_Ptr,
                                        DEDICATED,
                                        g_GTP_Ptr->UEsRecordsArray_Ptr[i]);
        } /*if(UE Record exist)*/
    } /*for*/

    /* Reset skeleton message handler */
    QUEUE_MANAGER_RESET_QUEUE(QUEUE_ID_RX_PDCP_GTP,
                                GTP_Rx_PDCP_data_Handler);

    /* Reset skeleton message handler */
    QUEUE_MANAGER_RESET_QUEUE(QUEUE_ID_TX_GTP_AL_TO_GTP,
                                GTP_Rx_NWK_data_Handler);


    /* Reset skeleton message handler */
    QUEUE_MANAGER_RESET_QUEUE(QUEUE_ID_TX_PDCP_GTP_S_ENB_FWD,
                                GTP_Rx_PDCP_TX_FWD_data_Handler);

    /* Reset skeleton message handler */
    QUEUE_MANAGER_RESET_QUEUE(QUEUE_ID_RX_PDCP_GTP_S_ENB_FWD,
                                   GTP_Rx_PDCP_RX_FWD_data_Handler);

    /* Releasing all allocated echo path records buffers */
    SKL_LINKED_LIST_GET_AND_REMOVE_FIRST(g_GTP_Ptr->echoPath_List_Ptr,removedPathEchoRecord_ptr);
    while(NULL != removedPathEchoRecord_ptr)
    {
        /* Release the allocated structure*/
        SKL_FIXED_SIZE_BUFF_RELEASE(g_GTP_Ptr->pathEchoRecordsPool_Ptr, DEDICATED, removedPathEchoRecord_ptr);

        SKL_LINKED_LIST_GET_AND_REMOVE_FIRST(g_GTP_Ptr->echoPath_List_Ptr,removedPathEchoRecord_ptr);

    } /*while(NULL != removedPathEchoRecord_ptr)*/

    /*Stop the timer*/
    SKL_TIMER_STOP(g_GTP_Ptr->T3_timer_Ptr);

    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/
    LOG_EXIT("GTP_reset_Handler");
}
void GTP_Rx_NWK_data_Handler(SDS_UINT32 a_SDUsToServe)
{
    GTP_SDU*        receivedGTP_desc_Ptr;
    GTP_HEADER      gtpPacket;
    SDS_UINT32      decodedBytesNumber;
    SDS_UINT8*      packetStart_Ptr;
    SDS_UINT32      payloadOffset;
    SDS_UINT32      processedBytesNumber;
    SDS_BOOL        decodeResult;
    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/
    LOG_ENTER("GTP_Rx_NWK_data_Handler");

    /* Initialize the Ptr */
    receivedGTP_desc_Ptr = NULL;

    LOG_ASSERT(a_SDUsToServe > 0, "SDUs requested to be handled <= 0");
    LOG_TRACE_ARG("no of GTP SDUs = %d",a_SDUsToServe,0,0);
    do
    {

        /* Extract the SDU from queue */
        QUEUE_MANAGER_DEQUEUE(QUEUE_ID_TX_GTP_AL_TO_GTP, receivedGTP_desc_Ptr);
        LOG_ASSERT(NULL != receivedGTP_desc_Ptr, "GTP_AL TX Queue is empty");

        LOG_TRACE("Handle Data from GTP_AL");


        /* Initialize processedBytesNumber */
        processedBytesNumber = ZERO;
        /* Initialize tempEnqueuedSDUsCount*/
        g_GTP_Ptr->tempEnqueuedSDUsCount = ZERO;

        while(receivedGTP_desc_Ptr->rawData_buffer_Length > processedBytesNumber)
        {
            /* Set the start of the new packet to be handled*/
            packetStart_Ptr = receivedGTP_desc_Ptr->rawData_buffer_Ptr + processedBytesNumber;

            /*DONE_walkthrough_R2.0_Oct 25, 2010_root: decode and process for n packets
             * as long as the receivedLength is non-zero (while on receivedlength)
             */
            /* Calling the decode function to decode the received packet */
            decodedBytesNumber = GTP_decodePacket(packetStart_Ptr,receivedGTP_desc_Ptr->rawData_buffer_Length-processedBytesNumber,
                &gtpPacket, &decodeResult);

            /* Update processedBytesNumber */
            processedBytesNumber += decodedBytesNumber;

            if(FALSE == decodeResult)
            {
                LOG_BRANCH("Failed to decode GTP message");
                continue;
            } /*if(Failed to decode GTP message)*/

            /* Set the payload offset which indicates the length of the decoded GTP header (including mandatory and optional header fields)*/
            payloadOffset = decodedBytesNumber - gtpPacket.length;

            /*DONE_walkthrough_R2.0_Oct 25, 2010_root: processPacket should take the
             * - pointer to address received GTP_desc_Ptr->rawData_buffer
             * - length of the decoded header
             * - payload length (already in gtpPacket)
             */
            /* Process the packet*/
            GTP_processPacket(&gtpPacket,packetStart_Ptr,payloadOffset,receivedGTP_desc_Ptr);

        } /*while(receivedGTP_desc_Ptr->rawData_buffer_Length > processedBytesNumber)*/

        /*DONE_walkthrough_R2.0_Oct 25, 2010_root: called outside the while loop of decode */
        receivedGTP_desc_Ptr->enqueuedSDUsCount = g_GTP_Ptr->tempEnqueuedSDUsCount;
        /* Free the Descriptor */
        FREE_GTP_SDU(receivedGTP_desc_Ptr);

        a_SDUsToServe--;

    } while(a_SDUsToServe !=0);
    /*CROSS_REVIEW_habdallah_DONE apply guildlines on while condition*/


/*function_exit:*/
    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/
    LOG_EXIT("GTP_Rx_NWK_data_Handler");
}
void GTP_Rx_PDCP_data_Handler(SDS_UINT32 a_SDUsToServe)
{

    RX_RLC_SDU_Desc*                receivedRLC_desc_Ptr;
    SDS_Status                      status;
    SDS_Status                      transmissionStatus;
    GTP_HEADER                      gtpPacket;
    GTP_TunnelContext*              ulTunnel_Ptr;
    SDS_BOOL32                      tunnelRecordExists;
    SDS_INT32                       encodedHeaderLength;
    SDS_UINT8*                      startOfPacket;
    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/

    /*CROSS_REVIEW_habdallah_DONE when dealing with data use SDS_UINT8 instead of SDS_CHAR*/

    LOG_ENTER("GTP_Rx_PDCP_data_Handler");

    LOG_ASSERT(a_SDUsToServe > 0, "SDUs requested to be handled <= 0");

    /* Fill the constant part in the GTP header */
    FILL_CONSTANT_PART_IN_GTP_PACKET(gtpPacket);

    /*CROSS_REVIEW_habdallah_DONE ? As no memory allocation is allowed, we may have fixed buffer to
     * use */
    do
    {
        /* Extract the SDU from queue */
        QUEUE_MANAGER_DEQUEUE(QUEUE_ID_RX_PDCP_GTP, receivedRLC_desc_Ptr);
        LOG_ASSERT(NULL != receivedRLC_desc_Ptr, "RX_PDCP_GTP Queue is empty");

        ulTunnel_Ptr = NULL ;
        tunnelRecordExists = (NULL != g_GTP_Ptr->tunnelsRecord[receivedRLC_desc_Ptr->bearerIndex]);
        if(TRUE == tunnelRecordExists)
        {
            /* Get a pointer to the tunnel context*/
            ulTunnel_Ptr = g_GTP_Ptr->tunnelsRecord[receivedRLC_desc_Ptr->bearerIndex]->UL_Tunnel_Ptr;
        } /*if(if_branch_string)*/

        if((TRUE == tunnelRecordExists) && (NULL !=ulTunnel_Ptr))
        {
            LOG_BRANCH("UL tunnel with the specified TEID exists");

            /* Set msg_Ptr in GTP Packet to the raw data + offset */

            /* Set the length of the data in the GTP packet  */
            gtpPacket.length = receivedRLC_desc_Ptr->rawDataLength;

            /* Set the TEID in the GTP Packet*/
            gtpPacket.TEID = ulTunnel_Ptr->TEID;

            /* Set the Sequence number and increment the expected sequence number for next time */
            gtpPacket.sequenceNumber = ulTunnel_Ptr->expectedSeqNumber++;

            /* Reset encoded header length */
            encodedHeaderLength = ZERO;

            /* Call encode function  */
            encodedHeaderLength = GTP_encodePacket(&gtpPacket,
                                                    receivedRLC_desc_Ptr->rawDataBuff_Ptr,
                                                    receivedRLC_desc_Ptr->data_Offset);

            if(encodedHeaderLength != -1)
            {
                /* Set the pointer to the start of the GTP packet*/
                startOfPacket = receivedRLC_desc_Ptr->rawDataBuff_Ptr + receivedRLC_desc_Ptr->data_Offset - encodedHeaderLength;

                /* Call upper layer  function to send GTP packet */
                GTP_TX_NWK(startOfPacket, receivedRLC_desc_Ptr->rawDataLength+encodedHeaderLength ,
                    (&(ulTunnel_Ptr->pathRecord_Ptr->pathAddress)), FALSE,&transmissionStatus) ;

                if(WRP_SUCCESS == transmissionStatus)
                {
                    LOG_BRANCH("Packet is transmitted successfully");
                    STATS_GTP_INCREMENT_TOTAL_SENT_PACKETS(receivedRLC_desc_Ptr->bearerIndex);
                } /*if(Packet is transmitted successfully)*/
                else
                {
                    LOG_BRANCH("Failed to send data on socket");
                    STATS_GTP_RX_INCREMENT_DROPPED_PACKETS(receivedRLC_desc_Ptr->bearerIndex);
                } /*else (Failed to send data on socket)*/

            } /*if(Header created successfully )*/
            else
            {
                LOG_ERROR("Error occurred , Failed to create GTP header because Buffer offset is not enough! ");
                /*statistics - dropped packet */
                STATS_GTP_RX_INCREMENT_DROPPED_PACKETS(receivedRLC_desc_Ptr->bearerIndex);
            } /*else (else_branch_string)*/
        } /*if (UL tunnel with the specified TEID exists)*/
        else
        {
            LOG_BRANCH("Invalid GTP TEID");
            /* The GTP tunnel was not established */
            /* statistics - dropped packet because tunnel is closed */
            STATS_GTP_RX_INCREMENT_DROPPED_PACKETS(receivedRLC_desc_Ptr->bearerIndex);
        } /*else (Invalid GTP TEID)*/

        /* After posting the buffer to be sent by the test layer
         * we can now put the RX_RLC_SDU_Desc descriptor to the free queue QUEUE_ID_RX_PDCP_FREE
         * using QUEUE_MANAGER_ENQUEUE macro
         */
        QUEUE_MANAGER_ENQUEUE(QUEUE_ID_RX_PDCP_FREE, receivedRLC_desc_Ptr, &status);
        if(WRP_SUCCESS != status)
        {
            LOG_EXCEPTION("Failed to Enqueue RX_RLC_SDU_Desc in PDCP_RX Free queue");
            /*DONE_CROSS_REVIEW_habdallah * needs to free data*/
        }

        LOG_TRACE("Freeing SDU Descriptor");
        a_SDUsToServe--;

    } while(a_SDUsToServe !=0);


    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/

    LOG_EXIT("GTP_Rx_PDCP_data_Handler");
}
void GTP_Rx_PDCP_TX_FWD_data_Handler(SDS_UINT32 a_SDUsToServe)
{

    TX_RLC_SDU_Desc*                receivedRLC_desc_Ptr;
    GTP_HEADER                      gtpPacket;
    GTP_TunnelContext*              fwdDlTunnel_Ptr;
    SDS_BOOL32                      tunnelRecordExist;
    RRC_DataForwardingFinished*     RRC_DataForwardingFinished_Ptr;
    SDS_INT32                       encodedHeaderLength;
    SDS_UINT8*                      startOfPacket;
    SDS_BOOL                        MSG_MORE_flag;
    SDS_Status                      transmissionStatus;

    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/

    LOG_ENTER("GTP_Rx_PDCP_TX_data_Handler");

    LOG_ASSERT(a_SDUsToServe > 0, "SDUs requested to be handled <= 0");

    /**************** Fill the constant part in GTP_PACKET ******************/
    /*DONE_walkthrough_R2.0_Oct 24, 2010_root: use sequenceNumberFlag */
    FILL_CONSTANT_PART_IN_GTP_PACKET(gtpPacket);

    /* PDCP SN required only in FWD case */
    gtpPacket.extensionHeadersArray[ZERO].type = PDCP_PDU_NUMBER;

    gtpPacket.extensionHeadersArray[ZERO].length = GTP_PDCP_EXTENSION_HEADER_LENGTH;
    gtpPacket.extensionHeadersArray[NUMBER_1].type = NO_MORE_HEADERS;

    /************************************************************************/

    do
    {
        /*Cross_Review_R2.0_vsafwat: use QUEUE_ID_RX_PDCP_GTP_S_ENB_FWD .. see comment in init */
        /* Extract the SDU from queue */
        QUEUE_MANAGER_DEQUEUE(QUEUE_ID_TX_PDCP_GTP_S_ENB_FWD, receivedRLC_desc_Ptr);
        LOG_ASSERT(NULL != receivedRLC_desc_Ptr, "TX_PDCP_GTP_S_ENB_FWD Queue is empty");


        /* DONE_WALK_THROUGH_R2.0_ratef_Oct 19, 2010: if g_GTP_Ptr->tunnelsRecord[bearerIndex] = NULL
         * you can't access the FWD_DL_Tunnel_Ptr
         * apply in all other cases
         */

        fwdDlTunnel_Ptr = NULL;

        tunnelRecordExist = (g_GTP_Ptr->tunnelsRecord[receivedRLC_desc_Ptr->bearerIndex] != NULL);
        if (TRUE == tunnelRecordExist)
        {
            /* Get a pointer to the tunnel context*/
            fwdDlTunnel_Ptr = g_GTP_Ptr->tunnelsRecord[receivedRLC_desc_Ptr->bearerIndex]->FWD_DL_Tunnel_Ptr;
        }

        /* Check if the specified tunnel exist */
        if(NULL != fwdDlTunnel_Ptr)
        {
            LOG_BRANCH("DL tunnel with the specified TEID exists");

            /* -------------- Fill GTP Packet --------------------*/

            /* Set the length of the data in the GTP packet */
            gtpPacket.length = receivedRLC_desc_Ptr->rawDataLength;

            /* Set the TEID in the GTP Packet*/
            gtpPacket.TEID = fwdDlTunnel_Ptr->TEID;

            /* Set the Sequence number */
            gtpPacket.sequenceNumber = fwdDlTunnel_Ptr->expectedSeqNumber++;

            /* Check if we should put SN in the packet */
            if(TRUE == receivedRLC_desc_Ptr->read_SN)
            {
                LOG_BRANCH("Should read SN");
                /* Add PDCP sequence number */
                gtpPacket.extensionHeadersArray[ZERO].content =
                    PDCP_CALCULATE_SN_FROM_COUNT(receivedRLC_desc_Ptr->count,STD_DRB_AM_SN_LENGTH);
                /* Set the extension header flag to true*/
                gtpPacket.extensionFlag = TRUE;
            } /*if(Should read SN)*/
            /* -------------------------------------------------- */

            /* Handle End Marker SDU */
            if(receivedRLC_desc_Ptr->isEndMarker == TRUE)
            {
                LOG_BRANCH("SDU is End Marker");
                gtpPacket.msgType = GTP_MSG_TYPE_END_MARKER;


                /* TODO_memad : maybe changed to a macro as its also used in GTP_RX_PDCP_RX*/
                /*-------- Sending RRC_DataForwardingFinished to RRC ----------*/
                SKL_ALLOC_INTERCOMP_MESSAGE(RRC_DataForwardingFinished_Ptr);

                LOG_ASSERT_WITH_EXCEPTION(EXCEPTION_INTER_COMP_MSG_ALLOC_FAILED,
                    NULL != RRC_DataForwardingFinished_Ptr,
                    "Failed to allocate inter component message");

                /*DONE_walkthrough_R2.0_Oct 25, 2010_root: this is DL forwarding
                 * DL forwarding = true and UL forward = false
                 */
                /* Set the flag that indicates that the  */
                RRC_DataForwardingFinished_Ptr->isDL_forwardingFinished = TRUE;

                RRC_DataForwardingFinished_Ptr->isDL_forwardingFinished = FALSE;

                /* Set the radio bearer */
                RRC_DataForwardingFinished_Ptr->rb_Index = receivedRLC_desc_Ptr->bearerIndex;

                SEND_RRC_DATA_FORWARDING_FINISHED(RRC_DataForwardingFinished_Ptr,
                    SKL_SENDING_METHOD_DEFAULT);
                /*-------------------------------------------------------------*/

                /*DONE_walkthrough_R2.0_Oct 25, 2010_root: don't free the end marker
                 * it has to be kept a PDCP_TX
                 */

            } /*if(SDU is End Marker)*/

            /*DONE_walkthrough_R2.0_Oct 25, 2010_root: move encode and end to NWK outside
             * the if/else
             */

            /* NOTE:
             * Here a problem arouses which is that the offset in the handled descriptor will not be enough to fit the GTP header with the extension header
             * As a solution to this problem, we use the MSG_MORE flag option in the sendto() API. This option allows us to send data indicating that there
             * will be more data related to it to follow, hence the data is not sent immediately on the socket,but will be concatenated with the comming one
             * until we call the sendto() without specifying the MSG_MORE option flag.
             */

            /* First we send the GTP Header */
            /* Call encode function  */
            encodedHeaderLength = GTP_encodePacket(&gtpPacket ,g_GTP_Ptr->tempGTPheaderBuff,sizeof(g_GTP_Ptr->tempGTPheaderBuff));

            if(encodedHeaderLength != -1)
            {
                /* Set the pointer to the start of the GTP Header*/
                startOfPacket = g_GTP_Ptr->tempGTPheaderBuff + sizeof(g_GTP_Ptr->tempGTPheaderBuff) - encodedHeaderLength;

                if (ZERO != receivedRLC_desc_Ptr->rawDataLength)
                {
                    MSG_MORE_flag = TRUE;
                }
                else
                {
                    MSG_MORE_flag = FALSE;
                }

                /* Call upper layer  function to send GTP Header with the MSG_MORE_Flag set to TRUE, as the data will be sent next */
                GTP_TX_NWK(startOfPacket,encodedHeaderLength ,(&(fwdDlTunnel_Ptr->pathRecord_Ptr->pathAddress)),MSG_MORE_flag, &transmissionStatus) ;

                if(WRP_SUCCESS == transmissionStatus)
                {
                    LOG_BRANCH("Packet is transmitted successfully");
                    STATS_GTP_INCREMENT_TOTAL_SENT_PACKETS(receivedRLC_desc_Ptr->bearerIndex);
                } /*if(Packet is transmitted successfully)*/
                else
                {
                    LOG_BRANCH("Failed to send data on socket");
                    STATS_GTP_RX_INCREMENT_DROPPED_PACKETS(receivedRLC_desc_Ptr->bearerIndex);
                } /*else (Failed to send data on socket)*/

            } /*if(Header created successfully  )*/
            else
            {
                LOG_ERROR("Error occurred , Failed to encode GTP header because Buffer size is not enough! ");
                /* TODO_memad : statistics - dropped packet */
            } /*else (else_branch_string)*/

            if(ZERO != receivedRLC_desc_Ptr->rawDataLength)
            {
                LOG_BRANCH("the data length is not zero case end marker ");
                /* Now we send the data */
                /* Set the pointer to the start of the data to be sent*/
                startOfPacket = receivedRLC_desc_Ptr->rawDataBuff_Ptr + receivedRLC_desc_Ptr->rawDataOffset;

                /* Call upper layer  function to send data. Notice that the MSG_MORE_Flag is set to FALSE, which means that the GTP_AL will concatenate
                 * this data to the previous GTP header and send it as a complete packet   */
                GTP_TX_NWK(startOfPacket,receivedRLC_desc_Ptr->rawDataLength ,(&(fwdDlTunnel_Ptr->pathRecord_Ptr->pathAddress)),FALSE, &transmissionStatus) ;

                if(WRP_SUCCESS == transmissionStatus)
                {
                    LOG_BRANCH("Packet is transmitted successfully");
                    STATS_GTP_INCREMENT_TOTAL_SENT_PACKETS(receivedRLC_desc_Ptr->bearerIndex);
                } /*if(Packet is transmitted successfully)*/
                else
                {
                    LOG_BRANCH("Failed to send data on socket");
                    STATS_GTP_RX_INCREMENT_DROPPED_PACKETS(receivedRLC_desc_Ptr->bearerIndex);
                } /*else (Failed to send data on socket)*/

            } /*if(the data length is not zero case end marker )*/
        } /*if (UL tunnel with the specified TEID exists)*/
        else
        {
            LOG_BRANCH("Invalid GTP TEID, tunnel not opened ");
            /* The GTP tunnel was not established */
            /* TODO_memad : statistics - dropped packet because tunnel is closed */
        } /*else (Invalid GTP TEID)*/


#if 0
        /* After posting the buffer to be sent by the test layer
         * we can now put the RX_RLC_SDU_Desc descriptor to the free queue QUEUE_ID_RX_PDCP_FREE
         * using QUEUE_MANAGER_ENQUEUE macro
         */
        /*Cross_Review_R2.0_vsafwat: there is no special free queue for forwarding
         * you need to use the normal free queue for QUEUE_ID_RX_PDCP_FREE */
        /*QUEUE_MANAGER_ENQUEUE(QUEUE_ID_RX_PDCP_FWD_FREE, receivedRLC_desc_Ptr, &status);*/
        QUEUE_MANAGER_ENQUEUE(QUEUE_ID_RX_PDCP_FREE, receivedRLC_desc_Ptr, &status);
        if(WRP_SUCCESS != status)
        {
            LOG_EXCEPTION("Failed to Enqueue RX_RLC_SDU_Desc in PDCP_RX Free queue");
        }

        LOG_TRACE("Freeing SDU Descriptor");
#endif
        a_SDUsToServe--;

    } while(a_SDUsToServe !=0);


    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/

    LOG_EXIT("GTP_Rx_PDCP_TX_data_Handler");
}
void GTP_Rx_PDCP_RX_FWD_data_Handler(SDS_UINT32 a_SDUsToServe)
{

    RX_RLC_SDU_Desc*                receivedRLC_desc_Ptr;
    GTP_HEADER                      gtpPacket;
    GTP_TunnelContext*              fwdUlTunnel_Ptr;
    SDS_BOOL32                      tunnelRecordExists;
    RRC_DataForwardingFinished*     RRC_DataForwardingFinished_Ptr;
    SDS_INT32                       encodedHeaderLength;
    SDS_UINT8*                      startOfPacket;
    SDS_Status                      transmissionStatus;
    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/

    LOG_ENTER("GTP_Rx_PDCP_RX_data_Handler");

    LOG_ASSERT(a_SDUsToServe > 0, "SDUs requested to be handled <= 0");

    /**************** Fill the constant part in GTP_PACKET ******************/
    /*DONE_walkthrough_R2.0_Oct 24, 2010_root: same as TX_PDCP fn*/
    FILL_CONSTANT_PART_IN_GTP_PACKET(gtpPacket);

    /* PDCP SN required or only in FWD case */
    gtpPacket.extensionHeadersArray[ZERO].type= PDCP_PDU_NUMBER;

    gtpPacket.extensionFlag = TRUE;

    gtpPacket.extensionHeadersArray[ZERO].length = GTP_PDCP_EXTENSION_HEADER_LENGTH;
    gtpPacket.extensionHeadersArray[NUMBER_1].type = NO_MORE_HEADERS;

    /************************************************************************/

    do
    {
        /* Extract the SDU from queue */
        QUEUE_MANAGER_DEQUEUE(QUEUE_ID_RX_PDCP_GTP_S_ENB_FWD, receivedRLC_desc_Ptr);
        LOG_ASSERT(NULL != receivedRLC_desc_Ptr, "RX_PDCP_GTP_S_ENB_FWD Queue is empty");

        fwdUlTunnel_Ptr= NULL;
        tunnelRecordExists = (NULL != g_GTP_Ptr->tunnelsRecord[receivedRLC_desc_Ptr->bearerIndex]);
        if(TRUE == tunnelRecordExists)
        {
            /* Get a pointer to the tunnel context*/
            fwdUlTunnel_Ptr = g_GTP_Ptr->tunnelsRecord[receivedRLC_desc_Ptr->bearerIndex]->FWD_UL_Tunnel_Ptr;
        } /*if(Tunnel record exists)*/

        /* Check if the specified tunnel exist */
        if(NULL != fwdUlTunnel_Ptr)
        {
            LOG_BRANCH("UL tunnel with the specified TEID exists");

            /* -------------- Fill GTP Packet --------------------*/

            /* Set the length of the data in the GTP packet */
            gtpPacket.length = receivedRLC_desc_Ptr->rawDataLength;

            /* Set the TEID in the GTP Packet*/
            gtpPacket.TEID = fwdUlTunnel_Ptr->TEID;

            /* Set the Sequence number */
            /*DONE_walkthrough_R2.0_Oct 24, 2010_root: the expectedSeqNumber should be U16*/
            gtpPacket.sequenceNumber = fwdUlTunnel_Ptr->expectedSeqNumber++;

            /* Add PDCP sequence number */
            gtpPacket.extensionHeadersArray[ZERO].content = receivedRLC_desc_Ptr->PDCP_RX_SN;

            /* -------------------------------------------------- */


            /* Call encode function  */
            encodedHeaderLength = GTP_encodePacket(&gtpPacket ,
                                                    receivedRLC_desc_Ptr->rawDataBuff_Ptr,
                                                    receivedRLC_desc_Ptr->data_Offset);
            if(encodedHeaderLength != -1)
            {
                /* Set the pointer to the start of the GTP packet*/
                startOfPacket = receivedRLC_desc_Ptr->rawDataBuff_Ptr + receivedRLC_desc_Ptr->data_Offset - encodedHeaderLength;

                /* Call upper layer  function to send GTP packet */
                GTP_TX_NWK(startOfPacket, receivedRLC_desc_Ptr->rawDataLength+encodedHeaderLength ,
                    (&(fwdUlTunnel_Ptr->pathRecord_Ptr->pathAddress)),FALSE, &transmissionStatus) ;

                if(WRP_SUCCESS == transmissionStatus)
                {
                    LOG_BRANCH("Packet is transmitted successfully");
                    STATS_GTP_INCREMENT_TOTAL_SENT_PACKETS(receivedRLC_desc_Ptr->bearerIndex);
                } /*if(Packet is transmitted successfully)*/
                else
                {
                    LOG_BRANCH("Failed to send data on socket");
                    STATS_GTP_RX_INCREMENT_DROPPED_PACKETS(receivedRLC_desc_Ptr->bearerIndex);
                } /*else (Failed to send data on socket)*/

            } /*if(Header created successfully  )*/
            else
            {
                LOG_ERROR("Error occurred , Failed to create GTP header because Buffer offset is not enough! ");
                /*statistics - dropped packet */
                STATS_GTP_RX_INCREMENT_DROPPED_PACKETS(receivedRLC_desc_Ptr->bearerIndex);
            } /*else (else_branch_string)*/


            /* Check if this is the last Forwarding descriptor*/
            if(TRUE == receivedRLC_desc_Ptr->LastForwardingDesc)
            {
                LOG_BRANCH("Last forwarding descriptor received ");
                /* Sending RRC_DataForwardingFinished to RRC*/
                SKL_ALLOC_INTERCOMP_MESSAGE(RRC_DataForwardingFinished_Ptr);

                LOG_ASSERT_WITH_EXCEPTION(EXCEPTION_INTER_COMP_MSG_ALLOC_FAILED,
                    NULL != RRC_DataForwardingFinished_Ptr,
                    "Failed to allocate inter component message");

                /*DONE_walkthrough_R2.0_Oct 25, 2010_root: set DL forward = false */
                /* Set the flag that indicates that the forwarding is finished */
                RRC_DataForwardingFinished_Ptr->isUL_forwardingFinished = TRUE;

                RRC_DataForwardingFinished_Ptr->isDL_forwardingFinished = FALSE;

                /* Set the radio bearer */
                RRC_DataForwardingFinished_Ptr->rb_Index = receivedRLC_desc_Ptr->bearerIndex;

                SEND_RRC_DATA_FORWARDING_FINISHED(RRC_DataForwardingFinished_Ptr,
                    SKL_SENDING_METHOD_DEFAULT);

            } /*if(Last forwarding descriptor received , sending RRC_DataForwardingFinished to RRC)*/
        } /*if (UL tunnel with the specified TEID exists)*/
        else
        {
            LOG_BRANCH("Invalid GTP TEID, tunnel not opened ");
            /* The GTP tunnel was not established */

            /* statistics - dropped packet because tunnel is closed */
            STATS_GTP_RX_INCREMENT_DROPPED_PACKETS(receivedRLC_desc_Ptr->bearerIndex);

        } /*else (Invalid GTP TEID)*/

        a_SDUsToServe--;

    } while(a_SDUsToServe !=0);


    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/

    LOG_EXIT("GTP_Rx_PDCP_RX_data_Handler");
}
SDS_Status GTP_Tx_PDCP(GTP_HEADER* gtpPacket ,SDS_UINT8 *packetStart_Ptr,SDS_UINT32 payloadOffset, GTP_SDU* parentGTP_SDU_Ptr)
{
    TX_RLC_SDU_Desc*    SDU_Desc_Ptr = NULL;
    RX_RLC_SDU_Desc*    RX_SDU_Desc_Ptr = NULL;
    SDS_Status          status = WRP_SUCCESS;
    SDS_UINT16          tunnelTypeIndex;
    SDS_UINT32          i;
    SDS_BOOL32          PDCP_RX_SN_isFilled;
    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/
    LOG_ENTER("GTP_Tx_PDCP");


    /*DONE_walkthrough_R2.0_Oct 25, 2010_root:  end marker is received in the following cases:
     *
     * 1- from S-GW on normal DL tunnel to be passed to PDCP_TX normally
     * 2- at Target eNb on forwarding DL tunnel to be passed to PDCP_TX forward queue


    DONE_walkthrough_R2.0_Oct 25, 2010_root: merge with function GTP_Tx_PDCP as there is
     * a lot of common code


    DONE_walkthrough_R2.0_Oct 27, 2010_root: remember to update this TX_SDU desc with
     * pointer, offset, parent desc, SDU enqueued count, ... */


    /*DONE_CROSS_REVIEW_habdallah ? generally try not to use magic numbers*/
    /* Extracting Tunnel type from TEID */
    tunnelTypeIndex = GET_TUNNEL_TYPE_INDEX_FROM_TEID(gtpPacket->TEID);

    /*DONE_CROSS_REVIEW_habdallah is the the right compare?*/

    /* Check if the Tunnel is not Uplink*/
    if(GTP_FWD_UL_TUNNEL_TEID_INDEX != tunnelTypeIndex )
    {
        /*Sending to PDCP_TX*/
        LOG_BRANCH("Tunnel type is either FWD_DL or Normal DL");

        /* Allocate RLC SDU descriptor*/
        SKL_FIXED_SIZE_BUFF_ALLOC(TX_DATA_POOL_RLC_SDU_DESC,
                                    DEDICATED,
                                    sizeof(*SDU_Desc_Ptr),
                                    SDU_Desc_Ptr);
        LOG_ASSERT_WITH_EXCEPTION(EXCEPTION_TX_DESC_ALLOC_FAILED,
                                  NULL != SDU_Desc_Ptr,
                                  "Failed to allocate TX_RLC_SDU_Desc") ;

        /*DONE_walkthrough_R2.0_Oct 25, 2010_root: add inside SDU_Desc_Ptr pointer to arent
         * GTP_DESC and increment SDU_Count inside GTP_DESC
         */
        /* Set the pointer to the parent GTP_SDU in the TX_RLC_SDU */
        SDU_Desc_Ptr->parentGTP_SDU_Ptr = parentGTP_SDU_Ptr;

        /* Increment Counter */
        g_GTP_Ptr->tempEnqueuedSDUsCount++;

        /*DONE_walkthrough_R2.0_Oct 25, 2010_root:  DO NOT allocate another rawDataBuffr
         * we don't need it here
         */
        /* Set the raw data pointer to the start of the packet */
        SDU_Desc_Ptr->rawDataBuff_Ptr = packetStart_Ptr;

        /*DONE_walkthrough_R2.0_Oct 25, 2010_root: the rawDataOffset = decoded GTP header length (variable not fixed)*/
        /* Set data offset in SDU */
        SDU_Desc_Ptr->rawDataOffset = payloadOffset;

        /* Set the length of the rawData*/
        SDU_Desc_Ptr->rawDataLength = gtpPacket->length;

        /* DONE_walkthrough_R2.0_Oct 25, 2010_root: instead of the memcpy, the
         * SDU_Desc_Ptr->rawDataBuff_Ptr should point to the original rawDataBuff_Ptr
         * passed in processPacket fn
         */

        /*DONE_walkthrough_R2.0_Oct 25, 2010_root: initialize the parameters of the Descriptor
         * by default values: read_SN, isPDCP_freeSDU, isEndMarker , ..etc
         */
        /* Initialize all parameters */
        SDU_Desc_Ptr->read_SN       = FALSE;

        /* Check if the packet was End Marker*/
        if(GTP_MSG_TYPE_END_MARKER == gtpPacket->msgType)
        {
            LOG_BRANCH("End Marker received");
            SDU_Desc_Ptr->isEndMarker   = TRUE;

            /* Hossam : Set the ciphered Data buffer pointer to NULL*/
            SDU_Desc_Ptr->cipheredDataBuff_Ptr = NULL;

        } /*if(End Marker received)*/
        else
        {
            /* SDU is not an End Marker*/
            SDU_Desc_Ptr->isEndMarker   = FALSE;
        }


        /* Fill descriptor parameters */
        /*Pass the allocated new data buffer from TX_DATA_POOL_RAW to the SDU_Desc_Ptr->rawDataBuff_Ptr */
        SDU_Desc_Ptr->rawDataLength = gtpPacket->length;
        SDU_Desc_Ptr->bearerIndex   = ENB_GTP_ID_TO_BEARER_INDEX(gtpPacket->TEID);

        /*Set the timeStamp */
        SDU_Desc_Ptr->timeStamp = GET_SUBFRAME_COUNTER_TX();


    } /*if(tunnel type is not GTP_FWD_UL_TUNNEL)*/
    else
    {
        LOG_BRANCH("Tunnel type is GTP_FWD_UL_TUNNEL");

        /* Allocate RLC SDU descriptor from different pool */
        SKL_FIXED_SIZE_BUFF_ALLOC(RX_DATA_POOL_PDCP_T_FWD_SDU_DESC,
                     SHARED,
                    sizeof(*RX_SDU_Desc_Ptr),
                    RX_SDU_Desc_Ptr);
        LOG_ASSERT_WITH_EXCEPTION(EXCEPTION_RX_DESC_ALLOC_FAILED,
                                  NULL != RX_SDU_Desc_Ptr,
                                  "Failed to allocate RX_RLC_SDU_Desc");

        /* Set the parent GTP_SDU */
        RX_SDU_Desc_Ptr->parent.parentGTP_SDU_Ptr = parentGTP_SDU_Ptr;

        /*DONE_walkthrough_R2.0_Oct 27, 2010_root: you can increment the count in another variable
         * and set it in GTP_SDU desc only after all processing of rawDataBuff is finished
         * such that the PCP/RLC does not reach the free condition by mistake
         * if it handled its SDU desc before end of GTP processing
         */
        /* Increment Counter */
        g_GTP_Ptr->tempEnqueuedSDUsCount++;

        /* Set the raw data pointer to the start of the packet */
        RX_SDU_Desc_Ptr->rawDataBuff_Ptr = packetStart_Ptr;

        /* Set data offset in SDU */
        RX_SDU_Desc_Ptr->data_Offset = payloadOffset;

        /*DONE_walkthrough_R2.0_Oct 25, 2010_root: same comments as above in if condition*/

        /* Fill descriptor parameters */
        /*Pass the allocated new data buffer from TX_DATA_POOL_RAW to the SDU_Desc_Ptr->rawDataBuff_Ptr */
        RX_SDU_Desc_Ptr->rawDataLength = gtpPacket->length;
        RX_SDU_Desc_Ptr->bearerIndex   = ENB_GTP_ID_TO_BEARER_INDEX(gtpPacket->TEID);

        /*DONE_walkthrough_R2.0_Oct 25, 2010_root: change the name of the flag to become: isPDCP_freeSDU */
        /* Set the flag to indicate that the PDCP will not free this SDU*/
        RX_SDU_Desc_Ptr->isPDCP_freeSDU = TRUE;

        /* ----- Set the PDCP Sequence number if found in the GTP packet ------ */

        /* Assert that the PDCP PDU exist as the PDCP_RX_SN is mandatory to be filled*/
        LOG_ASSERT(TRUE == gtpPacket->extensionFlag," Missing PDCP PDU extension header , cannot fill mandatory PDCP_RX_SN");

        /* Reset counter */
        i = ZERO;

        PDCP_RX_SN_isFilled = FALSE ;

        /* Loop on extension headers to extract the required PDCP SN from PDCP PDU extension header*/
        while((i<GTP_MAX_NUM_OF_EXTENSION_HEADERS_PER_PACKET)&&(NO_MORE_HEADERS != gtpPacket->extensionHeadersArray[i].type))
        {
            if(PDCP_PDU_NUMBER == gtpPacket->extensionHeadersArray[i].type)
            {
                LOG_BRANCH("Extension header type is PDCP PDU number");

                /* Copy the PDCP PDU number in the SDU */
                RX_SDU_Desc_Ptr->PDCP_RX_SN = gtpPacket->extensionHeadersArray[i].content;

                /* Set the flag to TRUE to indicate that the PDCP_RX_SN was filled */
                PDCP_RX_SN_isFilled = TRUE;

                /* Break from the while loop*/
                break;
            } /*if(Extension header type is PDCP PDU number)*/

            /*Increment counter */
            i++;

        } /*while((i<GTP_MAX_NUM_OF_EXTENSION_HEADERS_PER_PACKET)&&(NO_MORE_HEADERS != gtpPacket->extensionHeadersArray[i].type))*/

        /* Assert that the PDCP PDU exist as the PDCP_RX_SN is mandatory to be filled*/
        LOG_ASSERT(TRUE == PDCP_RX_SN_isFilled," Missing PDCP PDU extension header , cannot fill mandatory PDCP_RX_SN");

        /* TODO_memad :walkthrough_R2.0_Oct 27, 2010_root: make sure to decrement the enqueuedSDUs if insert
         * in queue failed*/
        /* Post the descriptor to the PDCP RX in case of Forwarding*/
        QUEUE_MANAGER_ENQUEUE(QUEUE_ID_RX_GTP_PDCP_T_ENB_FWD,RX_SDU_Desc_Ptr,&status);
        LOG_ASSERT_WITH_EXCEPTION(EXCEPTION_DATA_QUEUE_OVERFLOW, WRP_SUCCESS == status,
                                                  "RX_GTP_PDCP_T_ENB_FWD queue is full");

        goto function_exit;

    } /*else (tunnel type is GTP_FWD_UL_TUNNEL)*/


    /*TODO_memad :walkthrough_R2.0_Oct 27, 2010_root: if insert in queue of DCP fails, remember to decremnt
     * the enqueuedSDU count in GTP_SDU desc */
    LOG_BRANCH("Switching on (tunnelTypeIndex)");
        switch (tunnelTypeIndex)
        {
        case GTP_DL_TUNNEL_TEID_INDEX:
            LOG_BRANCH("CASE  GTP_DL_TUNNEL_TEID_MASK_INDEX: Enqueue QUEUE_ID_TX_GTP_PDCP");

            /* Post the descriptor to the Normal PDCP TX */
            QUEUE_MANAGER_ENQUEUE(QUEUE_ID_TX_GTP_PDCP,SDU_Desc_Ptr,&status);
            LOG_ASSERT_WITH_EXCEPTION(EXCEPTION_DATA_QUEUE_OVERFLOW, WRP_SUCCESS == status,
                                                      "TX_GTP_PDCP queue is full");

            break;

        case GTP_FWD_DL_TUNNEL_TEID_INDEX:
            LOG_BRANCH("CASE  GTP_FWD_DL_TUNNEL_TEID_MASK:Enqueue QUEUE_ID_TX_GTP_PDCP_T_ENB_FWD");

            /* Set the PDCP Sequence number if found in the GTP packet */
            if(TRUE == gtpPacket->extensionFlag)
            {
                LOG_BRANCH("Extension header exist");
                /* Reset counter */
                i = ZERO;

                while((i<GTP_MAX_NUM_OF_EXTENSION_HEADERS_PER_PACKET)&&(NO_MORE_HEADERS != gtpPacket->extensionHeadersArray[i].type))
                {
                    if(PDCP_PDU_NUMBER == gtpPacket->extensionHeadersArray[i].type)
                    {
                        LOG_BRANCH("Extension header type is PDCP PDU number");

                        /* Copy the PDCP PDU number in the SDU */
                        SDU_Desc_Ptr->count = gtpPacket->extensionHeadersArray[i].content;

                        /* Set the flag to true */
                        SDU_Desc_Ptr->read_SN = TRUE;

                        /* Break from the while loop*/
                        break;
                    } /*if(Extension header type is PDCP PDU number)*/

                    /*Increment counter */
                    i++;

                } /*while((i<GTP_MAX_NUM_OF_EXTENSION_HEADERS_PER_PACKET)&&(NO_MORE_HEADERS != gtpPacket->extensionHeadersArray[i].type))*/

            } /*if(Extension header exist)*/

            /* Post the descriptor to the PDCP TX in case of Forwarding*/
            QUEUE_MANAGER_ENQUEUE(QUEUE_ID_TX_GTP_PDCP_T_ENB_FWD,SDU_Desc_Ptr,&status);
            LOG_ASSERT_WITH_EXCEPTION(EXCEPTION_DATA_QUEUE_OVERFLOW, WRP_SUCCESS == status,
                                                      "TX_GTP_PDCP_T_ENB_FWD queue is full");

            break;
        default:
            LOG_BRANCH("default");
            break;
        } /*switch (tunnelTypeIndex)*/



    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/
function_exit:
    LOG_EXIT("GTP_Tx_PDCP");
    return status;
}
