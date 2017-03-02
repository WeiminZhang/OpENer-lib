//
// Created by Gabriel Ferreira (@gabrielcarvfer) on 18/11/2016.
//
#ifndef CIP_OBJECT_IMPL_H
#define CIP_OBJECT_IMPL_H

#include "../../../trace.hpp"
#include "../../CIP_Common.hpp"
#include "CIP_Attribute.hpp"
#include "../../../opener_user_conf.hpp"
#include <utility>
#include <cip/ciptypes.hpp>

//Static variables
template<class T> CipUdint    CIP_Object<T>::class_id;
template<class T> std::string CIP_Object<T>::class_name;
template<class T> CipUint     CIP_Object<T>::revision;
template<class T> CipUint     CIP_Object<T>::max_instances;
template<class T> CipUint     CIP_Object<T>::number_of_instances;
template<class T> CipUdint    CIP_Object<T>::optional_attribute_list;
template<class T> CipUdint    CIP_Object<T>::optional_service_list;
template<class T> CipUint     CIP_Object<T>::maximum_id_number_class_attributes;
template<class T> CipUint     CIP_Object<T>::maximum_id_number_instance_attributes;
template<class T> std::map<CipUdint, const T *> CIP_Object<T>::object_Set;

//Methods

template <class T>
CIP_Object<T>::CIP_Object()
{
  id = number_of_instances;
  number_of_instances++;
}

template <class T>
CIP_Object<T>::~CIP_Object()
{

}

template <class T>
const T * CIP_Object<T>::GetInstance(CipUdint instance_number)
{
    if (object_Set.size() >= instance_number)
        return object_Set[instance_number];
    else
        return nullptr;
}

template <class T>
const T  * CIP_Object<T>::GetClass()
{
    return GetInstance(0);
}

template <class T>
CipUdint CIP_Object<T>::GetNumberOfInstances()
{
    return (CipUdint)object_Set.size();
}

template <class T>
CipDint CIP_Object<T>::GetInstanceNumber(const T  * instance)
{
    for (auto it = object_Set.begin(); it != object_Set.end(); it++)
    {
        if (it->second == instance)
        {
            object_Set.erase(it);
            return it->first;
        }
    }
    return -1;
}

template <class T>
bool CIP_Object<T>::AddClassInstance(T  * instance, CipUdint position)
{
    object_Set.emplace(position,instance);
    auto it = object_Set.find(position);
    return (it != object_Set.end());
}

template <class T>
bool CIP_Object<T>::RemoveClassInstance(T  * instance)
{
    for (auto it = object_Set.begin(); it != object_Set.end(); it++)
    {
        if (it->second == instance)
        {
            object_Set.erase(it);
            return true;
        }
    }
    return false;
}

template <class T>
bool CIP_Object<T>::RemoveClassInstance(CipUdint position)
{
    if ( object_Set.find(position) != object_Set.end() )
    {
        object_Set.erase (position);
        return true;
    }
    else
    {
        return false;
    }
}

//Methods
template <class T>
void CIP_Object<T>::InsertAttribute(CipUint attribute_number, CipUsint cip_type, void * data, CipAttributeFlag cip_flags)
{
    auto it = this->attributes.find(attribute_number);

    /* cant add attribute that already exists */
    if (it != this->attributes.end())
    {
        OPENER_ASSERT(true);
    }
    else
    {
        CIP_Attribute* attribute_ptr = new CIP_Attribute(attribute_number, cip_type, data, cip_flags);

        this->attributes.emplace(attribute_number, attribute_ptr);
        return;

    }
    OPENER_ASSERT(false);
    /* trying to insert too many attributes*/
}

/*
 * template <class T>
void CIP_Object::InsertService(CipUsint service_number, CipServiceFunction * service_function, std::string service_name)
{
    auto it = this->services.find(service_number);

    // cant add service that already exists
    if (it == std::map::end())
    {
        OPENER_ASSERT(true);
    }
    else
    {
        CIP_Service* p = new CIP_Service(service_number, service_function, service_name);
        this->services.emplace(service_number, p);

        return;
    }
    OPENER_ASSERT(0);
    // adding more services than were declared is a no-no
}*/

