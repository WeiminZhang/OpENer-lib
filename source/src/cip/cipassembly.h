/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#pragma once

#include "ciptypes.h"
#include "typedefs.h"
#include "src/cip/class_stack/CIP_Class.h"

class CIPAssembly : public CIP_Class
{
	public:
		/** @brief Setup the Assembly object
		 * 
		 * Creates the Assembly Class with zero instances and sets up all services.
		 *
		 * @return Returns kCipStatusOk if assembly object was successfully created, otherwise kCipStatusError
		 */
		static CipStatus CipAssemblyInitialize(void);

		/** @brief clean up the data allocated in the assembly object instances
		 *
		 * Assembly object instances allocate per instance data to store attribute 3.
		 * This will be freed here. The assembly object data given by the application
		 * is not freed neither the assembly object instances. These are handled in the
		 * main shutdown function.
		 */
		static void ShutdownAssemblies(void);

		/** @brief notify an Assembly object that data has been received for it.
		 * 
		 *  The data will be copied into the assembly objects attribute 3 and
		 *  the application will be informed with the IApp_after_assembly_data_received function.
		 *  
		 *  @param instance the assembly object instance for which the data was received
		 *  @param data pointer to the data received
		 *  @param data_length number of bytes received
		 *  @return 
		 *     - EIP_OK the received data was okay
		 *     - EIP_ERROR the received data was wrong
		 */
		CipStatus NotifyAssemblyConnectedDataReceived(CipUsint* data, CipUint data_length);

    /** @brief Implementation of the SetAttributeSingle CIP service for Assembly
		 *          Objects.
		 *  Currently only supports Attribute 3 (CIP_BYTE_ARRAY) of an Assembly
		 */
    CipStatus SetAssemblyAttributeSingle( CIP_Class* instance,
                  CipMessageRouterRequest* message_router_request, CipMessageRouterResponse* message_router_response);



		CIP_Class* CreateAssemblyClass(void);
	    CIP_Class* CreateAssemblyInstance(CipUdint instance_id);

		CIP_Class* CreateAssemblyObject(CipUdint instance_id, EipByte* data, CipUint data_length);
	private:
		
};

