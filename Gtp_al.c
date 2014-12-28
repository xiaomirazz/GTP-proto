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
