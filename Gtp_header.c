/*- INCLUDES -------------------------------------------------------------------------------------*/
#include "enb_common_includes.h"
#include "gtp_header.h"
/*- MACROS ---------------------------------------------------------------------------------------*/

/*- TYPES ----------------------------------------------------------------------------------------*/

/*- GLOBAL VARIABLES -----------------------------------------------------------------------------*/

/*- LOCAL VARIABLES ------------------------------------------------------------------------------*/

/*- FUNCTION DECLARATIONS ------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             SDS_BOOL GTP_decodeIEs(SDS_UINT8* receivedPacket,SDS_UINT16 len, GTP_HEADER* gtpPacket)

  \param[in]      SDS_UINT8* receivedPacket : Pointer to the received message.
  \param[in]      SDS_UINT16 len : Total message length.
  \param[in]      GTP_HEADER* gtpPacket : Pointer to the decoded message structure.

  \brief          Decode information elements.

  \return         SDS_BOOL

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_STATIC SDS_BOOL GTP_decodeIEs(SDS_UINT8* receivedPacket,SDS_UINT16 len, GTP_HEADER* gtpPacket);


/*- FUNCTION DEFINITIONS -------------------------------------------------------------------------*/

/*DONE_Cross_Review_R2.0_vsafwat: move encode/decode functions and GTP_PACKET definition
 * from gtp.c/h to /proto_stack/common/air_msgs/headers
 */