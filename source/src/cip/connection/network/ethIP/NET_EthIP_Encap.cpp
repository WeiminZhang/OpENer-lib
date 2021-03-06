/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
//Includes
#include <cstring>
#include "NET_EthIP_Encap.hpp"
#include "../../../CIP_Objects/CIP_00F6_EthernetLink/CIP_EthernetIP_Link.hpp"
#include "eip_endianconv.hpp"
#include "../NET_Endianconv.hpp"
#include "../NET_NetworkHandler.hpp"
#include "../../../CIP_Objects/CIP_0001_Identity/CIP_Identity.hpp"
#include "../../CIP_CommonPacket.hpp"

//Static variables
bool NET_EthIP_Encap::initialized = false;
const int NET_EthIP_Encap::kOpENerEthernetPort = 0xAF12;
EncapsulationInterfaceInformation NET_EthIP_Encap::g_interface_information;
int NET_EthIP_Encap::g_registered_sessions[OPENER_NUMBER_OF_SUPPORTED_SESSIONS];
NET_EthIP_Encap::DelayedEncapsulationMessage NET_EthIP_Encap::g_delayed_encapsulation_messages[ENCAP_NUMBER_OF_SUPPORTED_DELAYED_ENCAP_MESSAGES];
const int NET_EthIP_Encap::kSupportedProtocolVersion = 1; /**< Supported Encapsulation protocol version */
const int NET_EthIP_Encap::kEncapsulationHeaderOptionsFlag = 0x00; /**< Mask of which options are supported as of the current CIP specs no other option value as 0 should be supported.*/
const int NET_EthIP_Encap::kEncapsulationHeaderSessionHandlePosition = 4; /**< the position of the session handle within the encapsulation header*/
const int NET_EthIP_Encap::kListIdentityDefaultDelayTime = 2000; /**< Default delay time for List Identity response */
const int NET_EthIP_Encap::kListIdentityMinimumDelayTime = 500; /**< Minimum delay time for List Identity response */
const int NET_EthIP_Encap::kSenderContextSize = 8; /**< size of sender context in encapsulation header*/


//Methods

NET_EthIP_Encap::NET_EthIP_Encap()
{

};

NET_EthIP_Encap::~NET_EthIP_Encap()
{

};
/*   @brief Initializes session list and interface information. */

bool NET_EthIP_Encap::EncapsulationInit()
{
    if (!initialized)
    {
        NET_Endianconv::DetermineEndianess();

        /*initialize random numbers for random delayed response message generation
       * we use the ip address as seed as suggested in the spec */
        //srand(interface_configuration_.ip_address);

        /* initialize Sessions to invalid == free session */
        for (unsigned int i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; i++)
        {
            g_registered_sessions[i] = kEipInvalidSocket;
        }

        for (unsigned int i = 0; i < ENCAP_NUMBER_OF_SUPPORTED_DELAYED_ENCAP_MESSAGES; i++)
        {
            g_delayed_encapsulation_messages[i].socket = -1;
        }

        /*TODO make the interface information configurable*/
        /* initialize interface information */
        g_interface_information.type_code = CIP_CommonPacket::kCipItemIdListServiceResponse;
        g_interface_information.length = sizeof(g_interface_information);
        g_interface_information.encapsulation_protocol_version = 1;
        g_interface_information.capability_flags = kCapabilityFlagsCipTcp | kCapabilityFlagsCipUdpClass0or1;
        strcpy((char*)g_interface_information.name_of_service, "Communications");

        initialized = true;
        return true;
    }
    return false;
}


