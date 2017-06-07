//
// Created by Gabriel Ferreira (@gabrielcarvfer) on 20/04/2017.
//

#include "TEST_Cip_Connection.hpp"
#include <iostream>
#include <cip/ciptypes.hpp>


bool test_allocation()
{
    CipStatus stat;

    stat = CIP_Connection::Create(nullptr, nullptr);

    if (stat.status != kCipStatusOk)
        return false;

    return true;
}

bool test_class_services()
{
    CIP_Connection * class_instance = (CIP_Connection*)CIP_Connection::GetInstance(0);

    CipMessageRouterRequest_t req;
    CipMessageRouterResponse_t resp;
    CipStatus stat;

    //Test instance creation (service 0x08)
        //stat = class_instance->InstanceServices(0x08,&req,&resp);

    //Test instance deletion (service 0x09)
        //stat = class_instance->InstanceServices(0x09,&req,&resp);

    //Test reset (service 0x05)
        //stat = class_instance->InstanceServices(0x05,&req,&resp);

    //Test find_next_object_instance (service 0x11)
        //stat = class_instance->InstanceServices(0x11,&req,&resp);

    //Test Get_Attribute_Single (service 0x0E)
        //stat = class_instance->InstanceServices(0x0E,&req,&resp);

    //Test Connection bind (service 0x4B)
        CIP_Connection::Create(nullptr, nullptr);

        req.request_data.push_back(1); //Number of first connection to be bound
        req.request_data.push_back(0);
        req.request_data.push_back(2); //Number of second connection to be bound
        req.request_data.push_back(0);
        stat = class_instance->InstanceServices(0x4B,&req,&resp);

        if (stat.status != kCipGeneralStatusCodeObjectStateConflict) //As connections are not set as configured
            return false;

        //Set connections as established and try to bind again
        CIP_Connection* conn = (CIP_Connection*)CIP_Connection::GetInstance(1);
        conn->State = CIP_Connection::kConnectionStateEstablished;

        conn = (CIP_Connection*)CIP_Connection::GetInstance(2);
        conn->State = CIP_Connection::kConnectionStateEstablished;

        stat = class_instance->InstanceServices(0x4B,&req,&resp);

        if (stat.status != kCipGeneralStatusCodeSuccess) //As connections are not set as configured
            return false;

    //Test producing application lookup (service 0x4C)
        //req.request_data = EPATH to Producing application
        CipEpath path;

        path.to_bytes(&req.request_data);

        stat = class_instance->InstanceServices(0x4C,&req,&resp);

        //resp.response_data = Uint with instanceCount + uint array with conn. instance list
        if (stat.status != kCipGeneralStatusCodeSuccess)
            return false;

    //Test safety close (service 0x4E)
        //stat = class_instance->InstanceServices(0x4E,&req,&resp);

    //Test safety open (service 0x54)
        //stat = class_instance->InstanceServices(0x54,&req,&resp);

    return true;
}

bool test_instance_services()
{
    //Test GetAttributeSingle (service 0x0E)

    //Test SetAttributeSingle (service 0x10)

    //Test Reset (service 0x05)

    //Test Delete (service 0x09)
}

int main()
{
	CIP_Connection::Init();

    if ( !test_allocation() )
        exit(-1);

	if ( !test_class_services() )
        exit(-1);

    if ( !test_instance_services() )
        exit(-1);

	CIP_Connection::Shut();

	return 0;
}