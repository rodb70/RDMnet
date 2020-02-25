/******************************************************************************
 * Copyright 2019 ETC Inc.
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
 ******************************************************************************
 * This file is a part of RDMnet. For more information, go to:
 * https://github.com/ETCLabs/RDMnet
 *****************************************************************************/

#include "rdmnet/device.h"

#include "etcpal/common.h"
#include "rdmnet/private/core.h"
#include "rdmnet/private/device.h"
#include "rdmnet/private/opts.h"

#if RDMNET_DYNAMIC_MEM
#include <stdlib.h>
#else
#include "etcpal/mempool.h"
#endif

/***************************** Private macros ********************************/

/* Macros for dynamic vs static allocation. Static allocation is done using etcpal_mempool. */
#if RDMNET_DYNAMIC_MEM
#define ALLOC_RDMNET_DEVICE() malloc(sizeof(RdmnetDevice))
#define FREE_RDMNET_DEVICE(ptr) free(ptr)
#else
#define ALLOC_RDMNET_DEVICE() etcpal_mempool_alloc(rdmnet_devices)
#define FREE_RDMNET_DEVICE(ptr) etcpal_mempool_free(rdmnet_devices, ptr)
#endif

/**************************** Private variables ******************************/

#if !RDMNET_DYNAMIC_MEM
ETCPAL_MEMPOOL_DEFINE(rdmnet_devices, RdmnetDevice, RDMNET_MAX_DEVICES);
#endif

/*********************** Private function prototypes *************************/

static void client_connected(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                             const RdmnetClientConnectedInfo* info, void* context);
static void client_connect_failed(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                  const RdmnetClientConnectFailedInfo* info, void* context);
static void client_disconnected(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                const RdmnetClientDisconnectedInfo* info, void* context);
static void client_broker_msg_received(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                       const BrokerMessage* msg, void* context);
static void client_llrp_msg_received(rdmnet_client_t handle, const LlrpRemoteRdmCommand* cmd, void* context);
static void client_msg_received(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle, const RptClientMessage* msg,
                                void* context);

// clang-format off
static const RptClientCallbacks client_callbacks =
{
  client_connected,
  client_connect_failed,
  client_disconnected,
  client_broker_msg_received,
  client_llrp_msg_received,
  client_msg_received
};
// clang-format on

/*************************** Function definitions ****************************/

etcpal_error_t rdmnet_device_init(void)
{
#if RDMNET_DYNAMIC_MEM
  return kEtcPalErrOk;
#else
  return etcpal_mempool_init(rdmnet_devices);
#endif
}

void rdmnet_device_deinit(void)
{
}

/*!
 * \brief Initialize an RDMnet Device Config with default values for the optional config options.
 *
 * The config struct members not marked 'optional' are not meanizngfully initialized by this
 * function. Those members do not have default values and must be initialized manually before
 * passing the config struct to an API function.
 *
 * Usage example:
 * \code
 * RdmnetDeviceConfig config;
 * RDMNET_DEVICE_CONFIG_INIT(&config, 0x6574);
 * \endcode
 *
 * \param[out] config Pointer to RdmnetDeviceConfig to init.
 * \param[in] manufacturer_id ESTA manufacturer ID. All RDMnet Devices must have one.
 */
void rdmnet_device_config_init(RdmnetDeviceConfig* config, uint16_t manufacturer_id)
{
  if (config)
  {
    memset(config, 0, sizeof(RdmnetDeviceConfig));
    RDMNET_CLIENT_SET_DEFAULT_SCOPE(&config->scope_config);
    RDMNET_INIT_DYNAMIC_UID_REQUEST(&config->uid, manufacturer_id);
    config->search_domain = E133_DEFAULT_DOMAIN;
  }
}

