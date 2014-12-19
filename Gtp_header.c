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
 SDS_INT32 GTP_encodePacket(GTP_HEADER* gtpPacket , SDS_UINT8* rawDataBuffer_Ptr , SDS_UINT32 rawData_offset)
{
    /*DONE_walkthrough_R2.0_Oct 24, 2010_root: use U32 unsigned for index*/
    SDS_UINT32              index=0;
    SDS_UINT32              i=0;
    SDS_UINT8*              lengthField_Ptr = NULL;
    SDS_UINT8               encodedPacket[MAX_GTP_HEADER_LENGTH];
    SDS_INT32               encodedHeaderLength;
    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/

    /* Form the first byte of the flags*/
    encodedPacket[index] = ZERO;
    /*DONE_walkthrough_R2.0_Oct 24, 2010_root: guidelines: don't use magic number
     * e.g; for version #define GTP_HEADER_VER_BIT_COUNT  5 ...... etc
     */
    encodedPacket[index] |= (gtpPacket->version) << GTP_HEADER_VERSION_BIT_COUNT;
    encodedPacket[index] |= (gtpPacket->protocolType) << GTP_HEADER_PROTOCOL_TYPE_BIT_COUNT;
    encodedPacket[index] |= (gtpPacket->extensionFlag) << GTP_HEADER_EXTENTSION_FLAG_BIT_COUNT;
    encodedPacket[index] |= (gtpPacket->sequenceNumberFlag) << GTP_HEADER_SEQ_NUMBER_FLAG_BIT_COUNT;
    encodedPacket[index] |= (gtpPacket->pnFlag);

    /* Incrementing the index*/
    index++;

    /* Setting the Message type */
    encodedPacket[index++] = gtpPacket->msgType;

    /*DONE_walkthrough_R2.0_Oct 24, 2010_root: use UTILS_ macros to set U16 and U32 in bytes*/

    /* Get pointer to the length field*/
    lengthField_Ptr = &encodedPacket[index];

    /* Skip this field and the length will be set later*/
    index += NUMBER_2;


    /* Setting the TEID*/
    UTILS_INT32_ALIGN_ENC(&encodedPacket[index],gtpPacket->TEID);
    index += NUMBER_4;
    /*encodedPacket[index++] = (SDS_UINT8)gtpPacket->TEID;
    encodedPacket[index++] = (SDS_UINT8)(gtpPacket->TEID>>8);
    encodedPacket[index++] = (SDS_UINT8)(gtpPacket->TEID>>16);
    encodedPacket[index++] = (SDS_UINT8)(gtpPacket->TEID>>24);*/

    /* Optional fields will be present if any of the flags was set*/
    if( gtpPacket->extensionFlag || gtpPacket->sequenceNumberFlag || gtpPacket->pnFlag)
    {

        /* Add the optional field length to the total packet length */
        gtpPacket->length += LENGTH_OF_OPTIONAL_GTP_HEADER_FIELDS;

        if(gtpPacket->sequenceNumberFlag)
        {
            /* Setting the Sequence Number*/
            UTILS_INT16_ALIGN_ENC(&encodedPacket[index],gtpPacket->sequenceNumber);
            index += NUMBER_2;
            /*encodedPacket[index++] = (SDS_UINT8)gtpPacket->sequenceNumber;
            encodedPacket[index++] = (SDS_UINT8)(gtpPacket->sequenceNumber>>8);*/
        }
        else
        {
            /* Setting the Sequence Number*/
            UTILS_INT16_ALIGN_ENC(&encodedPacket[index],ZERO);
            index += NUMBER_2;
 /*           encodedPacket[index++] = ZERO;
            encodedPacket[index++] = ZERO;*/
        }
        /* Setting the N PDU Number*/
        /*DONE_walkthrough_R2.0_Oct 24, 2010_root: N_PDU_number was not set in GTP_Packet struct before encode
         * u r setting rubbish in the encoded header
         */
        if(gtpPacket->pnFlag)
        {
            encodedPacket[index++] = gtpPacket->N_PDU_number;
        } /*if(N-PDU number is meaningful)*/
        else
        {
            encodedPacket[index++] = ZERO;
        }

        if(TRUE == gtpPacket->extensionFlag)
        {
            /* Setting the Extension header type*/
            encodedPacket[index++] = (SDS_UINT8)gtpPacket->extensionHeadersArray[i].type;
        }
        else
        {
            /* Setting the Extension header type*/
            encodedPacket[index++] = (SDS_UINT8)NO_MORE_HEADERS;
        }

    } /*if(Optional fields exist)*/

    if(TRUE == gtpPacket->extensionFlag)
    {
        /* loop on extension headers till there is no more */
        /*DONE_walkthrough_R2.0_Oct 24, 2010_root: loop on array length - 1 or aloc array by one more element*/
        for (i=0; i < (GTP_MAX_NUM_OF_EXTENSION_HEADERS_PER_PACKET-1); i++)
        {

            /* Add the length of the extension header to the total packet length */
            gtpPacket->length += NUMBER_4 * gtpPacket->extensionHeadersArray[i].length;

            /* Copy the extension header at the end of the packet */

            /* Set the length */
            encodedPacket[index++] = gtpPacket->extensionHeadersArray[i].length;

            /* Set the contents*/
            UTILS_INT16_ALIGN_ENC(&encodedPacket[index],gtpPacket->extensionHeadersArray[i].content);
            index += NUMBER_2;
            /*encodedPacket[index++] = (SDS_UINT8)gtpPacket->extensionHeadersArray[i].content;
            encodedPacket[index++] = (SDS_UINT8)(gtpPacket->extensionHeadersArray[i].content>>8);*/

            /* setting the next Extension Header Type field */
            encodedPacket[index++] = (SDS_UINT8)gtpPacket->extensionHeadersArray[i+1].type;

            if(NO_MORE_HEADERS == gtpPacket->extensionHeadersArray[i+1].type)
            {
                break;
            } /*if(if_branch_string)*/
        } /*for*/

    } /*if(Extension header exists)*/

    /*encodedPacket[index++] = (SDS_UINT8)gtpPacket->length;
    encodedPacket[index++] = (SDS_UINT8)(gtpPacket->length>>8);*/

    for (i = ZERO; i < gtpPacket->numberOfIEs; i++)
    {
        encodedPacket[index++] = (SDS_UINT8)gtpPacket->informationElements[i].type;
        gtpPacket->length++;
        switch (gtpPacket->informationElements[i].type)
        {
        case GTP_IE_TYPE_TUNNEL_ENDPOINT_ID_DATA_I:
            UTILS_INT32_ALIGN_ENC(&encodedPacket[index],gtpPacket->informationElements[i].value.TEID_I);
            index += 4;
            gtpPacket->length += 4;
            break;
        case GTP_IE_TYPE_GSN_ADDRESS:
            /* Only IP V4 is supported*/
            UTILS_INT16_ALIGN_ENC(&encodedPacket[index], gtpPacket->informationElements[i].length);
            index += 2;
            UTILS_INT32_ALIGN_ENC(&encodedPacket[index], gtpPacket->informationElements[i].value.GTPPeerAddress.addrV4.addr);
            index += 4;
            /* 2 bytes for length*/
            gtpPacket->length += (NUMBER_4 * gtpPacket->informationElements[i].length + NUMBER_2);
            break;
        case GTP_IE_TYPE_RECOVERY:
            encodedPacket[index++] = ZERO;
            gtpPacket->length++;
            break;
        case GTP_IE_TYPE_PRIVATE_EXTENSION:
            encodedPacket[index++] = (SDS_UINT8)gtpPacket->informationElements[i].length;
            SKL_MEMORY_COPY(&encodedPacket[index], gtpPacket->informationElements[i].value.ExtHeaderList_Ptr ,
                            gtpPacket->informationElements[i].length);

            /*1 byte for length*/
            gtpPacket->length += (gtpPacket->informationElements[i].length + 1);
            break;
        case GTP_IE_TYPE_EXTENSION_HEADER_TYPE_LIST:
            break;
        default:
            index = -1;
            break;
        } /*switch (gtpPacket->informationElements[i])*/;
    }

    /* Setting the length*/
    UTILS_INT16_ALIGN_ENC(lengthField_Ptr,gtpPacket->length);

    /* Set the length of the encoded header*/
    encodedHeaderLength = index;

    if(GTP_MSG_TYPE_GPDU == gtpPacket->msgType)
    {
        if(index <= rawData_offset)
        {

            /*                  index
             *                 <------>
             *       |--------|--------|------------------------------------|
             *       | Offset | Header |    data                            |
             *       |--------|--------|------------------------------------|
             *       |<--------------->|
             *      /^\ rawData_offset
             *     /___\
             *      | |
             *      | |
             *      |_|
             *       rawDataBuffer_Ptr
             */

            /* Copy the header at the end of the beginning of the data*/
            SKL_MEMORY_COPY((rawDataBuffer_Ptr + rawData_offset - index), encodedPacket, index);

        } /*if(Header can fit in buffer)*/
        else
        {
            /* Set the return value to -1 to indicate the error*/
            encodedHeaderLength = -1;
        }
    }

    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/

    /* Set the total encoded packet length */
    /* *len = GTP_MANDATORY_HEADER_LENGTH + gtpPacket->length;*/

    /* Return the length of the encoded header */
    return encodedHeaderLength;

}