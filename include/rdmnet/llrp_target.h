/******************************************************************************
 * Copyright 2020 ETC Inc.
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

/**
 * @file rdmnet/llrp_target.h
 * @brief Functions for implementing LLRP Target functionality.
 */

#ifndef RDMNET_LLRP_TARGET_H_
#define RDMNET_LLRP_TARGET_H_

#include <stdint.h>
#include "etcpal/common.h"
#include "etcpal/uuid.h"
#include "etcpal/error.h"
#include "etcpal/inet.h"
#include "rdm/uid.h"
#include "rdm/message.h"
#include "rdmnet/common.h"
#include "rdmnet/llrp.h"
#include "rdmnet/message.h"

/**
 * @defgroup llrp_target LLRP Target API
 * @ingroup rdmnet_api
 * @brief Implement the functionality required by an LLRP Target in E1.33.
 *
 * Typically, this API is called automatically when using the role APIs:
 *
 * * @ref rdmnet_controller
 * * @ref rdmnet_device
 * * @ref rdmnet_broker
 *
 * And thus these functions should not typically need to be used directly.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** A handle for an instance of LLRP Target functionality. */
typedef int llrp_target_t;
/** An invalid LLRP target handle value. */
#define LLRP_TARGET_INVALID -1

/**
 * @brief An RDM command has been received addressed to an LLRP target.
 * @param[in] handle Handle to the LLRP target which has received the RDM command.
 * @param[in] cmd The RDM command data.
 * @param[out] response Fill in with response data if responding synchronously (see @ref handling_rdm_commands).
 * @param[in] context Context pointer that was given at the creation of the LLRP target instance.
 */
typedef void (*LlrpTargetRdmCommandReceivedCallback)(llrp_target_t          handle,
                                                     const LlrpRdmCommand*  cmd,
                                                     RdmnetSyncRdmResponse* response,
                                                     void*                  context);

/** A set of notification callbacks received about an LLRP target. */
typedef struct LlrpTargetCallbacks
{
  LlrpTargetRdmCommandReceivedCallback rdm_command_received; /**< Required. */
  void* context; /**< (optional) Pointer to opaque data passed back with each callback. */
} LlrpTargetCallbacks;

/** A set of information that defines the startup parameters of an LLRP Target. */
typedef struct LlrpTargetConfig
{
  /************************************************************************************************
   * Required Values
   ***********************************************************************************************/

  /** The target's CID. */
  EtcPalUuid cid;
  /** A set of callbacks for the target to receive RDMnet notifications. */
  LlrpTargetCallbacks callbacks;

  /************************************************************************************************
   * Optional Values
   ***********************************************************************************************/

  /**
   * (optional) A data buffer to be used to respond synchronously to RDM commands. See
   * @ref handling_rdm_commands for more information.
   */
  uint8_t* response_buf;

  /**
   * (optional) The target's UID. Will be dynamically generated by default. If you want a static
   * UID instead, just fill this in with the static UID after initializing.
   */
  RdmUid uid;

  /**
   * (optional) A set of network interfaces on which to operate this LLRP target. If NULL, the set
   * passed to rdmnet_init() will be used, or all network interfaces on the system if that was not
   * provided.
   */
  const RdmnetMcastNetintId* netints;
  /** (optional) The size of the netints array. */
  size_t num_netints;
} LlrpTargetConfig;

/**
 * @brief A default-value initializer for an LlrpTargetConfig struct.
 *
 * Usage:
 * @code
 * LlrpTargetConfig config = LLRP_TARGET_CONFIG_DEFAULT_INIT(MY_ESTA_MANUFACTURER_ID);
 * // Now fill in the required portions as necessary with your data...
 * @endcode
 *
 * @param manu_id Your ESTA manufacturer ID.
 */
#define LLRP_TARGET_CONFIG_DEFAULT_INIT(manu_id)                \
  {                                                             \
    {{0}}, {NULL, NULL}, NULL, {(0x8000 | manu_id), 0}, NULL, 0 \
  }

void llrp_target_config_init(LlrpTargetConfig* config, uint16_t manufacturer_id);

etcpal_error_t llrp_target_create(const LlrpTargetConfig* config, llrp_target_t* handle);
etcpal_error_t llrp_target_destroy(llrp_target_t handle);

etcpal_error_t llrp_target_send_ack(llrp_target_t              handle,
                                    const LlrpSavedRdmCommand* received_cmd,
                                    const uint8_t*             response_data,
                                    uint8_t                    response_data_len);
etcpal_error_t llrp_target_send_nack(llrp_target_t              handle,
                                     const LlrpSavedRdmCommand* received_cmd,
                                     rdm_nack_reason_t          nack_reason);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* RDMNET_LLRP_TARGET_H_ */