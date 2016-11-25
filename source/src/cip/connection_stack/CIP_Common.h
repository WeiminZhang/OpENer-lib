/*******************************************************************************
 * Copyright (c) 2009, Rockwell Automation, Inc.
 * All rights reserved. 
 *
 ******************************************************************************/
#pragma once

/** @file cipcommon.h
 * Common CIP object interface
 */
#include "ciptypes.h"
#include "typedefs.h"
#include "src/cip/class_stack/CIP_ClassInstance.h"
//#include "opener_user_conf.h"
#include <array>




class CIP_Common
{

	public:
		/** A buffer for holding the replay generated by explicit message requests
		 *  or producing I/O connections. These will use this buffer in the following
		 *  ways:
		 *    1. Explicit messages will use this buffer to store the data generated by the request
		 *    2. I/O Connections will use this buffer for the produced data
		 */
		static std::map <CipUsint,CipByteArray> message_data_reply_buffer;

		/** @brief Check if requested service present in class/instance and call appropriate service.
		 *
		 * @param class class receiving the message
		 * @param message_router_request request message
		 * @param message_router_response reply message
		 * @return
		 *     - EIP_OK_SEND    ... success
		 *     - EIP_OK  ... no reply to send back
		 *     - EIP_ERROR ... error
		 */
		static CipStatus NotifyClass(CIP_ClassInstance * cipClass, CipMessageRouterRequest* message_router_request, CipMessageRouterResponse* message_router_response);


		/** @brief Decodes padded EPath
		 *  @param epath EPath to the receiving element
		 *  @param message CIP Message to decode
		 *  @return Number of decoded bytes
		 */
		static int DecodePaddedEPath(CipEpath* epath, CipUsint** data);

		static void CipStackInit(CipUint unique_connection_id);
		static void ShutdownCipStack(void);
	private:
		static const CipUint kCipUintZero = 0;
		static int EncodeEPath(CipEpath* epath, CipUsint** message);
		static int EncodeData(CipUsint cip_type, void* data, CipUsint** message);
		static int DecodeData(CipUsint cip_type, void* data, CipUsint** message);

};