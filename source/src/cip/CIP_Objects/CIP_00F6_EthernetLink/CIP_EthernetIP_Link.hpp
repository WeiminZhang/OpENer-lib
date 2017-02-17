/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved.
 *
 ******************************************************************************/
#ifndef OPENER_CIPETHERNETLINK_H_
#define OPENER_CIPETHERNETLINK_H_

#include "../../ciptypes.hpp"
#include "../template/CIP_Object.hpp"
#include "cip/CIP_Objects/CIP_00F5_TCPIP_Interface/CIP_TCPIP_Interface.hpp"

class CIP_EthernetIP_Link : public CIP_Object<CIP_EthernetIP_Link>
{
public:
    /** @brief Initialize the Ethernet Link Objects data
    */
    static CipStatus Init();
    static CipStatus Shutdown();
    CIP_TCPIP_Interface * associatedInterface;
private:
    //Definitions
    typedef struct {
        CipUdint interface_speed;
        CipUdint interface_flags;
        CipUsint physical_address[6];
    } CipEthernetLinkObject;



    //Methods
    void ConfigureMacAddress(const CipUsint* mac_address);

    //Variables
    CipEthernetLinkObject g_ethernet_link;
    CipStatus InstanceServices(int service, CipMessageRouterRequest * msg_router_request,CipMessageRouterResponse* msg_router_response);

};

#endif /* OPENER_CIPETHERNETLINK_H_*/