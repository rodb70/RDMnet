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

/*!
 * \file rdmnet/core/client.h
 * \brief An API for RDMnet Client functionality.
 */

#ifndef RDMNET_CORE_CLIENT_H_
#define RDMNET_CORE_CLIENT_H_

#include "etcpal/uuid.h"
#include "etcpal/inet.h"
#include "rdm/uid.h"
#include "rdm/message.h"
#include "rdmnet/defs.h"
#include "rdmnet/core.h"
#include "rdmnet/core/message.h"
#include "rdmnet/core/connection.h"
#include "rdmnet/core/llrp_target.h"
#include "rdmnet/core/util.h"

/*!
 * \defgroup rdmnet_client Client API
 * \ingroup rdmnet_core_lib
 * \brief Implementation of RDMnet client functionality
 *
 * RDMnet clients encompass controllers (which originate RDM commands on the network and receive
 * responses) and devices (which receive RDM commands and respond to them), as well as EPT clients,
 * which use the Extensible Packet Transport feature of RDMnet to transport opaque data through a
 * broker.
 *
 * Clients and the scopes they participate in are tracked by handles. Management of connections to
 * brokers, as well as LLRP functionality, is handled under the hood.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/*! A handle to an RDMnet client. */
typedef int rdmnet_client_t;
/*! An invalid RDMnet client handle value. */
#define RDMNET_CLIENT_INVALID -1

/*! A handle to a scope that an RDMnet client participates in. */
typedef rdmnet_conn_t rdmnet_client_scope_t;
/*! An invalid RDMnet client scope handle value. */
#define RDMNET_CLIENT_SCOPE_INVALID RDMNET_CONN_INVALID

/*! Information provided by the library about a successful RDMnet client connection. */
typedef struct RdmnetClientConnectedInfo
{
  /*! The IP address and port of the remote broker to which we have connected. */
  EtcPalSockAddr broker_addr;
} RdmnetClientConnectedInfo;

/*! Information provided by the library about an unsuccessful RDMnet client connection. */
typedef struct RdmnetClientConnectFailedInfo
{
  /*! The high-level reason that this connection failed. */
  rdmnet_connect_fail_event_t event;
  /*! The system error code associated with the failure; valid if event is
   *  kRdmnetConnectFailSocketFailure or kRdmnetConnectFailTcpLevel. */
  etcpal_error_t socket_err;
  /*! The reason given in the RDMnet-level connection refuse message. Valid if event is
   *  kRdmnetConnectFailRejected. */
  rdmnet_connect_status_t rdmnet_reason;
  /*!
   * \brief Whether the connection will be retried automatically.
   *
   * If this is true, the connection will be retried on the relevant scope; expect further
   * notifications of connection success or failure. If false, the rdmnet_client_scope_t handle
   * associated with the scope is invalidated, and the scope must be created again. This indicates
   * that the connection failed for a reason that usually must be corrected by a user or
   * application developer. Some possible reasons for this to be false include:
   * - The wrong scope was specified for a statically-configured broker
   * - A static UID was given that was invalid or duplicate with another UID in the system
   */
  bool will_retry;
} RdmnetClientConnectFailedInfo;

/*! Information provided by the library about an RDMnet client connection that disconnected after a
 *  successful connection. */
typedef struct RdmnetClientDisconnectedInfo
{
  /*! The high-level reason for the disconnect. */
  rdmnet_disconnect_event_t event;
  /*! The system error code associated with the disconnect; valid if event is
   *  kRdmnetDisconnectAbruptClose. */
  etcpal_error_t socket_err;
  /*! The reason given in the RDMnet-level disconnect message. Valid if event is
   *  kRdmnetDisconnectGracefulRemoteInitiated. */
  rdmnet_disconnect_reason_t rdmnet_reason;
  /*!
   * \brief Whether the connection will be retried automatically.
   *
   * There are currently no conditions that will cause this to be false; therefore, disconnection
   * events after a successful connection will always lead to the connection being retried
   * automatically. This field exists for potential future usage.
   */
  bool will_retry;
} RdmnetClientDisconnectedInfo;

/*!
 * Enumeration representing an action to take after an "RDM command received" callback completes.
 */