int NET_EthIP_Encap::HandleReceivedExplictTcpData(int socket, CipUsint* buffer,
    unsigned int length, int* remaining_bytes)
{
    CipStatus return_value = kCipGeneralStatusCodeSuccess;
    EncapsulationData encapsulation_data;
    /* eat the encapsulation header*/
    /* the structure contains a pointer to the encapsulated data*/
    /* returns how many bytes are left after the encapsulated data*/
    *remaining_bytes = CreateEncapsulationStructure(buffer, length, &encapsulation_data);

    if (kEncapsulationHeaderOptionsFlag == encapsulation_data.options) /*TODO generate appropriate error response*/
    {
        if (*remaining_bytes >= 0) /* check if the message is corrupt: header size + claimed payload size > than what we actually received*/
        {
            /* full package or more received */
            encapsulation_data.status = kEncapsulationProtocolSuccess;
            return_value = kCipGeneralStatusCodeSuccess;
            /* most of these functions need a reply to be send */
            switch (encapsulation_data.command_code) {
                case (kEncapsulationCommandNoOperation):
                    /* NOP needs no reply and does nothing */
                    return_value = kCipGeneralStatusCodeSuccess;
                    break;

                case (kEncapsulationCommandListServices):
                    HandleReceivedListServicesCommand(&encapsulation_data);
                    break;

                case (kEncapsulationCommandListIdentity):
                    HandleReceivedListIdentityCommandTcp(&encapsulation_data);
                    break;

                case (kEncapsulationCommandListInterfaces):
                    HandleReceivedListInterfacesCommand(&encapsulation_data);
                    break;

                case (kEncapsulationCommandRegisterSession):
                    HandleReceivedRegisterSessionCommand(socket, &encapsulation_data);
                    break;

                case (kEncapsulationCommandUnregisterSession):
                    return_value = HandleReceivedUnregisterSessionCommand(
                        &encapsulation_data);
                    break;

                case (kEncapsulationCommandSendRequestReplyData):
                    return_value = HandleReceivedSendRequestResponseDataCommand(
                        &encapsulation_data);
                    break;

                case (kEncapsulationCommandSendUnitData):
                    return_value = HandleReceivedSendUnitDataCommand(&encapsulation_data);
                    break;

                default:
                    encapsulation_data.status = kEncapsulationProtocolInvalidCommand;
                    encapsulation_data.data_length = 0;
                    break;
            }
            /* if nRetVal is greater than 0 data has to be sent */
            if (kCipGeneralStatusCodeSuccess < return_value.status)
            {
                return_value = (CipStatus)EncapsulateData(&encapsulation_data);
            }
        }
    }

    return return_value.status;
}

int NET_EthIP_Encap::HandleReceivedExplictUdpData(int socket, struct sockaddr* from_address, CipUsint* buffer, unsigned int buffer_length, int* number_of_remaining_bytes, bool unicast)
{
    CipStatus status = kCipGeneralStatusCodeSuccess;
    EncapsulationData encapsulation_data;
    /* eat the encapsulation header*/
    /* the structure contains a pointer to the encapsulated data*/
    /* returns how many bytes are left after the encapsulated data*/
    *number_of_remaining_bytes = CreateEncapsulationStructure(buffer, buffer_length, &encapsulation_data);

    if (kEncapsulationHeaderOptionsFlag == encapsulation_data.options) /*TODO generate appropriate error response*/
    {
        if (*number_of_remaining_bytes >= 0) /* check if the message is corrupt: header size + claimed payload size > than what we actually received*/
        {
            /* full package or more received */
            encapsulation_data.status = kEncapsulationProtocolSuccess;
            status = kCipGeneralStatusCodeSuccess;
            /* most of these functions need a reply to be send */
            switch (encapsulation_data.command_code)
            {
                case (kEncapsulationCommandListServices):
                    HandleReceivedListServicesCommand(&encapsulation_data);
                    break;

                case (kEncapsulationCommandListIdentity):
                    if (unicast == true)
                    {
                        HandleReceivedListIdentityCommandTcp(&encapsulation_data);
                    } else
                    {
                        HandleReceivedListIdentityCommandUdp (socket, (struct sockaddr_in*)from_address, &encapsulation_data);
                        status = kCipGeneralStatusCodeSuccess;
                    } /* as the response has to be delayed do not send it now */
                    break;

                case (kEncapsulationCommandListInterfaces):
                    HandleReceivedListInterfacesCommand(&encapsulation_data);
                    break;

                /* The following commands are not to be sent via UDP */
                case (kEncapsulationCommandNoOperation):
                case (kEncapsulationCommandRegisterSession):
                case (kEncapsulationCommandUnregisterSession):
                case (kEncapsulationCommandSendRequestReplyData):
                case (kEncapsulationCommandSendUnitData):
                default:
                    encapsulation_data.status = kEncapsulationProtocolInvalidCommand;
                    encapsulation_data.data_length = 0;
                    break;
            }
            /* if nRetVal is greater than 0 data has to be sent */
            if (0 < status.status)
            {
                status = (CipStatus)EncapsulateData(&encapsulation_data);
            }
        }
    }
    return status.status;
}

