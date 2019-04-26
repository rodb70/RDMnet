/******************************************************************************
************************* IMPORTANT NOTE -- READ ME!!! ************************
*******************************************************************************
* THIS SOFTWARE IMPLEMENTS A **DRAFT** STANDARD, BSR E1.33 REV. 63. UNDER NO
* CIRCUMSTANCES SHOULD THIS SOFTWARE BE USED FOR ANY PRODUCT AVAILABLE FOR
* GENERAL SALE TO THE PUBLIC. DUE TO THE INEVITABLE CHANGE OF DRAFT PROTOCOL
* VALUES AND BEHAVIORAL REQUIREMENTS, PRODUCTS USING THIS SOFTWARE WILL **NOT**
* BE INTEROPERABLE WITH PRODUCTS IMPLEMENTING THE FINAL RATIFIED STANDARD.
*******************************************************************************
* Copyright 2018 ETC Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************
* This file is a part of RDMnet. For more information, go to:
* https://github.com/ETCLabs/RDMnet
******************************************************************************/
#ifndef _RDMNET_PRIVATE_LLRP_H_
#define _RDMNET_PRIVATE_LLRP_H_

#include "lwpa/bool.h"
#include "lwpa/error.h"
#include "lwpa/inet.h"
#include "lwpa/socket.h"

#define LLRP_MULTICAST_TTL_VAL 20

#ifdef __cplusplus
extern "C" {
#endif

extern const LwpaSockaddr *kLlrpIpv4RespAddr;
extern const LwpaSockaddr *kLlrpIpv6RespAddr;
extern const LwpaSockaddr *kLlrpIpv4RequestAddr;
extern const LwpaSockaddr *kLlrpIpv6RequestAddr;

lwpa_error_t rdmnet_llrp_init();
void rdmnet_llrp_deinit();

void rdmnet_llrp_tick();

lwpa_error_t create_llrp_socket(const LwpaIpAddr *netint, bool manager, lwpa_socket_t *socket);

#ifdef __cplusplus
}
#endif

#endif /* _RDMNET_PRIVATE_LLRP_H_ */