/*!
 * \brief Create a new instance of RDMnet device functionality.
 *
 * Each device is identified by a single component ID (CID). Typical device applications will only
 * need one device instance. The library will attempt to discover and connect to a broker for the
 * scope given in config->scope_config (or just connect if a static broker is given); the status of
 * these attempts will be communicated via the callbacks associated with the device instance.
 *
 * \param[in] config Configuration parameters to use for this device instance.
 * \param[out] handle Filled in on success with a handle to the new device instance.
 * \return #kEtcPalErrOk: Device created successfully.
 * \return #kEtcPalErrInvalid: Invalid argument.
 * \return #kEtcPalErrNoMem: No memory to allocate new device instance.
 * \return Other errors forwarded from rdmnet_rpt_client_create().
 */
etcpal_error_t rdmnet_device_create(const RdmnetDeviceConfig* config, rdmnet_device_t* handle)
{
  if (!config || !handle)
    return kEtcPalErrInvalid;

  RdmnetDevice* new_device = ALLOC_RDMNET_DEVICE();
  if (!new_device)
    return kEtcPalErrNoMem;

  RdmnetRptClientConfig client_config;
  client_config.type = kRPTClientTypeDevice;
  client_config.cid = config->cid;
  client_config.callbacks = client_callbacks;
  client_config.callback_context = new_device;
  client_config.optional = config->optional;

  etcpal_error_t res = rdmnet_rpt_client_create(&client_config, &new_device->client_handle);
  if (res == kEtcPalErrOk)
  {
    res = rdmnet_client_add_scope(new_device->client_handle, &config->scope_config, &new_device->scope_handle);
    if (res == kEtcPalErrOk)
    {
      // Do the rest of the initialization
      new_device->callbacks = config->callbacks;
      new_device->callback_context = config->callback_context;

      *handle = new_device;
    }
    else
    {
      rdmnet_client_destroy(new_device->client_handle, kRdmnetDisconnectSoftwareFault);
      FREE_RDMNET_DEVICE(new_device);
    }
  }
  else
  {
    FREE_RDMNET_DEVICE(new_device);
  }
  return res;
}

/*!
 * \brief Destroy a device instance.
 *
 * Will disconnect from the broker to which this device is currently connected (if applicable),
 * sending the disconnect reason provided in the disconnect_reason parameter.
 *
 * \param[in] handle Handle to device to destroy, no longer valid after this function returns.
 * \param[in] disconnect_reason Disconnect reason code to send to the connected broker.
 * \return #kEtcPalErrOk: Device destroyed successfully.
 * \return #kEtcPalErrInvalid: Invalid argument.
 * \return Other errors forwarded from rdmnet_client_destroy().
 */
etcpal_error_t rdmnet_device_destroy(rdmnet_device_t handle, rdmnet_disconnect_reason_t disconnect_reason)
{
  if (!handle)
    return kEtcPalErrInvalid;

  etcpal_error_t res = rdmnet_client_destroy(handle->client_handle, disconnect_reason);
  if (res == kEtcPalErrOk)
    FREE_RDMNET_DEVICE(handle);

  return res;
}

/*!
 * \brief Send an RDM ACK response from a device.
 *
 * \param[in] handle Handle to the device from which to send the RDM ACK response.
 * \param[in] received_cmd Previously-received command that the ACK is a response to.
 * \param[in] response_data Parameter data that goes with this ACK, or NULL if no data.
 * \param[in] response_data_len Length in bytes of response_data, or 0 if no data.
 * \return #kEtcPalErrOk: ACK response sent successfully.
 * \return #kEtcPalErrInvalid: Invalid argument.
 * \return Other errors forwarded from rdmnet_rpt_client_send_rdm_ack().
 */
etcpal_error_t rdmnet_device_send_rdm_ack(rdmnet_device_t handle, const RdmnetRemoteRdmCommand* received_cmd,
                                          const uint8_t* response_data, size_t response_data_len)
{
  if (!handle)
    return kEtcPalErrInvalid;

  return rdmnet_rpt_client_send_rdm_ack(handle->client_handle, handle->scope_handle, received_cmd, response_data,
                                        response_data_len);
}

