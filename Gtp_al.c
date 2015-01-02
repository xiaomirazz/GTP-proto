/*- MACROS ---------------------------------------------------------------------------------------*/
#define ENB_COMPONENT_ID        COMPONENT_ID_GTP_AL
#define COMPONENT_ID()          (GTP_AL_LOG_ID)
#define COMPONENT_MASK          COMPONENT_MASK_GTP_AL

#define GTP_AL_SOCK_LISTEN_TASK_PRIORITY        25
#define GTP_AL_SOCK_LISTEN_TASK_AFFINITY        1
#define GTP_AL_SOCK_LISTEN_TASK_STACK_SIZE      21504
#define GTP_PORT                                2152
#define GTP_AL_TASK_SLEEP_MS                    1

/*- TYPES ----------------------------------------------------------------------------------------*/

/*- GLOBAL VARIABLES -----------------------------------------------------------------------------*/

/* DONE_WALK_THROUGH_R2.0_ratef_Oct 17, 2010: collect the info in GTP_AL structure
 * the structure can be allocated at init_handler and a pointer only saved as global
 * g_GTP_AL_Ptr
 */

SDS_STATIC GTP_AL_Context* g_GTP_AL_Ptr;
/*! FLag indicated weather the listen task should run or not */
SDS_BOOL            listenTaskEnable;

#ifdef T1_TEST
extern unsigned char g_GTPMsgTxBuffer[100000];
unsigned int         g_GTPMsgTxBuffer_offset;
#endif

/*- LOCAL VARIABLES ------------------------------------------------------------------------------*/

/*- FUNCTION DECLARATIONS ------------------------------------------------------------------------*/

/*- FUNCTION DEFINITIONS -------------------------------------------------------------------------*/

/* WALK_THROUGH_R2.0_ratef_Oct 19, 2010: add new message GTP_AL_Start:
 * - create the sockets
 * - create the listen task
 *
 * In Init: only allocate needed memory
 * In destroy: exit the task, and close sockets then destroy the allocated memory
 * Take care that the send socket to S-GW might still be called from RX path (find a
 * solution to handle this case)
 * In Reset: exit the task, and close sockets only ..
 * start is expected to be called after init and after reset
 */
void GTP_AL_init_Handler(GTP_AL_init* a_rcvdMsg_Ptr)
{

    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/
    GTP_AL_LOG_ID = LOGGER_REGISTER("GTP_AL");
    LOG_ENTER("GTP_AL_init_handler");

    /* Initialize private data*/
    SKL_MEMORY_ALLOC(SKL_GET_MEMORY_SEGMENT_FOR_TASK(),  sizeof(*g_GTP_AL_Ptr) ,g_GTP_AL_Ptr);

    /*CROSS_REVIEW_habdallah_DONE the structure of RX_GTP_SDU needs to make buffer SDS_UNIT8 and MTU
     * 1518*/

    /*CROSS_REVIEW_habdallah * we may reconsider allocating from data memory*/

    /* Register start handlers */
    SKL_MESSAGE_HANDLER_REGISTER(INTERCOMP_MSG_ID_GTP_AL_START, GTP_AL_start_Handler, NULL);

    /* Register destroy handler */
    SKL_MESSAGE_HANDLER_REGISTER(INTERCOMP_MSG_ID_GTP_AL_DESTROY, GTP_AL_destroy_handler,NULL);

    /*BUG: registration of GTP_AL_reset_Handler is missing */
    /* Register destroy handler */
    SKL_MESSAGE_HANDLER_REGISTER(INTERCOMP_MSG_ID_GTP_AL_RESET, GTP_AL_reset_Handler,NULL);



    /* Disable listen task till GTP_AL_start is called*/
    listenTaskEnable = FALSE;

    /* Create Listen task */
    SKL_TASK_INIT( g_GTP_AL_Ptr->listenTask_Ptr                                        /* Task pointer     */ ,
                   g_LTE_ENB_SKL.memorySegment_Ptr[GET_TASK_INDEX(TASK_ID_DATA_HANDLER)]       /* Task memory segment  */,
                   (EntryPointFunctionPointer)GTP_AL_sockListenTask    /* Entry point          */,
                        GTP_AL_SOCK_LISTEN_TASK_STACK_SIZE                  /* Stack size      */,
                        GTP_AL_SOCK_LISTEN_TASK_PRIORITY                    /* Priority        */,
                        GTP_AL_SOCK_LISTEN_TASK_AFFINITY                    /* Affinity        */,
                        NULL                                                /* Arguments       */,
                        "SockListenTask"                                    /* Task name       */);

    /* DONE_WALK_THROUGH_R2.0_ratef_Oct 19, 2010: add a reset_handler for the GTP_AL */

#ifdef T1_TEST
    g_GTPMsgTxBuffer_offset = 0;
#endif
    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/
    LOG_EXIT("GTP_AL_init_handler");
}