template <class T>
CIP_Attribute* CIP_Object<T>::GetCipAttribute(CipUint attribute_number)
{
    if (this->attributes.find(attribute_number) == this->attributes.end ())
    {
        OPENER_TRACE_WARN("attribute %d not defined\n", attribute_number);

        return 0;
    }
    else
    {
        return this->attributes[attribute_number];
    }


}

/* TODO: this needs to check for buffer overflow*/
template <class T>
CipStatus CIP_Object<T>::GetAttributeSingle(CipMessageRouterRequest_t* message_router_request, CipMessageRouterResponse_t* message_router_response)
{
    // Mask for filtering get-ability
    CipByte get_mask;

    CIP_Attribute* attribute = this->GetCipAttribute(message_router_request->request_path.attribute_number);
    CipByte* message = &message_router_response->response_data[0];

    message_router_response->reply_service = (0x80 | message_router_request->service);
    message_router_response->general_status = kCipErrorAttributeNotSupported;
    message_router_response->size_additional_status = 0;

    // set filter according to service: get_attribute_all or get_attribute_single
    if (kGetAttributeAll == message_router_request->service)
    {
        get_mask = kGetableAll;
        message_router_response->general_status = kCipErrorSuccess;
    }
    else
    {
        get_mask = kGetableSingle;
    }

    if ((attribute != 0) && (attribute->getData() != 0))
    {
        if (attribute->getFlag() & get_mask)
        {
            OPENER_TRACE_INFO("getAttribute %d\n",
                message_router_request->request_path.attribute_number); /* create a reply message containing the data*/
            /*TODO think if it is better to put this code in an own
               * getAssemblyAttributeSingle functions which will call get attribute
               * single.
               */

            if (attribute->getType() == kCipByteArray && this->class_id == kCipAssemblyClassCode)
            {
                // we are getting a byte array of a assembly object, kick out to the app callback
                OPENER_TRACE_INFO(" -> getAttributeSingle CIP_BYTE_ARRAY\r\n");

                //TODO:build an alternative
                //BeforeAssemblyDataSend(this);
            }
            //message_router_response->data_length = (CipInt)CIP_Common::EncodeData(attribute->getType(), attribute->getData(), &message);
            message_router_response->general_status = kCipErrorSuccess;
        }
    }

    return CipStatus(kCipStatusOkSend);
}

template <class T>
CipStatus CIP_Object<T>::GetAttributeAll(CipMessageRouterRequest_t* message_router_request, CipMessageRouterResponse_t* message_router_response)
{
    int i, j;
    CipOctet* reply;

    // pointer into the reply
    reply = message_router_response->response_data;

    if (this->id == 2)
    {
        OPENER_TRACE_INFO("GetAttributeAll: instance number 2\n");
    }

    CIP_Service * service;
    CIP_Attribute* attribute;
    for (i = 0; i < this->services.size(); i++) /* hunt for the GET_ATTRIBUTE_SINGLE service*/
    {
        // found the service
        if (this->services[i]->getNumber () == kGetAttributeSingle)
        {
            service = this->services[i];
            if (0 == this->attributes.size())
            {
                //there are no attributes to be sent back
                message_router_response->reply_service = (0x80 | message_router_request->service);
                message_router_response->general_status = kCipErrorServiceNotSupported;
                message_router_response->size_additional_status = 0;
            }
            else
            {
                for (j = 0; j < class_ptr->attributes.size(); j++) /* for each instance attribute of this class */
                {
                    attribute = attributes[j];
                    int attrNum = attribute->getNumber();

                    // only return attributes that are flagged as being part of GetAttributeALl
                    if (attrNum < 32 && (class_ptr->get_all_class_attributes_mask & 1 << attrNum))
                    {
                        message_router_request->request_path.attribute_number = attrNum;
                        if (kCipStatusOkSend != this->InstanceServices(kGetAttributeAll, message_router_request, message_router_response).status)
                        {
                            message_router_response->response_data = reply;

                            return CipStatus(kCipStatusError);
                        }
                        //message_router_response->data += message_router_response->data_length;
                    }
                }
                //message_router_response->data_length = message_router_response->data - reply;
                message_router_response->response_data = reply;
            }
            return CipStatus(kCipStatusOkSend);
        }
    }
    return CipStatus(kCipStatusOk); /* Return kCipStatusOk if cannot find GET_ATTRIBUTE_SINGLE service*/
}

#endif