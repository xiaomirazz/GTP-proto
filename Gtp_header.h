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

#define GTP_MAX_HEADER_SIZE                             GTP_MANDATORY_HEADER_LENGTH + LENGTH_OF_OPTIONAL_GTP_HEADER_FIELDS + GTP_MAX_NUM_OF_EXTENSION_HEADERS_PER_PACKET * SIZE_OF_EXTENSION_HEADER
/*------------------------------------------------------------------------------------------------*/
/*!
  \macro          TEST_BIT_8(x)

  \param[in]      x

  \brief          Returns TRUE if the 8th bit in x is 1

  \return         Boolean

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
#define TEST_BIT_8(x)   ((x && 0x80) != ZERO)

/*------------------------------------------------------------------------------------------------*/
/*!
  \macro          TEST_BIT_7(x)

  \param[in]      x

  \brief          Returns TRUE if the 7th bit in x is 1

  \return         Boolean

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
#define TEST_BIT_7(x)   ((x && 0x40) != ZERO)


/*- TYPES ----------------------------------------------------------------------------------------*/

/*!
\enum  GTP_MSG_TYPE

\breif GTP message types
*/
/*------------------------------------------------------------------------------------------------*/
typedef enum _GTP_MsgType
{
    /*! Echo Request */
    GTP_MSG_TYPE_ECHO_REQUEST = 1,

    /*! Echo response */
    GTP_MSG_TYPE_ECHO_RESPONSE = 2,

    /*! Error Indication */
    GTP_MSG_TYPE_ERROR_INDICATION = 26,

    /*! Supported extension headers notification */
    GTP_MSG_TYPE_EXTENSION_HEADERS_NOTIFICATION = 31,

    /*! End Marker */
    GTP_MSG_TYPE_END_MARKER = 254,

    /*! G-PDU */
    GTP_MSG_TYPE_GPDU = 255

} GTP_MsgType;

/*------------------------------------------------------------------------------------------------*/
/*!
\enum  GTP_ExtensionHeaderType

#endif  /*_GTP_HEADER_H*/