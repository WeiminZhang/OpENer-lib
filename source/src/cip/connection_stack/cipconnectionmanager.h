/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#pragma once

#include "ciptypes.h"
#include "opener_api.h"
#include "opener_user_conf.h"
#include "typedefs.h"
#include "src/cip/class_stack/CIP_Class.h"
#include "cpf.h"
#include <map>

/**
 * @brief Sets the routing type of a connection, either
 * - Point-to-point connections (unicast)
 * - Multicast connection
 */
typedef enum {
    kRoutingTypePointToPointConnection = 0x4000,
    kRoutingTypeMulticastConnection = 0x2000
} RoutingType;

/** @brief Connection Manager Error codes */
typedef enum {
    kConnectionManagerStatusCodeSuccess = 0x00,
    kConnectionManagerStatusCodeErrorConnectionInUse = 0x0100,
    kConnectionManagerStatusCodeErrorTransportTriggerNotSupported = 0x0103,
    kConnectionManagerStatusCodeErrorOwnershipConflict = 0x0106,
    kConnectionManagerStatusCodeErrorConnectionNotFoundAtTargetApplication = 0x0107,
    kConnectionManagerStatusCodeErrorInvalidOToTConnectionType = 0x123,
    kConnectionManagerStatusCodeErrorInvalidTToOConnectionType = 0x124,
    kConnectionManagerStatusCodeErrorInvalidOToTConnectionSize = 0x127,
    kConnectionManagerStatusCodeErrorInvalidTToOConnectionSize = 0x128,
    kConnectionManagerStatusCodeErrorNoMoreConnectionsAvailable = 0x0113,
    kConnectionManagerStatusCodeErrorVendorIdOrProductcodeError = 0x0114,
    kConnectionManagerStatusCodeErrorDeviceTypeError = 0x0115,
    kConnectionManagerStatusCodeErrorRevisionMismatch = 0x0116,
    kConnectionManagerStatusCodeInvalidConfigurationApplicationPath = 0x0129,
    kConnectionManagerStatusCodeInvalidConsumingApllicationPath = 0x012A,
    kConnectionManagerStatusCodeInvalidProducingApplicationPath = 0x012B,
    kConnectionManagerStatusCodeInconsistentApplicationPathCombo = 0x012F,
    kConnectionManagerStatusCodeNonListenOnlyConnectionNotOpened = 0x0119,
    kConnectionManagerStatusCodeErrorParameterErrorInUnconnectedSendService = 0x0205,
    kConnectionManagerStatusCodeErrorInvalidSegmentTypeInPath = 0x0315,
    kConnectionManagerStatusCodeTargetObjectOutOfConnections = 0x011A
} ConnectionManagerStatusCode;

typedef enum {
    kConnectionTriggerTypeProductionTriggerMask = 0x70,
    kConnectionTriggerTypeCyclicConnection = 0x0,
    kConnectionTriggerTypeChangeOfStateTriggeredConnection = 0x10,
    kConnectionTriggerTypeApplicationTriggeredConnection = 0x20
} ConnectionTriggerType;

/** @brief macros for comparing sequence numbers according to CIP spec vol
 * 2 3-4.2 for int type variables
 * @define SEQ_LEQ32(a, b) Checks if sequence number a is less or equal than b
 * @define SEQ_GEQ32(a, b) Checks if sequence number a is greater or equal than
 *  b
 *  @define SEQ_GT32(a, b) Checks if sequence number a is greater than b
 */
#define SEQ_LEQ32(a, b) ((int)((a) - (b)) <= 0)
#define SEQ_GEQ32(a, b) ((int)((a) - (b)) >= 0)
#define SEQ_GT32(a, b) ((int)((a) - (b)) > 0)

/** @brief similar macros for comparing 16 bit sequence numbers
 * @define SEQ_LEQ16(a, b) Checks if sequence number a is less or equal than b
 * @define SEQ_GEQ16(a, b) Checks if sequence number a is greater or equal than
 *  b
 */
#define SEQ_LEQ16(a, b) ((short)((a) - (b)) <= 0)
#define SEQ_GEQ16(a, b) ((short)((a) - (b)) >= 0)

/** @brief States of a connection */
typedef enum {
    kConnectionStateNonExistent = 0,
    kConnectionStateConfiguring = 1,
    kConnectionStateWaitingForConnectionId = 2 /**< only used in DeviceNet */,
    kConnectionStateEstablished = 3,
    kConnectionStateTimedOut = 4,
    kConnectionStateDeferredDelete = 5 /**< only used in DeviceNet */,
    kConnectionStateClosing
} ConnectionState;

/** @brief instance_type attributes */
typedef enum {
    kConnectionTypeExplicit = 0,
    kConnectionTypeIoExclusiveOwner = 0x01,
    kConnectionTypeIoInputOnly = 0x11,
    kConnectionTypeIoListenOnly = 0x21
} ConnectionType;