typedef enum
{
  /*! Send an RDM ACK to the originating controller. */
  kRdmnetResponseActionSendAck,
  /*! Send an RDM NACK with reason to the originating controller. */
  kRdmnetResponseActionSendNack,
  /*! Send an RPT status message with code to the originating controller. */
  kRdmnetResponseActionSendStatus,
  /*! Defer the response to be sent later. Be sure to save the command. */
  kRdmnetResponseActionDefer
} rdmnet_response_action_t;

/*!
 * \name Client Callback Functions
 * \brief Function types used as callbacks for RPT and EPT clients.
 * @{
 */

/*!
 * \brief An RDMnet client has connected successfully to a broker on a scope.
 *
 * Messages may now be sent using the relevant API functions, and messages may be received via
 * the msg_received callback.
 *
 * \param[in] handle Handle to client which has connected.
 * \param[in] scope_handle Handle to scope on which the client has connected.
 * \param[in] info More information about the successful connection.
 * \param[in] context Context pointer that was given at the creation of the client.
 */
typedef void (*RdmnetClientConnectedCb)(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                        const RdmnetClientConnectedInfo* info, void* context);

/*!
 * \brief An RDMnet client experienced a failure while attempting to connect to a broker on a
 *        scope.
 *
 * Connection failures can be fatal or non-fatal; the will_retry member of the info struct
 * indicates whether the connection will be retried automatically. If will_retry is false, it
 * usually indicates a misconfiguration that needs to be resolved by an application user.
 *
 * \param[in] handle Handle to client on which connection has failed.
 * \param[in] scope_handle Handle to scope on which connection has failed.
 * \param[in] info More information about the failed connection.
 * \param[in] context Context pointer that was given at the creation of the client.
 */
typedef void (*RdmnetClientConnectFailedCb)(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                            const RdmnetClientConnectFailedInfo* info, void* context);

/*! \brief An RDMnet client disconnected from a broker on a scope.
 *
 *  Disconnection can be fatal or non-fatal; the will_retry member of the info struct indicates
 *  whether the connection will be retried automatically. If will_retry is false, it usually
 *  indicates a misconfiguration that needs to be resolved by an application user.
 *
 *  \param[in] handle Handle to client which has disconnected.
 *  \param[in] scope_handle Handle to scope on which the disconnect occurred.
 *  \param[in] info More information about the disconnect.
 *  \param[in] context Context pointer that was given at the creation of the client.
 */
typedef void (*RdmnetClientDisconnectedCb)(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                           const RdmnetClientDisconnectedInfo* info, void* context);

/*!
 * \brief A broker message has been received on an RDMnet client connection.
 *
 * Broker messages are exchanged between an RDMnet client and broker to setup and faciliate RDMnet
 * communication. If using the \ref rdmnet_device "Device" or \ref rdmnet_controller "Controller"
 * API, this callback will be consumed internally and propagated to callbacks specific to those
 * client types.
 *
 * \param[in] handle Handle to client which has received a broker message.
 * \param[in] scope_handle Handle to scope on which the broker message was received.
 * \param[in] msg The broker message.
 * \param[in] context Context pointer that was given at the creation of the client.
 */
typedef void (*RdmnetClientBrokerMsgReceivedCb)(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                                const BrokerMessage* msg, void* context);

/*!
 * \brief An LLRP message has been received by an RDMnet client.
 *
 * RPT clients (controllers and devices) automatically listen for LLRP messages as required by
 * E1.33. This callback is called when an LLRP RDM command is received.
 *
 * \param[in] handle Handle to client which has received an LLRP message.
 * \param[in] cmd The LLRP RDM command.
 * \param[in] response A response object to be used for responding synchronously.
 * \param[in] context Context pointer that was given at the creation of the client.
 */
typedef llrp_response_action_t (*RdmnetClientLlrpMsgReceivedCb)(rdmnet_client_t handle, const LlrpRemoteRdmCommand* cmd,
                                                                LlrpSyncRdmResponse* response, void* context);