int NET_EthIP_Encap::EncapsulateData(const EncapsulationData* const send_data)
{
    CipUsint* communcation_buffer = send_data->communication_buffer_start + 2;
    NET_Endianconv::AddIntToMessage(send_data->data_length, communcation_buffer);
    /*the CommBuf should already contain the correct session handle*/
    NET_Endianconv::MoveMessageNOctets(4, communcation_buffer);
    NET_Endianconv::AddDintToMessage(send_data->status, communcation_buffer);
    /*the CommBuf should already contain the correct sender context*/
    /*the CommBuf should already contain the correct  options value*/

    return ENCAPSULATION_HEADER_LENGTH + send_data->data_length;
}

/** @brief generate reply with "Communications Services" + compatibility Flags.
 *  @param receive_data pointer to structure with received data
 */
void NET_EthIP_Encap::HandleReceivedListServicesCommand(EncapsulationData* receive_data)
{
    CipUsint* communication_buffer = receive_data->current_communication_buffer_position;

    receive_data->data_length = (CipUint) (g_interface_information.length + 2);

    /* copy Interface data to msg for sending */
    NET_Endianconv::AddIntToMessage(1, communication_buffer);

    NET_Endianconv::AddIntToMessage(g_interface_information.type_code, communication_buffer);

    NET_Endianconv::AddIntToMessage((CipUint)(g_interface_information.length - 4), communication_buffer);

    NET_Endianconv::AddIntToMessage(g_interface_information.encapsulation_protocol_version, communication_buffer);

    NET_Endianconv::AddIntToMessage(g_interface_information.capability_flags, communication_buffer);

    memcpy(communication_buffer, g_interface_information.name_of_service, sizeof(g_interface_information.name_of_service));
}

void NET_EthIP_Encap::HandleReceivedListInterfacesCommand(EncapsulationData* receive_data)
{
    CipUsint* communication_buffer = receive_data->current_communication_buffer_position;
    receive_data->data_length = 2;
    NET_Endianconv::AddIntToMessage(0x0000, communication_buffer); /* copy Interface data to msg for sending */
}

void NET_EthIP_Encap::HandleReceivedListIdentityCommandTcp(EncapsulationData* receive_data)
{
    receive_data->data_length = (CipUint) EncapsulateListIdentyResponseMessage(receive_data->current_communication_buffer_position);
}

void NET_EthIP_Encap::HandleReceivedListIdentityCommandUdp(int socket, struct sockaddr_in* from_address, EncapsulationData* receive_data)
{
    DelayedEncapsulationMessage* delayed_message_buffer = nullptr;

    for (unsigned int i = 0; i < ENCAP_NUMBER_OF_SUPPORTED_DELAYED_ENCAP_MESSAGES; i++)
    {
        if (kEipInvalidSocket == g_delayed_encapsulation_messages[i].socket)
        {
            delayed_message_buffer = &(g_delayed_encapsulation_messages[i]);
            break;
        }
    }

    if (nullptr != delayed_message_buffer)
    {
        delayed_message_buffer->socket = socket;
        memcpy((&delayed_message_buffer->receiver), from_address, sizeof(struct sockaddr_in));

        DetermineDelayTime(receive_data->communication_buffer_start, delayed_message_buffer);

        memcpy(&(delayed_message_buffer->message[0]), receive_data->communication_buffer_start, ENCAPSULATION_HEADER_LENGTH);

        delayed_message_buffer->message_size = (unsigned int) EncapsulateListIdentyResponseMessage(
                    &(delayed_message_buffer->message[ENCAPSULATION_HEADER_LENGTH]));

        CipUsint* communication_buffer = delayed_message_buffer->message + 2;
        NET_Endianconv::AddIntToMessage((CipUint) delayed_message_buffer->message_size, communication_buffer);
        delayed_message_buffer->message_size += ENCAPSULATION_HEADER_LENGTH;
    }
}

