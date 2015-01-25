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