/*!
 * \brief An RPT message was received on an RPT client connection.
 *
 * RPT messages include Request and Notification, which wrap RDM commands and responses, as well as
 * Status, which informs of exceptional conditions in response to a Request. If using the
 * \ref rdmnet_device "Device" or \ref rdmnet_controller "Controller" API, this callback will be
 * consumed internally and propagated to callbacks specific to those client types.
 *
 * \param[in] handle Handle to client which has received an RPT message.
 * \param[in] scope_handle Handle to scope on which the RPT message was received.
 * \param[in] msg The RPT message.
 * \param[in] context Context pointer that was given at the creation of the client.
 */
typedef rdmnet_response_action_t (*RptClientMsgReceivedCb)(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                                           const RptClientMessage* msg, RdmnetSyncRdmResponse* response,
                                                           void* context);

/*!
 * \brief An EPT message was received on an EPT client connection.
 *
 * EPT messages include Data, which wraps opaque data, and Status, which informs of exceptional
 * conditions in response to Data.
 *
 * \param[in] handle Handle to client which has received an EPT message.
 * \param[in] scope_handle Handle to scope on which the EPT message was received.
 * \param[in] msg The EPT message.
 * \param[in] context Context pointer that was given at the creation of the client.
 */
typedef rdmnet_response_action_t (*EptClientMsgReceivedCb)(rdmnet_client_t handle, rdmnet_client_scope_t scope,
                                                           const EptClientMessage* msg, EptClientMessage* response,
                                                           void* context);

/*!
 * @}
 */

/*! The set of possible callbacks that are delivered to an RPT client. */
typedef struct RptClientCallbacks
{
  RdmnetClientConnectedCb connected;
  RdmnetClientConnectFailedCb connect_failed;
  RdmnetClientDisconnectedCb disconnected;
  RdmnetClientBrokerMsgReceivedCb broker_msg_received;
  RdmnetClientLlrpMsgReceivedCb llrp_msg_received;
  RptClientMsgReceivedCb msg_received;
} RptClientCallbacks;

/*! The set of possible callbacks that are delivered to an EPT client. */
typedef struct EptClientCallbacks
{
  RdmnetClientConnectedCb connected;
  RdmnetClientConnectFailedCb connect_failed;
  RdmnetClientDisconnectedCb disconnected;
  RdmnetClientBrokerMsgReceivedCb broker_msg_received;
  EptClientMsgReceivedCb msg_received;
} EptClientCallbacks;

/*!
 * A set of configuration information for a single scope in which an RDMnet client is
 * participating.
 */
typedef struct RdmnetScopeConfig
{
  /*!
   * The scope string. Scope strings are UTF-8, and their maximum length including NULL terminator
   * is #E133_SCOPE_STRING_PADDED_LENGTH, which is derived from the requirements of DNS and DNS-SD.
   */
  const char* scope;
  /*!
   * Whether a static broker address has been configured on this scope. If this is true, discovery
   * using DNS-SD will be bypassed and a connection will be attempted directly to the address
   * indicated in static_broker_addr.
   */
  bool has_static_broker_addr;
  /*! The broker address to which to connect, if a static broker has been configured. */
  EtcPalSockAddr static_broker_addr;
} RdmnetScopeConfig;

/*! A set of information that defines the startup parameters of an RPT RDMnet Client. */
typedef struct RdmnetRptClientConfig
{
  /****** Required Values ******/

  /*! The client type, either controller or device. */
  rpt_client_type_t type;
  /*! The client's CID. */
  EtcPalUuid cid;
  /*! A set of callbacks for the client to receive RDMnet notifications. */
  RptClientCallbacks callbacks;

  /****** Optional Values ******/

  /*! (optional) Pointer to opaque data passed back with each callback. */
  void* callback_context;
  /*!
   * (optional) The client's UID. This will be initialized with a Dynamic UID request using the
   * initialization functions/macros for this structure. If you want to use a static UID instead,
   * just fill this in with the static UID after initializing.
   */
  RdmUid uid;
  /*! (optional) The client's configured search domain for discovery. */
  const char* search_domain;
  /*!
   * (optional) A set of network interfaces to use for the LLRP target associated with this client.
   * If NULL, the set passed to rdmnet_core_init() will be used, or all network interfaces on the
   * system if that was not provided.
   */
  RdmnetMcastNetintId* llrp_netint_arr;
  /*! (optional) The size of llrp_netint_arr. */
  size_t num_llrp_netints;
} RdmnetRptClientConfig;