int NET_EthIP_Encap::EncapsulateListIdentyResponseMessage(CipByte* const communication_buffer)
{
    CipUsint* communication_buffer_runner = communication_buffer;

    NET_Endianconv::AddIntToMessage(1, communication_buffer_runner); /* Item count: one item */
    NET_Endianconv::AddIntToMessage(CIP_CommonPacket::kCipItemIdListIdentityResponse, communication_buffer_runner);

    CipByte* id_length_buffer = communication_buffer_runner;
    communication_buffer_runner += 2; /*at this place the real length will be inserted below*/

    NET_Endianconv::AddIntToMessage((CipUint) kSupportedProtocolVersion, communication_buffer_runner);

    EncapsulateIpAddress(NET_Connection::endian_htons ((uint16_t) kOpENerEthernetPort),
                         CIP_TCPIP_Interface::interface_configuration_.ip_address, communication_buffer_runner);

    memset(communication_buffer_runner, 0, 8);

    communication_buffer_runner += 8;

    const CIP_Identity * identity_instance = CIP_Identity::GetInstance(0);

    NET_Endianconv::AddIntToMessage(identity_instance->vendor_id, communication_buffer_runner);

    NET_Endianconv::AddIntToMessage(identity_instance->device_type, communication_buffer_runner);

    NET_Endianconv::AddIntToMessage(identity_instance->product_code, communication_buffer_runner);

    *(communication_buffer_runner)++ = identity_instance->revision.major_revision;

    *(communication_buffer_runner)++ = identity_instance->revision.minor_revision;

    NET_Endianconv::AddIntToMessage(identity_instance->status.val, communication_buffer_runner);

    NET_Endianconv::AddDintToMessage(identity_instance->serial_number, communication_buffer_runner);

    *communication_buffer_runner++ = (unsigned char)identity_instance->product_name.length;

    memcpy(communication_buffer_runner, identity_instance->product_name.string, identity_instance->product_name.length);

    communication_buffer_runner += identity_instance->product_name.length;

    *communication_buffer_runner++ = 0xFF;

    // the -2 is for not counting the length field
    NET_Endianconv::AddIntToMessage((CipUint) (communication_buffer_runner - id_length_buffer - 2), id_length_buffer);

    return (int) (communication_buffer_runner - communication_buffer);
}

void NET_EthIP_Encap::DetermineDelayTime(CipByte* buffer_start,  DelayedEncapsulationMessage* delayed_message_buffer)
{

    buffer_start += 12; /* start of the sender context */
    CipUint maximum_delay_time = NET_Endianconv::GetIntFromMessage(buffer_start);

    if (0 == maximum_delay_time)
    {
        maximum_delay_time = (CipUint) kListIdentityDefaultDelayTime;
    }
    else if (kListIdentityMinimumDelayTime > maximum_delay_time)
    {
        // if maximum_delay_time is between 1 and 500ms set it to 500ms
        maximum_delay_time = (CipUint) kListIdentityMinimumDelayTime;
    }

    // Sets delay time between 0 and maximum_delay_time
    delayed_message_buffer->time_out = (maximum_delay_time);//todo: add randomic factor
}

/* @brief Check supported protocol, generate session handle, send replay back to originator.
 * @param socket Socket this request is associated to. Needed for double register check
 * @param receive_data Pointer to received data with request/response.
 */
void NET_EthIP_Encap::HandleReceivedRegisterSessionCommand(int socket,
    EncapsulationData* receive_data)
{
    int session_index = 0;
    CipUsint* receive_data_buffer;
    CipUint protocol_version = NET_Endianconv::GetIntFromMessage(receive_data->current_communication_buffer_position);
    CipUint nOptionFlag = NET_Endianconv::GetIntFromMessage(receive_data->current_communication_buffer_position);

    /* check if requested protocol version is supported and the register session option flag is zero*/
    if ((0 < protocol_version) && (protocol_version <= kSupportedProtocolVersion)
        && (0 == nOptionFlag)) { /*Option field should be zero*/
        /* check if the socket has already a session open */
        for (int i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; ++i) {
            if (g_registered_sessions[i] == socket) {
                /* the socket has already registered a session this is not allowed*/
                receive_data->session_handle = (CipUdint) (i + 1); /*return the already assigned session back, the cip spec is not clear about this needs to be tested*/
                receive_data->status = kEncapsulationProtocolInvalidCommand;
                session_index = kSessionStatusInvalid;
                receive_data_buffer = &receive_data->communication_buffer_start[kEncapsulationHeaderSessionHandlePosition];
                NET_Endianconv::AddDintToMessage(receive_data->session_handle, receive_data_buffer); /*EncapsulateData will not update the session handle so we have to do it here by hand*/
                break;
            }
        }

        if (kSessionStatusInvalid != session_index) {
            session_index = GetFreeSessionIndex();
            if (kSessionStatusInvalid == session_index) /* no more sessions available */
            {
                receive_data->status = kEncapsulationProtocolInsufficientMemory;
            }
            else
            { /* successful session registered */
                g_registered_sessions[session_index] = socket; /* store associated socket */
                receive_data->session_handle = (CipUdint) (session_index + 1);
                receive_data->status = kEncapsulationProtocolSuccess;
                receive_data_buffer = &receive_data->communication_buffer_start[kEncapsulationHeaderSessionHandlePosition];
                NET_Endianconv::AddDintToMessage(receive_data->session_handle, receive_data_buffer); /*EncapsulateData will not update the session handle so we have to do it here by hand*/
            }
        }
    } else { /* protocol not supported */
        receive_data->status = kEncapsulationProtocolUnsupportedProtocol;
    }

    receive_data->data_length = 4;
}