/** @brief Possible values for the watch dog time out action of a connection */
typedef enum {
    kWatchdogTimeoutActionTransitionToTimedOut = 0, /**< , invalid for explicit message connections */
    kWatchdogTimeoutActionAutoDelete = 1, /**< Default for explicit message connections,
   default for I/O connections on EIP */
    kWatchdogTimeoutActionAutoReset = 2, /**< Invalid for explicit message connections */
    kWatchdogTimeoutActionDeferredDelete = 3 /**< Only valid for DeviceNet, invalid for I/O connections */
} WatchdogTimeoutAction;

typedef struct {
    ConnectionState state;
    CipUint connection_id;
    /*TODO think if this is needed anymore
 TCMReceiveDataFunc m_ptfuncReceiveData; */
} LinkConsumer;

typedef struct {
    ConnectionState state;
    CipUint connection_id;
} LinkProducer;

typedef struct {
    LinkConsumer consumer;
    LinkProducer producer;
} LinkObject;


typedef struct {
    CipUdint class_id;
    OpenConnectionFunction open_connection_function;
} ConnectionManagementHandling;

CipStatus (*OpenConnectionPtr) (ConnectionObject* connection_object, CipUint* extended_error);

class ConnectionObject : public CIP_Class
{

public:
    static const int classCode = kCipConnectionManagerClassCode;
    ConnectionObject(CipUint unique_connection_id);
    ConnectionObject() = delete ;
    ~ConnectionObject();

    /** @brief Initialize the data of the connection manager object
     */
    static CipStatus ConnectionManagerInit(CipUint unique_connection_id);

    /** @brief Get a connected object dependent on requested ConnectionID.
     *
     *   @param connection_id  requested @var connection_id of opened connection
     *   @return pointer to connected Object
     *           0 .. connection not present in device
     */
    ConnectionObject* GetConnectedObject(CipUdint connection_id);

    /**  Get a connection object for a given output assembly.
     *
     *   @param output_assembly_id requested output assembly of requested
     *connection
     *   @return pointer to connected Object
     *           0 .. connection not present in device
     */
    static ConnectionObject* GetConnectedOutputAssembly(CipUdint output_assembly_id);

    // Copy the given connection data from pa_pstSrc to pa_pstDst
    static void CopyConnectionData(ConnectionObject* destination, ConnectionObject* source);

    /** @brief Close the given connection
     *
     * This function will take the data form the connection and correctly closes the
     *connection (e.g., open sockets)
     * @param connection_object pointer to the connection object structure to be
     *closed
     */
    static void CloseConnection(ConnectionObject* connection_object);

    /* TODO: Missing documentation */
    static CipBool IsConnectedOutputAssembly(CipUdint instance_number);

    /** @brief Generate the ConnectionIDs and set the general configuration
     * parameter in the given connection object.
     *
     * @param connection_object pointer to the connection object that should be set
     *up.
     */
    void GeneralConnectionConfiguration();

    /** @brief Insert the given connection object to the list of currently active
     *  and managed connections.
     *
     * By adding a connection to the active connection list the connection manager
     * will perform the supervision and handle the timing (e.g., timeout,
     * production inhibit, etc).
     *
     * @param connection_object pointer to the connection object to be added.
     */
   static void AddNewActiveConnection(ConnectionObject* connection_object);

    /* TODO: Missing documentation */
    static void RemoveFromActiveConnections(ConnectionObject* connection_object);


    static std::map<CipUdint, ConnectionObject *> active_connections_set;

    /** List holding information on the object classes and open/close function
     * pointers to which connections may be established.
     */
    static ConnectionManagementHandling g_astConnMgmList[2 + OPENER_CIP_NUM_APPLICATION_SPECIFIC_CONNECTABLE_OBJECTS];

    /** buffer connection object needed for forward open */
    static ConnectionObject g_dummy_connection_object;

    /** @brief Holds the connection ID's "incarnation ID" in the upper 16 bits */
    static CipUdint g_incarnation_id;

    /* Beginning of instance info */
    /** The data needed for handling connections. This data is strongly related to
     * the connection object defined in the CIP-specification. However the full
     * functionality of the connection object is not implemented. Therefore this
     * data can not be accessed with CIP means.
     */
    ConnectionState state;
    ConnectionType instance_type;

    /* conditional
   CipUint DeviceNetProductedConnectionID;
   CipUint DeviceNetConsumedConnectionID;
   */
    CipByte device_net_initial_comm_characteristcs;
    CipUint produced_connection_size;
    CipUint consumed_connection_size;
    CipUint expected_packet_rate;