void GTP_AL_start_Handler(GTP_AL_start* a_msg)
{
    SDS_INT32           bret;
    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/
    LOG_ENTER("GTP_AL_start_handler");

    /* ANSWERED_WALK_THROUGH_R2.0_ratef_Oct 17, 2010: use WRP_SOCK functions
     * memad: No UDP sockets in WRP sockets
     */

    /* Create server socket */
    g_GTP_AL_Ptr->listenSocketDesc = socket(PF_INET, SOCK_DGRAM, 0);

    /* Reset structure */
    SKL_MEMORY_SET(&g_GTP_AL_Ptr->serverAddr, ZERO,sizeof(g_GTP_AL_Ptr->serverAddr));

    /* Set family as IP protocol */
    g_GTP_AL_Ptr->serverAddr.sin_family = AF_INET;
    /* Set server port */
    g_GTP_AL_Ptr->serverAddr.sin_port = htons(GTP_PORT);
    /* Receive from any address */
    g_GTP_AL_Ptr->serverAddr.sin_addr.s_addr = INADDR_ANY;
    /* Bind socket */
    bret = bind(g_GTP_AL_Ptr->listenSocketDesc, (struct sockaddr*)&g_GTP_AL_Ptr->serverAddr, sizeof(g_GTP_AL_Ptr->serverAddr));
    if( bret < 0)
    {
        LOG_BRANCH("Failed to bind server socket");
        perror("bind");
    } /*if(bind)*/

    /* Create a task to listen on the GTP port */
   /*CROSS_REVIEW_habdallah_DONE define the priority with 25*/
    /*CROSS_REVIEW_habdallah_DONE define stack size 21504*/
    /*CROSS_REVIEW_habdallah * we may reconsider allocating thread in skeleton*/
    listenTaskEnable = TRUE;

    /* Create peer host socket to send on , the address of the peer will be set by the GTP during open tunnel procedure*/
    g_GTP_AL_Ptr->sendSocketDesc = socket(PF_INET, SOCK_DGRAM, 0);

    /* Reset structure */
    SKL_MEMORY_SET(&g_GTP_AL_Ptr->peerHostAddr, ZERO,sizeof(g_GTP_AL_Ptr->peerHostAddr));

    /* Set family as IP protocol */
    g_GTP_AL_Ptr->peerHostAddr.sin_family = AF_INET;
    /* Set server port */
    g_GTP_AL_Ptr->peerHostAddr.sin_port = htons(GTP_PORT);

#ifdef GTP_AL_TEST
    if(inet_aton("172.25.25.232", &g_GTP_AL_Ptr->peerHostAddr.sin_addr) == 0)
    {
        LOG_BRANCH("Failed to set peer host IP");
    } /*if(Failed to set peer )*/
#endif

    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/
    LOG_EXIT("GTP_AL_start_handler");
}

void GTP_AL_reset_Handler(GTP_AL_reset* a_msg)
{

    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/
    LOG_ENTER("GTP_AL_reset_Handler");

    /* Disable listen task till GTP_AL_start is called*/
    listenTaskEnable = FALSE;

    /* Close server socket */
    shutdown (g_GTP_AL_Ptr->listenSocketDesc, SHUT_RDWR);
    close(g_GTP_AL_Ptr->listenSocketDesc);

    /* Close client socket */
    shutdown (g_GTP_AL_Ptr->sendSocketDesc, SHUT_RDWR);
    close(g_GTP_AL_Ptr->sendSocketDesc);

/*function_exit:*/
    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/
    LOG_EXIT("GTP_AL_reset_Handler");
}


void GTP_AL_destroy_handler(GTP_AL_destroy* a_rcvdMsg_Ptr)
{

    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/
    LOG_ENTER("GTP_AL_destroy_handler");

    /*CROSS_REVIEW_habdallah_DONE close the sockets first then destroy the pool*/
    listenTaskEnable = FALSE;

    /* FIXME_memad : destroy listen task*/
    /* WALK_THROUGH_R2.0_ratef_Oct 19, 2010: you can proceed and destroy the pool
     * only after the listenTask exists
     * You need here a semaphore lock, when un-locked by the listen task you can proceed
     * and destroy the task
     */

    /*BUG: the release of this pool should be from skeleton since it is shared between 2 components
     * Destroy allocated buffer
     * SKL_FIXED_SIZE_POOL_DESTROY(GTP_SDU_POOL,SHARED);
     *
     */

    /* Destroy the listen task */
    SKL_TASK_DESTROY(g_GTP_AL_Ptr->listenTask_Ptr);

    /* Free the component structure */
    SKL_MEMORY_FREE(g_GTP_AL_Ptr);

/*function_exit:*/
    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/
    LOG_EXIT("GTP_AL_destroy_handler");
}