/*!
 * \brief Send an RDM NACK response from a device.
 *
 * \param[in] handle Handle to the device from which to send the RDM NACK response.
 * \param[in] received_cmd Previously-received command that the NACK is a response to.
 * \param[in] nack_reason RDM NACK reason code to send with the NACK.
 * \return #kEtcPalErrOk: NACK response sent successfully.
 * \return #kEtcPalErrInvalid: Invalid argument.
 * \return Other errors forwarded from rdmnet_rpt_client_send_rdm_nack().
 */
etcpal_error_t rdmnet_device_send_rdm_nack(rdmnet_device_t handle, const RdmnetRemoteRdmCommand* received_cmd,
                                           rdm_nack_reason_t nack_reason)
{
  if (!handle)
    return kEtcPalErrInvalid;

  return rdmnet_rpt_client_send_rdm_nack(handle->client_handle, handle->scope_handle, received_cmd, nack_reason);
}

/*!
 * \brief Send an RPT status message from a device.
 *
 * Status messages should only be sent in response to RDM commands received over RDMnet, if
 * something has gone wrong while attempting to resolve the command.
 *
 * \param[in] handle Handle to the device from which to send the RPT status.
 * \param[in] status Status message to send.
 * \return #kEtcPalErrOk: Status sent successfully.
 * \return #kEtcPalErrInvalid: Invalid argument.
 * \return Other errors forwarded from rdmnet_rpt_client_send_status().
 */
etcpal_error_t rdmnet_device_send_status(rdmnet_device_t handle, const RdmnetRemoteRdmCommand* received_cmd,
                                         rpt_status_code_t status_code, const char* status_string)
{
  if (!handle)
    return kEtcPalErrInvalid;

  return rdmnet_rpt_client_send_status(handle->client_handle, handle->scope_handle, received_cmd, status_code,
                                       status_string);
}

/*!
 * \brief Send an ACK response to an RDM command received over LLRP.
 *
 * \param[in] handle Handle to the device from which to send the LLRP RDM ACK response.
 * \param[in] received_cmd Previously-received command that the ACK is a response to.
 * \param[in] response_data Parameter data that goes with this ACK, or NULL if no data.
 * \param[in] response_data_len Length in bytes of response_data, or 0 if no data.
 * \return #kEtcPalErrOk: LLRP ACK response sent successfully.
 * \return #kEtcPalErrInvalid: Invalid argument.
 * \return Other errors forwarded from rdmnet_rpt_client_send_llrp_ack().
 */
etcpal_error_t rdmnet_device_send_llrp_ack(rdmnet_device_t handle, const LlrpRemoteRdmCommand* received_cmd,
                                           const uint8_t* response_data, uint8_t response_data_len)
{
  if (!handle)
    return kEtcPalErrInvalid;

  return rdmnet_rpt_client_send_llrp_ack(handle->client_handle, received_cmd, response_data, response_data_len);
}

/*!
 * \brief Send an ACK response to an RDM command received over LLRP.
 *
 * \param[in] handle Handle to the device from which to send the LLRP RDM NACK response.
 * \param[in] received_cmd Previously-received command that the ACK is a response to.
 * \param[in] nack_reason RDM NACK reason code to send with the NACK.
 * \return #kEtcPalErrOk: LLRP NACK response sent successfully.
 * \return #kEtcPalErrInvalid: Invalid argument.
 * \return Other errors forwarded from rdmnet_rpt_client_send_llrp_nack().
 */
etcpal_error_t rdmnet_device_send_llrp_nack(rdmnet_device_t handle, const LlrpLocalRdmCommand* received_cmd,
                                            rdm_nack_reason_t nack_reason)
{
  if (!handle)
    return kEtcPalErrInvalid;

  return rdmnet_rpt_client_send_llrp_nack(handle->client_handle, received_cmd, nack_reason);
}