    /*conditional*/
    CipUdint produced_connection_id;
    CipUdint consumed_connection_id;
    /**/
    WatchdogTimeoutAction watchdog_timeout_action;
    CipUint produced_connection_path_length;
    CipEpath produced_connection_path;
    CipUint consumed_connection_path_length;
    CipEpath consumed_connection_path;
    /* conditional
   UINT16 ProductionInhibitTime;
   */
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
    CipByte transport_type_class_trigger;
    CipUsint connection_path_size;
    CipElectronicKey electronic_key;
    CipConnectionPath connection_path; // padded EPATH
    LinkObject link_object;

    CIP_Class* consuming_instance;
    /*S_CIP_CM_Object *p_stConsumingCMObject; */

    CIP_Class* producing_instance;
    /*S_CIP_CM_Object *p_stProducingCMObject; */

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

    // socket address for produce
    struct sockaddr_in remote_address;

    // the address of the originator that established the connection. needed for
    // scanning if the right packet is arriving
    struct sockaddr_in originator_address;
    int socket[2]; /* socket handles, indexed by kConsuming or kProducing */

    /* pointers to connection handling functions */
    ConnectionCloseFunction connection_close_function;
    ConnectionTimeoutFunction connection_timeout_function;
    ConnectionSendDataFunction connection_send_data_function;
    ConnectionReceiveDataFunction connection_receive_data_function;

    CipUint correct_originator_to_target_size;
    CipUint correct_target_to_originator_size;


    /* private functions */
    CipStatus ForwardOpen(CIP_Class* instance,
                          CipMessageRouterRequest* message_router_request,
                          CipMessageRouterResponse* message_router_response);

    CipStatus ForwardClose(CIP_Class* instance,
                           CipMessageRouterRequest* message_router_request,
                           CipMessageRouterResponse* message_router_response);

    CipStatus GetConnectionOwner(CIP_Class* instance,
                                 CipMessageRouterRequest* message_router_request,
                                 CipMessageRouterResponse* message_router_response);

    CipStatus AssembleForwardOpenResponse(
            ConnectionObject* connection_object,
            CipMessageRouterResponse* message_router_response, CipUsint general_status,
            CipUint extended_status);

    CipStatus AssembleForwardCloseResponse(
            CipUint connection_serial_number, CipUint originatior_vendor_id,
            CipUdint originator_serial_number,
            CipMessageRouterRequest* message_router_request,
            CipMessageRouterResponse* message_router_response,
            CipUint extended_error_code);

    /** @brief check if the data given in the connection object match with an already established connection
     *
     * The comparison is done according to the definitions in the CIP specification Section 3-5.5.2:
     * The following elements have to be equal: Vendor ID, Connection Serial Number, Originator Serial Number
     * @param connection_object connection object containing the comparison elements from the forward open request
     * @return
     *    - NULL if no equal established connection exists
     *    - pointer to the equal connection object
     */
    ConnectionObject* CheckForExistingConnection(ConnectionObject* connection_object);

    /** @brief Compare the electronic key received with a forward open request with the device's data.
     *
     * @param key_format format identifier given in the forward open request
     * @param key_data pointer to the electronic key data received in the forward open request
     * @param extended_status the extended error code in case an error happened
     * @return general status on the establishment
     *    - EIP_OK ... on success
     *    - On an error the general status code to be put into the response
     */
    CipStatus CheckElectronicKeyData(CipUsint key_format, CipKeyData* key_data, CipUint* extended_status);

    /** @brief Parse the connection path of a forward open request
     *
     * This function will take the connection object and the received data stream and parse the connection path.
     * @param connection_object pointer to the connection object structure for which the connection should
     *                      be established
     * @param message_router_request pointer to the received request structure. The position of the data stream pointer has to be at the connection length entry
     * @param extended_status the extended error code in case an error happened
     * @return general status on the establishment
     *    - EIP_OK ... on success
     *    - On an error the general status code to be put into the response
     */
    CipUsint ParseConnectionPath(ConnectionObject* connection_object, CipMessageRouterRequest* message_router_request, CipUint* extended_error);

    ConnectionManagementHandling* GetConnMgmEntry(CipUdint class_id);

    void InitializeConnectionManagerData(void);

    void AddNullAddressItem(CipCommonPacketFormatData* common_data_packet_format_data);

    CipStatus TriggerConnections(CipUdint pa_unOutputAssembly, CipUdint pa_unInputAssembly);
    static CipStatus AddConnectableObject(CipUdint pa_nClassId, OpenConnectionFunction pa_pfOpenFunc);
    static unsigned int GetPaddedLogicalPath(unsigned char** logical_path_segment);
    static CipStatus ManageConnections (MilliSeconds elapsed_time);
    CipUdint GetConnectionId (void);
    CipStatus HandleReceivedConnectedData (CipUsint *data, int data_length, struct sockaddr_in *from_address);


    /* End of instance info */

};
