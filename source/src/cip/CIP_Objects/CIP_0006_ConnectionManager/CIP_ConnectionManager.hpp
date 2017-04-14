/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIP_CONNECTION_H_
#define OPENER_CIP_CONNECTION_H_


#include <map>
#include <cip/CIP_Objects/CIP_0005_Connection/CIP_Connection.hpp>
#include <cip/CIP_ElectronicKey.hpp>
#include <cip/CIP_Segment.hpp>
#include "../../ciptypes.hpp"
#include "../../connection/CIP_CommonPacket.hpp"
#include "../template/CIP_Object.hpp"
#include "../../connection/network/NET_Connection.hpp"
#include "../../../OpENer_Interface.hpp"
#include "../CIP_0004_Assembly/CIP_Assembly.hpp"

class CIP_ConnectionManager;

class CIP_ConnectionManager :   public CIP_Object<CIP_ConnectionManager>
{

public:
    static CipStatus Init();
    static CipStatus Shut();

    CipMessageRouterRequest_t  g_message_router_request;
    CipMessageRouterResponse_t g_message_router_response;

    /**
 * @brief Sets the routing type of a connection, either
 * - Point-to-point connections (unicast)
 * - Multicast connection
 */
    typedef enum
    {
        kRoutingTypePointToPointConnection = 0x4000,
        kRoutingTypeMulticastConnection    = 0x2000
    } RoutingType;

/** @brief Connection Manager Error codes */
    typedef enum
    {
        kConnMgrStatusCodeSuccess                             = 0x00,
        kConnMgrStatusCodeErrorConnectionInUse                = 0x0100,
        kConnMgrStatusCodeErrorTransportTriggerNotSupported   = 0x0103,
        kConnMgrStatusCodeErrorOwnershipConflict              = 0x0106,
        kConnMgrStatusCodeErrorConnectionNotFoundAtTargetApplication = 0x0107,
        kConnMgrStatusCodeErrorInvalidOToTConnectionType      = 0x123,
        kConnMgrStatusCodeErrorInvalidTToOConnectionType      = 0x124,
        kConnMgrStatusCodeErrorInvalidOToTConnectionSize      = 0x127,
        kConnMgrStatusCodeErrorInvalidTToOConnectionSize      = 0x128,
        kConnMgrStatusCodeErrorNoMoreConnectionsAvailable     = 0x0113,
        kConnMgrStatusCodeErrorVendorIdOrProductcodeError     = 0x0114,
        kConnMgrStatusCodeErrorDeviceTypeError                = 0x0115,
        kConnMgrStatusCodeErrorRevisionMismatch               = 0x0116,
        kConnMgrStatusCodeInvalidConfigurationApplicationPath = 0x0129,
        kConnMgrStatusCodeInvalidConsumingApllicationPath     = 0x012A,
        kConnMgrStatusCodeInvalidProducingApplicationPath     = 0x012B,
        kConnMgrStatusCodeInconsistentApplicationPathCombo    = 0x012F,
        kConnMgrStatusCodeNonListenOnlyConnectionNotOpened    = 0x0119,
        kConnMgrStatusCodeErrorParameterErrorInUnconnectedSendService = 0x0205,
        kConnMgrStatusCodeErrorInvalidSegmentTypeInPath       = 0x0315,
        kConnMgrStatusCodeTargetObjectOutOfConnections        = 0x011A
    } ConnectionManagerStatusCode_e;

    typedef enum
    {
        kConnMgrServiceGetAttributeAll      = 0x01,
        kConnMgrServiceSetAttributeAll      = 0x02,
        kConnMgrServiceGetAttributeSingle   = 0x0E,
        kConnMgrServiceSetAttributeSingle   = 0x10,
        kConnMgrServiceUnconnectedSend      = 0x52,
        kConnMgrServiceForwardOpen          = 0x54,
        kConnMgrServiceGetConnectionData    = 0x56,
        kConnMgrServiceSearchConnectionData = 0x57,
        kConnMgrServiceObsolete             = 0x59,
        kConnMgrServiceGetConnectionOwner   = 0x5A,
        kConnMgrServiceLargeForwardOpen     = 0x5B,
    } ConnectionManagerServiceCodes_e;

    static std::map<CipUdint, const CIP_ConnectionManager *> * active_connections_set;

    /** @brief Holds the connection ID's "incarnation ID" in the upper 16 bits */
    static CipUdint g_incarnation_id;

    /* Beginning of instance info */
    /** The data needed for handling connections. This data is strongly related to
     * the connection object defined in the CIP-specification. However the full
     * functionality of the connection object is not implemented. Therefore this
     * data can not be accessed with CIP means.
     */