etcpal_error_t rdmnet_device_request_dynamic_uids(rdmnet_device_t handle, const BrokerDynamicUidRequest* requests,
                                                  size_t num_requests)
{
  if (!handle)
    return kEtcPalErrInvalid;

  return rdmnet_client_request_dynamic_uids(handle->client_handle, handle->scope_handle, requests, num_requests);
}

etcpal_error_t rdmnet_device_change_scope(rdmnet_device_t handle, const RdmnetScopeConfig* new_scope_config,
                                          rdmnet_disconnect_reason_t disconnect_reason)
{
  if (!handle)
    return kEtcPalErrInvalid;

  return rdmnet_client_change_scope(handle->client_handle, handle->scope_handle, new_scope_config, disconnect_reason);
}

etcpal_error_t rdmnet_device_change_search_domain(rdmnet_device_t handle, const char* new_search_domain,
                                                  rdmnet_disconnect_reason_t disconnect_reason)
{
  if (!handle)
    return kEtcPalErrInvalid;

  return rdmnet_client_change_search_domain(handle->client_handle, new_search_domain, disconnect_reason);
}

void client_connected(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle, const RdmnetClientConnectedInfo* info,
                      void* context)
{
  ETCPAL_UNUSED_ARG(handle);

  RdmnetDevice* device = (RdmnetDevice*)context;
  if (device && scope_handle == device->scope_handle)
  {
    device->callbacks.connected(device, info, device->callback_context);
  }
}

void client_connect_failed(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                           const RdmnetClientConnectFailedInfo* info, void* context)
{
  ETCPAL_UNUSED_ARG(handle);

  RdmnetDevice* device = (RdmnetDevice*)context;
  if (device && scope_handle == device->scope_handle)
  {
    device->callbacks.connect_failed(device, info, device->callback_context);
  }
}

void client_disconnected(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                         const RdmnetClientDisconnectedInfo* info, void* context)
{
  ETCPAL_UNUSED_ARG(handle);

  RdmnetDevice* device = (RdmnetDevice*)context;
  if (device && scope_handle == device->scope_handle)
  {
    device->callbacks.disconnected(device, info, device->callback_context);
  }
}

void client_broker_msg_received(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle, const BrokerMessage* msg,
                                void* context)
{
  ETCPAL_UNUSED_ARG(handle);
  ETCPAL_UNUSED_ARG(scope_handle);
  ETCPAL_UNUSED_ARG(context);

  etcpal_log(rdmnet_log_params, ETCPAL_LOG_INFO, "Got Broker message with vector %d", msg->vector);
}

void client_llrp_msg_received(rdmnet_client_t handle, const LlrpRemoteRdmCommand* cmd, void* context)
{
  ETCPAL_UNUSED_ARG(handle);

  RdmnetDevice* device = (RdmnetDevice*)context;
  if (device)
  {
    device->callbacks.llrp_rdm_command_received(device, cmd, device->callback_context);
  }
}

void client_msg_received(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle, const RptClientMessage* msg,
                         void* context)
{
  ETCPAL_UNUSED_ARG(handle);
  ETCPAL_UNUSED_ARG(scope_handle);
  ETCPAL_UNUSED_ARG(msg);
  ETCPAL_UNUSED_ARG(context);

  // RdmnetDevice* device = (RdmnetDevice*)context;
  // if (device && scope_handle == device->scope_handle)
  //{
  //  if (msg->type == kRptClientMsgRdmCmd)
  //  {
  //    device->callbacks.rdm_command_received(device, &msg->payload.cmd, device->callback_context);
  //  }
  //  else
  //  {
  //    etcpal_log(rdmnet_log_params, ETCPAL_LOG_INFO, "Device incorrectly got non-RDM-command message.");
  //  }
  //}
}