/*   INT8 UnregisterSession(struct S_Encapsulation_Data *pa_S_ReceiveData)
 *   close all corresponding TCP connections and delete session handle.
 *      pa_S_ReceiveData pointer to unregister session request with corresponding socket handle.
 */
CipStatus NET_EthIP_Encap::HandleReceivedUnregisterSessionCommand(
    EncapsulationData* receive_data)
{
    int i;

    if ((0 < receive_data->session_handle)
        && (receive_data->session_handle <= OPENER_NUMBER_OF_SUPPORTED_SESSIONS)) {
        i = receive_data->session_handle - 1;
        if (kEipInvalidSocket != g_registered_sessions[i])
        {
            //IApp_CloseSocket_tcp(g_registered_sessions[i]);
            g_registered_sessions[i] = kEipInvalidSocket;
            return kCipGeneralStatusCodeSuccess;
        }
    }

    /* no such session registered */
    receive_data->data_length = 0;
    receive_data->status = kEncapsulationProtocolInvalidSessionHandle;
    return kCipGeneralStatusCodeSuccess;
}

/** @brief Call Connection Manager.
 *  @param receive_data Pointer to structure with data and header information.
 */
CipStatus NET_EthIP_Encap::HandleReceivedSendUnitDataCommand(EncapsulationData* receive_data)
{
    CipInt send_size;
    CipStatus return_value = kCipGeneralStatusCodeSuccess;

    if (receive_data->data_length >= 6) {
        /* Command specific data UDINT .. Interface Handle, UINT .. Timeout, CPF packets */
        /* don't use the data yet */
        NET_Endianconv::GetDintFromMessage(receive_data->current_communication_buffer_position); /* skip over null interface handle*/
        NET_Endianconv::GetIntFromMessage(receive_data->current_communication_buffer_position); /* skip over unused timeout value*/
        receive_data->data_length -= 6; /* the rest is in CPF format*/

        if (kSessionStatusValid == CheckRegisteredSessions(receive_data)) /* see if the EIP session is registered*/
        {
            send_size = (CipInt) CIP_CommonPacket::NotifyConnectedCommonPacketFormat(receive_data, &receive_data->communication_buffer_start[ENCAPSULATION_HEADER_LENGTH]);

            if (0 < send_size)
            { /* need to send reply */
                receive_data->data_length = (CipUint) send_size;
            }
            else
            {
                return_value = kCipStatusError;
            }
        }
        else
        { /* received a package with non registered session handle */
            receive_data->data_length = 0;
            receive_data->status = kEncapsulationProtocolInvalidSessionHandle;
        }
    }
    return return_value;
}

/** @brief Call UCMM or Message Router if UCMM not implemented.
 *  @param receive_data Pointer to structure with data and header information.
 *  @return status 	0 .. success.
 * 					-1 .. error
 */
CipStatus NET_EthIP_Encap::HandleReceivedSendRequestResponseDataCommand(EncapsulationData* receive_data)
{
    CipInt send_size;
    CipStatus return_value = kCipGeneralStatusCodeSuccess;

    if (receive_data->data_length >= 6) {
        /* Command specific data UDINT .. Interface Handle, UINT .. Timeout, CPF packets */
        /* don't use the data yet */
        NET_Endianconv::GetDintFromMessage(receive_data->current_communication_buffer_position); /* skip over null interface handle*/
        NET_Endianconv::GetIntFromMessage(receive_data->current_communication_buffer_position); /* skip over unused timeout value*/
        receive_data->data_length -= 6; /* the rest is in CPF format*/

        if (kSessionStatusValid == CheckRegisteredSessions(receive_data)) /* see if the EIP session is registered*/
        {
            send_size = (CipInt) CIP_CommonPacket::NotifyCommonPacketFormat(receive_data, &receive_data->communication_buffer_start[ENCAPSULATION_HEADER_LENGTH]);

            if (send_size >= 0)
            {
                // need to send reply
                receive_data->data_length = (CipUint) send_size;
            }
            else
            {
                return_value = kCipStatusError;
            }
        }
        else
        {
            // received a package with non registered session handle
            receive_data->data_length = 0;
            receive_data->status = kEncapsulationProtocolInvalidSessionHandle;
        }
    }
    return return_value;
}

