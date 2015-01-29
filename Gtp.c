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