    typedef struct
    {
        CipUint NumconnEntries;
        CipBool ConnOpenBits[32];//todo: get size from connection max_instances
    }connection_entry_list_t;

    //CIP Attributes
    CipUint Open_requests;
    CipUint Open_format_rejects;
    CipUint Open_resource_rejects;
    CipUint Open_other_rejects;
    CipUint Close_requests;
    CipUint Close_format_requests;
    CipUint Close_other_requests;
    CipUint Connection_timeouts;
    connection_entry_list_t connection_entry_list;
    //attribute 10 is obsolete
    CipUint CPU_utilization;
    CipUdint Max_buff_size;
    CipUdint Buff_size_remaining;

    /* non CIP Attributes, only relevant for opened connections */
    CipByte priority_timetick;
    CipUsint timeout_ticks;
    CipUint connection_serial_number;
    CipUint originator_vendor_id;
    CipUdint originator_serial_number;
    CipUint connection_timeout_multiplier;
    CipUdint o_to_t_requested_packet_interval;
    CipUint o_to_t_network_connection_parameter;
    CipUdint t_to_o_requested_packet_interval;
    CipUint t_to_o_network_connection_parameter;
    CipUsint connection_path_size;
    CIP_Segment electronic_key;
    CipConnectionPath connection_path; // padded EPATH
    //todo: check with connection LinkObject link_object;

    CIP_Connection * consuming_instance;

    CIP_Connection * producing_instance;

    /* the EIP level sequence Count for Class 0/1 Producing Connections may have a different
   value than SequenceCountProducing */
    CipUdint eip_level_sequence_count_producing;

    /* the EIP level sequence Count for Class 0/1 Producing Connections may have a
       different value than SequenceCountProducing */
    CipUdint eip_level_sequence_count_consuming;

    // sequence Count for Class 1 Producing Connections
    CipUint sequence_count_producing;

    // sequence Count for Class 1 Producing Connections
    CipUint sequence_count_consuming;

    CipDint transmission_trigger_timer;
    CipDint inactivity_watchdog_timer;

    /** @brief Minimal time between the production of two application triggered
   * or change of state triggered I/O connection messages
   */
    CipUint production_inhibit_time;

    /** @brief Timer for the production inhibition of application triggered or
   * change-of-state I/O connections.
   */
    CipDint production_inhibit_timer;

    CipUint correct_originator_to_target_size;
    CipUint correct_target_to_originator_size;


    /* public functions */
    CipStatus ForwardClose(CipMessageRouterRequest_t* message_router_request,
                           CipMessageRouterResponse_t* message_router_response);

    CipStatus UnconnectedSend(CipMessageRouterRequest_t* message_router_request,
                              CipMessageRouterResponse_t* message_router_response);

    CipStatus ForwardOpen(CipMessageRouterRequest_t* message_router_request,
                          CipMessageRouterResponse_t* message_router_response);

    CipStatus GetConnectionData(CipMessageRouterRequest_t* message_router_request,
                                CipMessageRouterResponse_t* message_router_response);

    CipStatus SearchConnectionData(CipMessageRouterRequest_t* message_router_request,
                                   CipMessageRouterResponse_t* message_router_response);

    CipStatus GetConnectionOwner(CipMessageRouterRequest_t* message_router_request,
                                 CipMessageRouterResponse_t* message_router_response);

    CipStatus LargeForwardOpen(CipMessageRouterRequest_t* message_router_request,
                               CipMessageRouterResponse_t* message_router_response);

private:


    static CipUdint GetConnectionId (void);

    typedef enum
    {
        kConnMgrForwardOpenSizeFixed = 0,
        kConnMgrForwardOpenSizeVariable = 1
    } forwardOpenSizeFixedOrVariable_e;

    typedef enum
    {
        kConnMgrForwardOpenPriorityLow       = 0x00,
        kConnMgrForwardOpenPriorityHigh      = 0x01,
        kConnMgrForwardOpenPriorityScheduled = 0x10,
        kConnMgrForwardOpenPriorityUrgent    = 0x11
    } forwardOpenPriority_e;

    typedef enum
    {
        kConnMgrForwardOpenConnTypeNull      = 0x00,
        kConnMgrForwardOpenConnTypeMulticast = 0x01,
        kConnMgrForwardOpenConnTypeP2P       = 0x10,
        kConnMgrForwardOpenConnTypeReserved  = 0x11
    } forwardOpenConnType_e;

    typedef enum
    {
        kConnMgrForwardOpenRedundantOwnerNo  = 0, // Exclusive-owner, input/listen only
        kConnMgrForwardOpenRedundantOwnerYes = 1 // multiple owners can make a connection simultaniously
    } forwardOpenRedundantOwner_e;

};
#endif // OPENER_CIP_CONNECTION_H
