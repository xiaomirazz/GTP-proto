#ifndef _GTP_HEADER_H
#define _GTP_HEADER_H
/*!
 \defgroup include_guard_symbol include_guard_symbol
 \ingroup  _another_group_guard_symbol
 \brief
 \{
 */

/*- INCLUDES -------------------------------------------------------------------------------------*/

/*- MACROS ---------------------------------------------------------------------------------------*/
#define GTP_MAX_NUM_OF_EXTENSION_HEADERS_PER_PACKET     5
/*MAX number of IEs is for error indication case, 2 IEs*/
#define GTP_MAX_NUM_OF_INFORMATION_ELEMENTS_PER_PACKET  2
#define GTP_MANDATORY_HEADER_LENGTH                     8

#define SIZE_OF_EXTENSION_HEADER                        (sizeof(SDS_UINT8)+sizeof(SDS_UINT16)+sizeof(GTP_ExtensionHeaderType))

#define GTP_HEADER_VERSION_BIT_COUNT                    5
#define GTP_HEADER_PROTOCOL_TYPE_BIT_COUNT              4
#define GTP_HEADER_EXTENTSION_FLAG_BIT_COUNT            2
#define GTP_HEADER_SEQ_NUMBER_FLAG_BIT_COUNT            1

/* GTP version*/
#define GTP_VERSION                                 1
/* GTP protocol types*/
#define GTP_PROTOCOL_TYPE_GTP                       1
#define GTP_PROTOCOL_TYPE_GTP_PRIME                 0

#define LENGTH_OF_OPTIONAL_GTP_HEADER_FIELDS        4

#endif  /*_GTP_HEADER_H*/
