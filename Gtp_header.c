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

SDS_UINT32 GTP_decodePacket(SDS_UINT8* receivedPacket, SDS_UINT32 len, GTP_HEADER* gtpPacket, SDS_BOOL* decodeResult)
{

    SDS_UINT32                  index=0;
    GTP_ExtensionHeaderType     currentExtensionHeaderType;
    SDS_UINT32                  i;
    SDS_UINT32                  totalNumberOfDecodedBytes;
    /*- VARIABLE DECLARATION ENDS HERE -----------------------------------------------------------*/

    /* Extracting the first byte*/
    gtpPacket->version = (SDS_UINT8)receivedPacket[0]>>5;
    gtpPacket->protocolType = (SDS_BOOL)(receivedPacket[index]>>4 & (SDS_CHAR)0x01);


    gtpPacket->extensionFlag = (SDS_BOOL)(receivedPacket[index]>>2 & (SDS_CHAR)0x01);
    gtpPacket->sequenceNumberFlag = (SDS_BOOL)(receivedPacket[index]>>1 & (SDS_CHAR)0x01);
    gtpPacket->pnFlag = (SDS_BOOL)(receivedPacket[index++] & (SDS_CHAR)0x01);

    /* Setting Message type*/
    gtpPacket->msgType = receivedPacket[index++];

    /*DONE_walkthrough_R2.0_Oct 25, 2010_root: make sure that this length will be adjusted
     * to indicate the length of the data only
     */
    /* Setting the Length */
    UTILS_INT16_ALIGN_DEC(&receivedPacket[index],&(gtpPacket->length));
    /*gtpPacket->length = ((SDS_UINT16)receivedPacket[index]) | (((SDS_UINT16)receivedPacket[index+1])<<8);*/
    index += 2;

    /* Set the total number of decoded bytes
     * Note: The length field in the GTP header includes indicated the length of the payload
     *       which is the bytes following the first 8 mandatory bytes in the header.
     */
    totalNumberOfDecodedBytes = gtpPacket->length + GTP_MANDATORY_HEADER_LENGTH;

    if((gtpPacket->version != GTP_VERSION) || (gtpPacket->protocolType != GTP_PROTOCOL_TYPE_GTP))
    {
        /*LOG_BRANCH("GTP version not equal one. Packet is dropped");*/
        /*DONE_walkthrough_R2.0_Oct 25, 2010_root: skip with the length of this SDU */
        *decodeResult = FALSE;
        goto function_exit;
    } /*if(GTP version not equal one. Packet is dropped)*/


    /* Setting the TEID */
    UTILS_INT32_ALIGN_DEC(&receivedPacket[index],&(gtpPacket->TEID));
    /*gtpPacket->TEID = (SDS_UINT32)receivedPacket[index] | ((SDS_UINT32)receivedPacket[index+1] <<8)
                        | ((SDS_UINT32)receivedPacket[index+2] <<16) | ((SDS_UINT32)receivedPacket[index+3] <<24);*/
    index+=4;

    /*DONE_walkthrough_R2.0_Oct 25, 2010_root: check first on the flags to decide
     * whether sequenceNumber , extension and N_PDU_number exist
     */
    if( gtpPacket->extensionFlag || gtpPacket->sequenceNumberFlag || gtpPacket->pnFlag)
    {

        /* Subtracting the length of the optional fields from the data length */
        gtpPacket->length -= LENGTH_OF_OPTIONAL_GTP_HEADER_FIELDS;

        /* Setting the sequence number */
        UTILS_INT16_ALIGN_DEC(&receivedPacket[index],&(gtpPacket->sequenceNumber));
        /*gtpPacket->sequenceNumber = ((SDS_UINT16)receivedPacket[index]) | ((SDS_UINT16)receivedPacket[index+1] <<8);*/
        index+=2;

        /* Setting the N-PDU number */
        gtpPacket->N_PDU_number = (SDS_UINT8)receivedPacket[index++];

        /* Handing extension Headers */

        currentExtensionHeaderType = receivedPacket[index++];

        i = 0;

        while(currentExtensionHeaderType != NO_MORE_HEADERS)
        {

            /* Setting the Next extension header field */
            /*gtpPacket->nextExtensionHeaderType = receivedPacket[11];*/
            gtpPacket->extensionHeadersArray[i].type = currentExtensionHeaderType;

            /* Setting the length */
            gtpPacket->extensionHeadersArray[i].length = receivedPacket[index++];

            /* Subtract the length of the extension header from the data length */
            gtpPacket->length -= gtpPacket->extensionHeadersArray[i].length * NUMBER_4;

            /*            LOG_BRANCH("Switching on (currentExtensionHeaderType)");*/
            switch (currentExtensionHeaderType)
            {
            case UDP_PORT:
            case PDCP_PDU_NUMBER:
                /* extensionHeader.content is PDCP PDU Number */;
                UTILS_INT16_ALIGN_DEC(&receivedPacket[index],&(gtpPacket->extensionHeadersArray[i].content));
                /*extensionHeader.content = ((SDS_UINT16)receivedPacket[index]) | ((SDS_UINT16)receivedPacket[index+1]<<8);*/
                index+=2;
                break;
            default:
                /* LOG_BRANCH("Unsupported extension header");*/

                /*
                 * Calculate the offset to the "Next Extension Header Type" field
                 *
                 *          |------------------------------------------|
                 * octet 1  |    Extension header length    (1 byte)   | Note : Length is multiples of 4 octets
                 *          |------------------------------------------|
                 * octet 2,3|    Extension header content   (2 bytes)  |
                 *          |------------------------------------------|
                 * octet 4  |    Next extension header type (1 byte)   |
                 *          |------------------------------------------|
                 *
                 * */

                /*CROSS_REVIEW_habdallah_DONE please clarify this with a comment*/

                /*The length of the Extension header shall be defined in a variable length of 4
                 * octets, i.e. m+1 = n*4 octets, where n is a positive integer.*/

                /*this line was index += (4 * gtpPacket->extensionHeadersArray[i].length ) - 2
                 * i changed it to -1 as the length written in the extension header =
                 *  1 byte length + content + next type so we need to decrement one for the next
                 *  extension type
                 */

                /* update the index */
                index += (4 * gtpPacket->extensionHeadersArray[i].length ) - 1 ;
                break;
            } /*switch (currentExtensionHeaderType)*/

            /*extensionHeader.nextExtensionHeaderType = receivedPacket[index++];*/
            currentExtensionHeaderType = receivedPacket[index++];

            /* Increment extension header array indexer */
            i++;
            /*DONE_walkthrough_R2.0_Oct 25, 2010_root: assert that decoded extensionHeaders
             * are not larger than array of extensionHeaders inside GTP_Packet struct
             */
        } /*while(gtpPacket->nextExtensionHeaderType != NO_MORE_HEADERS)*/
        LOG_ASSERT(i < GTP_MAX_NUM_OF_EXTENSION_HEADERS_PER_PACKET ,"Received number of extension headers exceeds maximum allowed number");

        /* Set next extension header type as NO_MORE_HEADERS*/
        gtpPacket->extensionHeadersArray[i].type = NO_MORE_HEADERS;
    }

    if(GTP_MSG_TYPE_GPDU != gtpPacket->msgType)
    {
        (*decodeResult) = GTP_decodeIEs(&(receivedPacket[index]),gtpPacket->length,gtpPacket);
    }

function_exit:
/*DONE_walkthrough_R2.0_Oct 25, 2010_root: return the decoded message length = header length
 */
    /*- CLEAN UP CODE STARTS HERE ----------------------------------------------------------------*/
return totalNumberOfDecodedBytes;
   /* LOG_EXIT("GTP_decodeAction");*/
}

