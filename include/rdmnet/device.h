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

/*! \file rdmnet/device.h
 *  \brief Definitions for the RDMnet Device API
 *  \author Sam Kearney
 */
#ifndef _RDMNET_DEVICE_H_
#define _RDMNET_DEVICE_H_

#include "lwpa/bool.h"
#include "lwpa/uuid.h"
#include "rdm/uid.h"
#include "rdmnet/client.h"

typedef struct RdmnetDevice *rdmnet_device_t;

typedef struct RdmnetDeviceCallbacks
{
  void (*connected)(rdmnet_device_t handle, const RdmnetClientConnectedInfo *info, void *context);
  void (*connect_failed)(rdmnet_device_t handle, const RdmnetClientConnectFailedInfo *info, void *context);
  void (*disconnected)(rdmnet_device_t handle, const RdmnetClientDisconnectedInfo *info, void *context);
  void (*rdm_command_received)(rdmnet_device_t handle, const RemoteRdmCommand *cmd, void *context);
  void (*llrp_rdm_command_received)(rdmnet_device_t handle, const LlrpRemoteRdmCommand *cmd, void *context);
} RdmnetDeviceCallbacks;

/*! A set of information that defines the startup parmaeters of an RDMnet Device. */
typedef struct RdmnetDeviceConfig
{
  /*! The device's CID. */
  LwpaUuid cid;
  /*! The device's configured RDMnet scope. */
  RdmnetScopeConfig scope_config;
  /*! A set of callbacks for the device to receive RDMnet notifications. */
  RdmnetDeviceCallbacks callbacks;
  /*! Pointer to opaque data passed back with each callback. Can be NULL. */
  void *callback_context;
  /*! Optional configuration data for the device's RPT Client functionality. */
  RptClientOptionalConfig optional;
  /*! Optional configuration data for the device's LLRP Target functionality. */
  LlrpTargetOptionalConfig llrp_optional;
} RdmnetDeviceConfig;

#define RDMNET_DEVICE_CONFIG_INIT(devicecfgptr, manu_id) RPT_CLIENT_CONFIG_INIT(devicecfgptr, manu_id)

lwpa_error_t rdmnet_device_init(const LwpaLogParams *lparams);
void rdmnet_device_deinit();

lwpa_error_t rdmnet_device_create(const RdmnetDeviceConfig *config, rdmnet_device_t *handle);
lwpa_error_t rdmnet_device_destroy(rdmnet_device_t handle);

lwpa_error_t rdmnet_device_send_rdm_response(rdmnet_device_t handle, const LocalRdmResponse *resp);
lwpa_error_t rdmnet_device_send_status(rdmnet_device_t handle, const LocalRptStatus *status);
lwpa_error_t rdmnet_device_send_llrp_response(rdmnet_device_t handle, const LlrpLocalRdmResponse *resp);

lwpa_error_t rdmnet_device_change_scope(rdmnet_device_t handle, const RdmnetScopeConfig *new_scope_config,
                                        rdmnet_disconnect_reason_t reason);
lwpa_error_t rdmnet_device_change_search_domain(rdmnet_device_t handle, const char *new_search_domain,
                                                rdmnet_disconnect_reason_t reason);

#endif /* _RDMNET_DEVICE_H_ */
