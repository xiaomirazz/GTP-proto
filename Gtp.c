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