SDS_STATIC SDS_BOOL GTP_decodeIEs(SDS_UINT8* receivedPacket,SDS_UINT16 len, GTP_HEADER* gtpPacket)
{
    SDS_UINT32                  index = 0;
    SDS_UINT32                  i = 0;
    GTP_InformationElementType  IEType;
    SDS_BOOL                    decodeStatus = FALSE;
    SDS_UINT16                  length = 0;

    switch (gtpPacket->msgType)
    {
    case GTP_MSG_TYPE_ECHO_REQUEST:
    case GTP_MSG_TYPE_END_MARKER:
        /*possible IE is private extension ,so we will check this IE if it is supported*/
        decodeStatus = TRUE;
        break;
    case GTP_MSG_TYPE_ECHO_RESPONSE:
        while(index < len)
        {
            IEType = receivedPacket[index++];
            switch(IEType)
            {
            case GTP_IE_TYPE_RECOVERY:
                /*Mandatory IE and it will be neglected*/
                index++;
                decodeStatus = TRUE;
                index = len;
                break;
            case GTP_IE_TYPE_TUNNEL_ENDPOINT_ID_DATA_I:
                /*unexpected TV IE*/
                index += 4;
                break;
            case GTP_IE_TYPE_EXTENSION_HEADER_TYPE_LIST:
                /*unexpected IE*/
                length = receivedPacket[index++];
                index += length;
                break;
            default:
                if(TRUE == UTILS_IS_BIT_SET(IEType,8))
                {
                    UTILS_INT16_ALIGN_DEC(&(receivedPacket[index]), &length);
                    index += 2;
                    index += (length * 4);
                }
                else
                {
                    /*unknown TV element*/
                    index = len;
                }
                break;
            }
        }
        break;
    case GTP_MSG_TYPE_EXTENSION_HEADERS_NOTIFICATION:
        while(index < len)
        {
            IEType = receivedPacket[index++];
            switch(IEType)
            {
            case GTP_IE_TYPE_RECOVERY:
                /*Mandatory IE and it will be neglected*/
                index++;
                break;
            case GTP_IE_TYPE_TUNNEL_ENDPOINT_ID_DATA_I:
                /*unexpected TV IE*/
                index += 4;
                break;
            case GTP_IE_TYPE_EXTENSION_HEADER_TYPE_LIST:
                if(ZERO == i)
                {
                    length = receivedPacket[index++];
                    index = len;
                    gtpPacket->informationElements[0].type = GTP_IE_TYPE_EXTENSION_HEADER_TYPE_LIST;
                    gtpPacket->informationElements[0].length = length;
                    gtpPacket->informationElements[0].value.ExtHeaderList_Ptr = &(receivedPacket[index]);
                    decodeStatus = TRUE;
                    i++;
                }
                break;
            default:
                if(TRUE == UTILS_IS_BIT_SET(IEType,8))
                {
                    UTILS_INT16_ALIGN_DEC(&(receivedPacket[index]), &length);
                    index += 2;
                    index += (length * 4);
                }
                else
                {
                    /*unknown TV element*/
                    index = len;
                }
                break;
            }
        }
        break;
    case GTP_MSG_TYPE_ERROR_INDICATION:
        while(index < len)
        {
            IEType = receivedPacket[index++];
            switch(IEType)
            {
            case GTP_IE_TYPE_RECOVERY:
                /*Mandatory IE and it will be neglected*/
                index++;
                break;
            case GTP_IE_TYPE_EXTENSION_HEADER_TYPE_LIST:
                /*unexpected IE*/
                length = receivedPacket[index++];
                index += length;
                break;
            case GTP_IE_TYPE_TUNNEL_ENDPOINT_ID_DATA_I:
                if(ZERO == i)
                {
                    gtpPacket->informationElements[i].type = GTP_IE_TYPE_TUNNEL_ENDPOINT_ID_DATA_I;
                    UTILS_INT32_ALIGN_DEC(&receivedPacket[index],
                        &(gtpPacket->informationElements[i].value.TEID_I));
                    index += 4;
                    i++;
                }
                break;
            case GTP_IE_TYPE_GSN_ADDRESS:
                if(NUMBER_1 == i)
                {
                    gtpPacket->informationElements[i].type = GTP_IE_TYPE_GSN_ADDRESS;
                    gtpPacket->informationElements[i].length = receivedPacket[index++];
                    UTILS_INT32_ALIGN_DEC(&receivedPacket[index],
                        &(gtpPacket->informationElements[i].value.GTPPeerAddress));
                    index += 4* gtpPacket ->informationElements[i].length;
                    i++;
                    decodeStatus = TRUE;
                    index = len;
                }
                break;
            default:
                if(TRUE == UTILS_IS_BIT_SET(IEType,8))
                {
                    UTILS_INT16_ALIGN_DEC(&(receivedPacket[index]), &length);
                    index += 2;
                    index += (length * 4);
                }
                else
                {
                    /*unknown TV element*/
                    index = len;
                }
                break;
            }
        }
        break;
    default:
        break;
    } /*switch (gtpPacket->msgType)*/

    gtpPacket->numberOfIEs = i;

   return decodeStatus;
}
