
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