/** @brief search for available sessions an return index.
 *  @return return index of free session in anRegisteredSessions.
 * 			kInvalidSession .. no free session available
 */
int NET_EthIP_Encap::GetFreeSessionIndex(void)
{
    for (int session_index = 0; session_index < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; session_index++)
    {
        if (kEipInvalidSocket == g_registered_sessions[session_index])
        {
            return session_index;
        }
    }
    return kSessionStatusInvalid;
}

/** @brief copy data from pa_buf in little endian to host in structure.
 * @param receive_buffer
 * @param length Length of the data in receive_buffer. Might be more than one message
 * @param encapsulation_data	structure to which data shall be copied
 * @return return difference between bytes in pa_buf an data_length
 *  		0 .. full package received
 * 			>0 .. more than one packet received
 * 			<0 .. only fragment of data portion received
 */
CipInt NET_EthIP_Encap::CreateEncapsulationStructure(CipUsint* receive_buffer, int receive_buffer_length, EncapsulationData* encapsulation_data)
{
    encapsulation_data->communication_buffer_start = receive_buffer;
    encapsulation_data->command_code = NET_Endianconv::GetIntFromMessage(receive_buffer);
    encapsulation_data->data_length = NET_Endianconv::GetIntFromMessage(receive_buffer);
    encapsulation_data->session_handle = NET_Endianconv::GetDintFromMessage(receive_buffer);
    encapsulation_data->status = NET_Endianconv::GetDintFromMessage(receive_buffer);

    receive_buffer += kSenderContextSize;
    encapsulation_data->options = NET_Endianconv::GetDintFromMessage(receive_buffer);
    encapsulation_data->current_communication_buffer_position = receive_buffer;
    return (CipInt) (receive_buffer_length - ENCAPSULATION_HEADER_LENGTH - encapsulation_data->data_length);
}

/** @brief Check if received package belongs to registered session.
 *  @param receive_data Received data.
 *  @return 0 .. Session registered
 *  		kInvalidSession .. invalid session -> return unsupported command received
 */
SessionStatus NET_EthIP_Encap::CheckRegisteredSessions(EncapsulationData* receive_data)
{
    if ((0 < receive_data->session_handle) && (receive_data->session_handle <= OPENER_NUMBER_OF_SUPPORTED_SESSIONS))
    {
        if (kEipInvalidSocket != g_registered_sessions[receive_data->session_handle - 1])
        {
            return kSessionStatusValid;
        }
    }
    return kSessionStatusInvalid;
}

void NET_EthIP_Encap::CloseSession(int socket)
{
    int i;
    for (i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; ++i)
    {
        if (g_registered_sessions[i] == socket)
        {
            //IApp_CloseSocket_tcp(socket);
            g_registered_sessions[i] = kEipInvalidSocket;
            break;
        }
    }
}

bool NET_EthIP_Encap::EncapsulationShutdown()
{
    if (initialized)
    {
        for (int i = 0; i < OPENER_NUMBER_OF_SUPPORTED_SESSIONS; ++i)
        {
            if (kEipInvalidSocket != g_registered_sessions[i])
            {
                //IApp_CloseSocket_tcp(g_registered_sessions[i]);
                g_registered_sessions[i] = kEipInvalidSocket;
            }
        }
        CIP_EthernetIP_Link::Shut();
        initialized=false;
        return true;
    }
    return false;
}

void NET_EthIP_Encap::ManageEncapsulationMessages(MilliSeconds elapsed_time)
{
    for (unsigned int i = 0; i < ENCAP_NUMBER_OF_SUPPORTED_DELAYED_ENCAP_MESSAGES; i++)
    {
        if (kEipInvalidSocket != g_delayed_encapsulation_messages[i].socket)
        {
            g_delayed_encapsulation_messages[i].time_out -= elapsed_time;
            if (0 > g_delayed_encapsulation_messages[i].time_out)
            {
                // If delay is reached or passed, send the UDP message
                NET_NetworkHandler::SendUdpData((struct sockaddr*)g_delayed_encapsulation_messages[i].receiver, g_delayed_encapsulation_messages[i].socket, &(g_delayed_encapsulation_messages[i].message[0]),
                                                (CipUint) g_delayed_encapsulation_messages[i].message_size);g_delayed_encapsulation_messages[i].socket = -1;
            }
        }
    }
}