/*!
 * \brief A default-value initializer for an RdmnetRptClientConfig struct.
 *
 * Usage:
 * \code
 * RdmnetRptClientConfig config = RDMNET_RPT_CLIENT_CONFIG_DEFAULT_INIT(MY_ESTA_MANUFACTURER_ID);
 * // Now fill in the required portions as necessary with your data...
 * \endcode
 *
 * \param manu_id Your ESTA manufacturer ID.
 */
#define RDMNET_RPT_CLIENT_CONFIG_DEFAULT_INIT(manu_id)                                                           \
  {                                                                                                              \
    kRPTClientTypeUnknown, kEtcPalNullUuid, {NULL, NULL, NULL, NULL, NULL, NULL}, NULL, {0x8000u | manu_val, 0}, \
        E133_DEFAULT_DOMAIN, NULL, 0                                                                             \
  }

/*! A set of information that defines the startup parameters of an EPT RDMnet Client. */
typedef struct RdmnetEptClientConfig
{
  /*! An array of EPT sub-protocols that this EPT client uses. */
  EptSubProtocol* protocols;
  /*! The size of the protocols array. */
  size_t num_protocols;
  /*! A set of callbacks for the client to receive RDMnet notifications. */
  EptClientCallbacks callbacks;
  /*! (optional) Pointer to opaque data passed back with each callback. */
  void* callback_context;
} RdmnetEptClientConfig;

/*!
 * \brief A default-value initializer for an RdmnetEptClientConfig struct.
 *
 * Usage:
 * \code
 * RdmnetEptClientConfig config = RDMNET_EPT_CLIENT_CONFIG_DEFAULT_INIT;
 * // Now fill in the required portions as necessary with your data...
 * \endcode
 */
#define RDMNET_EPT_CLIENT_CONFIG_DEFAULT_INIT     \
  {                                               \
    NULL, 0, {NULL, NULL, NULL, NULL, NULL}, NULL \
  }

/*!
 * \brief Initialize an RdmnetScopeConfig struct with a scope string.
 *
 * Scopes are resolved using RDMnet discovery (DNS-SD) by default; to override this behavior with a
 * static broker address and port, use rdmnet_set_static_scope().
 *
 * \param configptr Pointer to RdmnetScopeConfig.
 * \param scope_str UTF-8 scope string (const char *), must remain valid for as long as this scope
 *                  config.
 */
#define RDMNET_CLIENT_SET_SCOPE(configptr, scope_str) \
  do                                                  \
  {                                                   \
    (configptr)->scope = scope_str;                   \
    (configptr)->has_static_broker_addr = false;      \
  } while (0)

/*!
 * \brief Initialize an RdmnetScopeConfig struct with the default RDMnet scope.
 *
 * Scopes are resolved using RDMnet discovery (DNS-SD) by default; to override this behavior with a
 * static broker address and port, use rdmnet_set_static_default_scope().
 *
 * \param configptr Pointer to RdmnetScopeConfig.
 */
#define RDMNET_CLIENT_SET_DEFAULT_SCOPE(configptr) \
  do                                               \
  {                                                \
    (configptr)->scope = E133_DEFAULT_SCOPE;       \
    (configptr)->has_static_broker_addr = false;   \
  } while (0)

/*!
 * \brief Initialize an RdmnetScopeConfig struct with a scope string and static broker address.
 *
 * DNS-SD discovery will be bypassed and broker connection will be attempted using the address and
 * port given.
 *
 * \param configptr Pointer to RdmnetScopeConfig.
 * \param scope_str UTF-8 scope string (const char *), must remain valid for as long as this scope
 *                  config.
 * \param broker_addr Address and port for a static broker (EtcPalSockAddr).
 */
#define RDMNET_CLIENT_SET_STATIC_SCOPE(configptr, scope_str, broker_addr) \
  do                                                                      \
  {                                                                       \
    (configptr)->scope = (scope_str);                                     \
    (configptr)->has_static_broker_addr = true;                           \
    (configptr)->static_broker_addr = (broker_addr);                      \
  } while (0)

