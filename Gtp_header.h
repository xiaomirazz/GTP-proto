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
\breif Type of the Extension Headers
*/
/*------------------------------------------------------------------------------------------------*/
typedef enum _GTP_ExtensionHeaderType
{

    /*! No more extension headers*/
    NO_MORE_HEADERS = 0,

    /*! UDP port.Provides the UDP source port of the triggering message*/
    UDP_PORT = 64,

    /*! PDCP PDU Number*/
    PDCP_PDU_NUMBER = 192

} GTP_ExtensionHeaderType;

/*------------------------------------------------------------------------------------------------*/
/*!
\enum  GTP_InformationElementType

\breif GTP-U signaling message may contain several information elements and these are the Types
*/
/*------------------------------------------------------------------------------------------------*/
typedef enum _GTP_InformationElementType
{
    /*! Type Recovery */
    GTP_IE_TYPE_RECOVERY = 14,

    /*! Tunnel End-point ID data I */
    GTP_IE_TYPE_TUNNEL_ENDPOINT_ID_DATA_I = 16,

    /*! GSN Address */
    GTP_IE_TYPE_GSN_ADDRESS = 133,

    /*! Extension Header Type List */
    GTP_IE_TYPE_EXTENSION_HEADER_TYPE_LIST = 141,

    /*! Private extension */
    GTP_IE_TYPE_PRIVATE_EXTENSION = 255

} GTP_InformationElementType;

/*------------------------------------------------------------------------------------------------*/
/*!
\union  InformationElementValue

\brief	description
*/
/*------------------------------------------------------------------------------------------------*/
typedef union _InformationElementValue
{
    /*! GTP IP*/
    eNB_TransportAddress GTPPeerAddress;

    /*! Tunnel Endpoint Identifier Data I*/
    SDS_UINT32           TEID_I;

    /*! Restart Counter*/
    SDS_UINT8            RestartCounter;

    /*! Extension Header List*/
    SDS_UINT8*           ExtHeaderList_Ptr;

} InformationElementValue;
/*------------------------------------------------------------------------------------------------*/
/*!
\struct   GTP_extensionHeader

\brief    GTP-U extension header
*/
/*------------------------------------------------------------------------------------------------*/
typedef struct _GTP_extensionHeader
{

    /*! Next extension header Type*/
    GTP_ExtensionHeaderType type;

    /*! Specifies the length in 4 octets units. ie an extension header of length 4 bytes will have a value of 1 in this field*/
    SDS_UINT8 length;

    /*! Extension header contetn, maybe PDCP PDU number or UDP port number*/
    SDS_UINT16 content;

} GTP_extensionHeader;

/*------------------------------------------------------------------------------------------------*/
/*!
\struct   InformationElement

\brief 	  description
*/
/*------------------------------------------------------------------------------------------------*/
typedef struct _InformationElement
{
    /*! Information Element Type*/
    GTP_InformationElementType type;

    /*! Information Element Length*/
    SDS_UINT16       length;

    /*! Information Element Value*/
    InformationElementValue value;

} InformationElement;

/*------------------------------------------------------------------------------------------------*/
/*!
\struct   GTP_PACKET

\brief    GTP-U packet
*/
/*------------------------------------------------------------------------------------------------*/
typedef struct _GTP_HEADER
{
    /*! GTP-U version */
    SDS_UINT8 version;

    /*! Protocol type indicates weather the used protocol is GTP or GTP'. Should be 1 for GTP */
    SDS_BOOL protocolType;

    /*! If set to 1,the Extension header is meaningful */
    SDS_BOOL extensionFlag;

    /*! If set to 1,the sequence number is meaningful */
    SDS_BOOL sequenceNumberFlag;

    /*! If set to 1 , the N-PDU Number field is meaningful */
    SDS_BOOL pnFlag;

    /*! Indicates the type of the GTP message */
    GTP_MsgType msgType;

    /*! Indicates the Indicates the length of the payload */
    SDS_UINT16 length;

    /*! Holds Tunnel ID */
    SDS_UINT32 TEID;

    /*! Holds sequence number */

    SDS_UINT16 sequenceNumber;

    /*! Holds N-PDU number */
    SDS_UINT8 N_PDU_number;

    /*! Number of information elements*/
    SDS_UINT8 numberOfIEs;

    /*! Array of extension headers*/
    GTP_extensionHeader  extensionHeadersArray[GTP_MAX_NUM_OF_EXTENSION_HEADERS_PER_PACKET];

    /*! Array of Information Elements*/
    InformationElement   informationElements[GTP_MAX_NUM_OF_INFORMATION_ELEMENTS_PER_PACKET];

} GTP_HEADER;

/*- GLOBAL VARIABLES -----------------------------------------------------------------------------*/

/*- LOCAL VARIABLES ------------------------------------------------------------------------------*/

/*- FUNCTION DECLARATIONS ------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             SDS_UINT32 GTP_encodePacket(GTP_HEADER* gtpPacket , SDS_UINT8* rawDataBuffer_Ptr , SDS_UINT32 rawData_offset)

  \param[in]      GTP_HEADER* gtpPacket , SDS_UINT8* rawDataBuffer_Ptr , SDS_UINT32 rawData_offset

  \brief          Encodes a GTP packet to be sent on the network and copies the header in rawDataBuffer_ptr.

  \return         Length of encoded header and -1 if rawData_offset was less than the required header not enough

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_INT32 GTP_encodePacket(GTP_HEADER* gtpPacket , SDS_UINT8* rawDataBuffer_Ptr , SDS_UINT32 rawData_offset);
/*------------------------------------------------------------------------------------------------*/
/*!
  \fn             void GTP_decodePacket(SDS_UINT8* receivedPacket, SDS_UINT32 len, GTP_PACKET gtpPacket)

  \param[in]      SDS_UINT8* receivedPacket, SDS_UINT32 len, GTP_PACKET gtpPacket

  \brief          Decodes receivedPacket to GTP_PACKET structure.

  \return         Number of read bytes

  \b See \b Also: <related functions, data structures, global variables>
*/
/*------------------------------------------------------------------------------------------------*/
SDS_UINT32 GTP_decodePacket(SDS_UINT8* receivedPacket, SDS_UINT32 len, GTP_HEADER* gtpPacket, SDS_BOOL* decodeResult);

/*!
\}
*/
#endif  /*_GTP_HEADER_H*/