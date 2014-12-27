
#ifndef _GTP_AL_H
#define _GTP_AL_H

/*!
 \defgroup GTP_AL_H        GTP_AL
 \ingroup  ABSTRACTION_LAYER_H_
 \brief    GTP layer abstraction 
 \{
 */

/*- INCLUDES -------------------------------------------------------------------------------------*/
/*- MACROS ---------------------------------------------------------------------------------------*/
#define GTP_AL_TEST
#define MAX_RECEPTION_BUFFER            2500
/*- TYPES ----------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/*!
\struct   GTP_AL_Context

\brief 	  Holds the variables related to the GTP_AL
*/
/*------------------------------------------------------------------------------------------------*/
typedef struct _GTP_AL_Context
{

    /*! Server socket structure */
    struct sockaddr_in  serverAddr;

    /*! Client socket structure */
    struct sockaddr_in  peerHostAddr;

    /*! Server socket descriptor */
    SDS_INT32           listenSocketDesc;

    /*! Client socket descriptor */
    SDS_INT32           sendSocketDesc;

    /*! Listen task pointer */
    WRP_Task*           listenTask_Ptr;

#if 0 /*moved to the global data since it can't be accessed from GTP_AL_Ptr if already de-allocated */
    /*! FLag indicated weather the listen task should run or not */
    SDS_BOOL            listenTaskEnable ;
#endif
    /*! Reception buffer */
    SDS_UINT8           data_buff[MAX_RECEPTION_BUFFER];

} GTP_AL_Context;

/*- GLOBAL VARIABLES -----------------------------------------------------------------------------*/

/*- LOCAL VARIABLES ------------------------------------------------------------------------------*/

/*- FUNCTION DECLARATIONS ------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             void GTP_AL_sockListenTask(void* a_arg_Ptr)

  \param[in]      void* a_arg_Ptr: 

  \brief          Task created by GTP_AL to listen on GTP UDP port.

  \return         void

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
void* GTP_AL_sockListenTask(void* a_arg_Ptr);

/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             void GTP_AL_destroy_handler(GTP_AL_destroy* a_rcvdMsg_Ptr)

  \param[in]      GTP_AL_destroy* a_rcvdMsg_Ptr

  \brief          Destroys GTP AL.

  \return         void

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
void GTP_AL_destroy_handler(GTP_AL_destroy* a_rcvdMsg_Ptr);

/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             void GTP_AL_reset_Handler(GTP_AL_reset* a_msg)

  \param[in]      GTP_AL_reset* a_rcvdMsg_Ptr

  \brief          resets GTP AL: closes UDP socket reception/transmission

  \return         void

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
void GTP_AL_reset_Handler(GTP_AL_reset* a_msg);

/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             void GTP_AL_init_Handler(GTP_AL_init* a_rcvdMsg_Ptr)

  \param[in]      GTP_AL_init* a_rcvdMsg_Ptr

  \brief          Initializes GTP AL.

  \return         void

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
void GTP_AL_init_Handler(GTP_AL_init* a_rcvdMsg_Ptr);

/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             void GTP_AL_start_Handler(GTP_AL_start* a_msg)

  \param[in]      GTP_AL_start* a_msg

  \brief          Starts the GTP_AL and creates the listen task and sockets.

  \return         void

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
void GTP_AL_start_Handler(GTP_AL_start* a_msg);


/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             SDS_Status GTP_AL_sendGTPpacket(SDS_UINT8* a_buff, SDS_UINT32 len, SDS_UINT32 peerHostIp,SDS_BOOL32 MSG_MORE_Flag)

  \param[in]  SDS_UINT8* a_buff, SDS_UINT32 len, SDS_UINT32 peerHostIp,SDS_BOOL32 MSG_MORE_Flag

  \brief          Sends buff to the peer host GTP and returns status.

  \return         SDS_Status

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_Status GTP_AL_sendGTPpacket(SDS_UINT8* a_buff, SDS_UINT32 len, SDS_UINT32 peerHostIp,SDS_BOOL32 MSG_MORE_Flag);

/*!
\}
*/

#endif  /*_GTP_AL_H*/