/*!
 * \brief Initialize an RdmnetScopeConfig struct with the default RDMnet scope and a static broker
 *        address.
 *
 * DNS-SD discovery will be bypassed and broker connection will be attempted using the address and
 * port given.
 *
 * \param configptr Pointer to RdmnetScopeConfig.
 * \param broker_addr Address and port for a static broker (EtcPalSockAddr)
 */
#define RDMNET_CLIENT_SET_STATIC_DEFAULT_SCOPE(configptr, broker_addr) \
  do                                                                   \
  {                                                                    \
    (configptr)->scope = E133_DEFAULT_SCOPE;                           \
    (configptr)->has_static_broker_addr = true;                        \
    (configptr)->static_broker_addr = (broker_addr);                   \
  } while (0)

void rdmnet_rpt_client_config_init(RdmnetRptClientConfig* config, uint16_t manufacturer_id);
void rdmnet_ept_client_config_init(RdmnetEptClientConfig* config);

etcpal_error_t rdmnet_rpt_client_create(const RdmnetRptClientConfig* config, rdmnet_client_t* handle);
etcpal_error_t rdmnet_ept_client_create(const RdmnetEptClientConfig* config, rdmnet_client_t* handle);
etcpal_error_t rdmnet_client_destroy(rdmnet_client_t handle, rdmnet_disconnect_reason_t disconnect_reason);

etcpal_error_t rdmnet_client_add_scope(rdmnet_client_t handle, const RdmnetScopeConfig* scope_config,
                                       rdmnet_client_scope_t* scope_handle);
etcpal_error_t rdmnet_client_remove_scope(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                          rdmnet_disconnect_reason_t reason);
etcpal_error_t rdmnet_client_get_scope_string(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                              char* scope_str_buf);
etcpal_error_t rdmnet_client_get_static_broker_config(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                                      bool* has_static_broker_addr, EtcPalSockAddr* static_broker_addr);

etcpal_error_t rdmnet_client_change_scope(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                          const RdmnetScopeConfig* new_scope_config,
                                          rdmnet_disconnect_reason_t disconnect_reason);
etcpal_error_t rdmnet_client_change_search_domain(rdmnet_client_t handle, const char* new_search_domain,
                                                  rdmnet_disconnect_reason_t reason);

etcpal_error_t rdmnet_client_request_client_list(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle);
etcpal_error_t rdmnet_client_request_dynamic_uids(rdmnet_conn_t handle, rdmnet_client_scope_t scope_handle,
                                                  const BrokerDynamicUidRequest* requests, size_t num_requests);
etcpal_error_t rdmnet_client_request_dynamic_uid_mappings(rdmnet_conn_t handle, rdmnet_client_scope_t scope_handle,
                                                          const RdmUid* uids, size_t num_uids);

etcpal_error_t rdmnet_rpt_client_send_rdm_command(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                                  const RdmnetLocalRdmCommand* cmd, uint32_t* seq_num);

etcpal_error_t rdmnet_rpt_client_send_rdm_ack(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                              const RdmnetRemoteRdmCommand* received_cmd, const uint8_t* response_data,
                                              size_t response_data_len);
etcpal_error_t rdmnet_rpt_client_send_rdm_nack(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                               const RdmnetRemoteRdmCommand* received_cmd,
                                               rdm_nack_reason_t nack_reason);
etcpal_error_t rdmnet_rpt_client_send_unsolicited_response(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                                           const RdmnetUnsolicitedRdmResponse* response);

etcpal_error_t rdmnet_rpt_client_send_status(rdmnet_client_t handle, rdmnet_client_scope_t scope_handle,
                                             const RdmnetRemoteRdmCommand* received_cmd, rpt_status_code_t status_code,
                                             const char* status_string);

etcpal_error_t rdmnet_rpt_client_send_llrp_ack(rdmnet_client_t handle, const LlrpRemoteRdmCommand* received_cmd,
                                               const uint8_t* response_data, uint8_t response_data_len);
etcpal_error_t rdmnet_rpt_client_send_llrp_nack(rdmnet_client_t handle, const LlrpLocalRdmCommand* received_cmd,
                                                rdm_nack_reason_t nack_reason);

#ifdef __cplusplus
}
#endif

/*!
 * @}
 */

#endif /* RDMNET_CORE_CLIENT_H_ */
