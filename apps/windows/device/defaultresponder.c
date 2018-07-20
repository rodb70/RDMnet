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

#include "defaultresponder.h"

#include <string.h>
#include <stdio.h>
#include "lwpa_pack.h"
#include "lwpa_thread.h"
#include "lwpa_lock.h"
#include "estardmnet.h"
#include "estardm.h"
#include "rdmnet/version.h"

/**************************** Private constants ******************************/

#define NUM_SUPPORTED_PIDS 14
static const uint16_t kSupportedPIDList[NUM_SUPPORTED_PIDS] = {
    E120_IDENTIFY_DEVICE,           E120_SUPPORTED_PARAMETERS,   E120_DEVICE_INFO,      E120_MANUFACTURER_LABEL,
    E120_DEVICE_MODEL_DESCRIPTION,  E120_SOFTWARE_VERSION_LABEL, E120_DEVICE_LABEL,     E133_COMPONENT_SCOPE,
    E133_BROKER_STATIC_CONFIG_IPV4, E133_SEARCH_DOMAIN,          E133_TCP_COMMS_STATUS, E133_DUPLICATE_UID_DETECTED,
    E137_7_ENDPOINT_LIST,           E137_7_ENDPOINT_RESPONDERS,
};

/* clang-format off */
static const uint8_t kDeviceInfo[] = {
    0x01, 0x00, /* RDM Protocol version */
    0xe1, 0x33, /* Device Model ID */
    0xe1, 0x33, /* Product Category */

    /* Software Version ID */
    RDMNET_VERSION_MAJOR, RDMNET_VERSION_MINOR,
    RDMNET_VERSION_PATCH, RDMNET_VERSION_BUILD,

    0x00, 0x00, /* DMX512 Footprint */
    0x00, 0x00, /* DMX512 Personality */
    0xff, 0xff, /* DMX512 Start Address */
    0x00, 0x00, /* Sub-device count */
    0x00 /* Sensor count */
};
/* clang-format on */

#define DEVICE_LABEL_MAX_LEN 32
#define DEFAULT_DEVICE_LABEL "My ETC RDMnet Device"
#define SOFTWARE_VERSION_LABEL RDMNET_VERSION_STRING
#define MANUFACTURER_LABEL "ETC"
#define DEVICE_MODEL_DESCRIPTION "Prototype RDMnet Device"

/**************************** Private variables ******************************/

static struct DefaultResponderPropertyData
{
  uint32_t endpoint_list_change_number;
  lwpa_thread_t identify_thread;
  bool identifying;
  char device_label[DEVICE_LABEL_MAX_LEN + 1];
  RdmnetConnectParams rdmnet_params;
  uint16_t tcp_unhealthy_counter;
  LwpaSockaddr cur_broker_addr;
} prop_data;

static lwpa_rwlock_t prop_lock;

/*********************** Private function prototypes *************************/

/* SET COMMANDS */
bool set_identify_device(const uint8_t *param_data, uint8_t param_data_len, uint16_t *nack_reason,
                         bool *requires_reconnect);
bool set_device_label(const uint8_t *param_data, uint8_t param_data_len, uint16_t *nack_reason,
                      bool *requires_reconnect);
bool set_component_scope(const uint8_t *param_data, uint8_t param_data_len, uint16_t *nack_reason,
                         bool *requires_reconnect);
bool set_broker_static_config_ipv4(const uint8_t *param_data, uint8_t param_data_len, uint16_t *nack_reason,
                                   bool *requires_reconnect);
bool set_search_domain(const uint8_t *param_data, uint8_t param_data_len, uint16_t *nack_reason,
                       bool *requires_reconnect);
bool set_tcp_comms_status(const uint8_t *param_data, uint8_t param_data_len, uint16_t *nack_reason,
                          bool *requires_reconnect);

/* GET COMMANDS */
bool get_identify_device(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                         size_t *num_responses, uint16_t *nack_reason);
bool get_device_label(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                      size_t *num_responses, uint16_t *nack_reason);
bool get_component_scope(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                         size_t *num_responses, uint16_t *nack_reason);
bool get_broker_static_config_ipv4(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                                   size_t *num_responses, uint16_t *nack_reason);
bool get_search_domain(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                       size_t *num_responses, uint16_t *nack_reason);
bool get_tcp_comms_status(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                          size_t *num_responses, uint16_t *nack_reason);
bool get_supported_parameters(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                              size_t *num_responses, uint16_t *nack_reason);
bool get_device_info(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                     size_t *num_responses, uint16_t *nack_reason);
bool get_manufacturer_label(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                            size_t *num_responses, uint16_t *nack_reason);
bool get_device_model_description(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                                  size_t *num_responses, uint16_t *nack_reason);
bool get_software_version_label(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                                size_t *num_responses, uint16_t *nack_reason);
bool get_endpoint_list(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                       size_t *num_responses, uint16_t *nack_reason);
bool get_endpoint_responders(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                             size_t *num_responses, uint16_t *nack_reason);

/*************************** Function definitions ****************************/

void default_responder_init(const LwpaSockaddr *static_broker_addr, const char *scope)
{
  lwpa_rwlock_create(&prop_lock);

  strncpy(prop_data.device_label, DEFAULT_DEVICE_LABEL, DEVICE_LABEL_MAX_LEN);
  strncpy(prop_data.rdmnet_params.scope, scope, E133_SCOPE_STRING_PADDED_LENGTH - 1);
  strncpy(prop_data.rdmnet_params.search_domain, E133_DEFAULT_DOMAIN, 64);
  prop_data.rdmnet_params.broker_static_addr = *static_broker_addr;
}

void default_responder_deinit()
{
  if (prop_data.identifying)
  {
    prop_data.identifying = false;
    lwpa_thread_stop(&prop_data.identify_thread, 5000);
  }
  lwpa_rwlock_destroy(&prop_lock);
}

void default_responder_get_e133_params(RdmnetConnectParams *params)
{
  if (lwpa_rwlock_readlock(&prop_lock, LWPA_WAIT_FOREVER))
  {
    *params = prop_data.rdmnet_params;
    lwpa_rwlock_readunlock(&prop_lock);
  }
}

void default_responder_incr_unhealthy_count()
{
  if (lwpa_rwlock_writelock(&prop_lock, LWPA_WAIT_FOREVER))
  {
    ++prop_data.tcp_unhealthy_counter;
    lwpa_rwlock_writeunlock(&prop_lock);
  }
}

void default_responder_set_tcp_status(LwpaSockaddr *broker_addr)
{
  if (lwpa_rwlock_writelock(&prop_lock, LWPA_WAIT_FOREVER))
  {
    prop_data.cur_broker_addr = *broker_addr;
    lwpa_rwlock_writeunlock(&prop_lock);
  }
}

bool default_responder_supports_pid(uint16_t pid)
{
  size_t i;
  for (i = 0; i < NUM_SUPPORTED_PIDS; ++i)
  {
    if (kSupportedPIDList[i] == pid)
      return true;
  }
  return false;
}

bool default_responder_set(uint16_t pid, const uint8_t *param_data, uint8_t param_data_len, uint16_t *nack_reason,
                           bool *requires_reconnect)
{
  bool res = false;
  if (lwpa_rwlock_writelock(&prop_lock, LWPA_WAIT_FOREVER))
  {
    switch (pid)
    {
      case E120_IDENTIFY_DEVICE:
        res = set_identify_device(param_data, param_data_len, nack_reason, requires_reconnect);
        break;
      case E120_DEVICE_LABEL:
        res = set_device_label(param_data, param_data_len, nack_reason, requires_reconnect);
        break;
      case E133_COMPONENT_SCOPE:
        res = set_component_scope(param_data, param_data_len, nack_reason, requires_reconnect);
        break;
      case E133_BROKER_STATIC_CONFIG_IPV4:
        res = set_broker_static_config_ipv4(param_data, param_data_len, nack_reason, requires_reconnect);
        break;
      case E133_SEARCH_DOMAIN:
        res = set_search_domain(param_data, param_data_len, nack_reason, requires_reconnect);
        break;
      case E133_TCP_COMMS_STATUS:
        res = set_tcp_comms_status(param_data, param_data_len, nack_reason, requires_reconnect);
        break;
      case E120_SUPPORTED_PARAMETERS:
      case E120_MANUFACTURER_LABEL:
      case E120_DEVICE_MODEL_DESCRIPTION:
      case E120_SOFTWARE_VERSION_LABEL:
      case E137_7_ENDPOINT_LIST:
      case E137_7_ENDPOINT_RESPONDERS:
        *nack_reason = E120_NR_UNSUPPORTED_COMMAND_CLASS;
        *requires_reconnect = false;
        break;
      default:
        *nack_reason = E120_NR_UNKNOWN_PID;
        *requires_reconnect = false;
        break;
    }
    lwpa_rwlock_writeunlock(&prop_lock);
  }
  return res;
}

bool default_responder_get(uint16_t pid, const uint8_t *param_data, uint8_t param_data_len,
                           param_data_list_t resp_data_list, size_t *num_responses, uint16_t *nack_reason)
{
  bool res = false;
  if (lwpa_rwlock_readlock(&prop_lock, LWPA_WAIT_FOREVER))
  {
    switch (pid)
    {
      case E120_IDENTIFY_DEVICE:
        res = get_identify_device(param_data, param_data_len, resp_data_list, num_responses, nack_reason);
        break;
      case E120_DEVICE_LABEL:
        res = get_device_label(param_data, param_data_len, resp_data_list, num_responses, nack_reason);
        break;
      case E133_COMPONENT_SCOPE:
        res = get_component_scope(param_data, param_data_len, resp_data_list, num_responses, nack_reason);
        break;
      case E133_BROKER_STATIC_CONFIG_IPV4:
        res = get_broker_static_config_ipv4(param_data, param_data_len, resp_data_list, num_responses, nack_reason);
        break;
      case E133_SEARCH_DOMAIN:
        res = get_search_domain(param_data, param_data_len, resp_data_list, num_responses, nack_reason);
        break;
      case E133_TCP_COMMS_STATUS:
        res = get_tcp_comms_status(param_data, param_data_len, resp_data_list, num_responses, nack_reason);
        break;
      case E120_SUPPORTED_PARAMETERS:
        res = get_supported_parameters(param_data, param_data_len, resp_data_list, num_responses, nack_reason);
        break;
      case E120_DEVICE_INFO:
        res = get_device_info(param_data, param_data_len, resp_data_list, num_responses, nack_reason);
        break;
      case E120_MANUFACTURER_LABEL:
        res = get_manufacturer_label(param_data, param_data_len, resp_data_list, num_responses, nack_reason);
        break;
      case E120_DEVICE_MODEL_DESCRIPTION:
        res = get_device_model_description(param_data, param_data_len, resp_data_list, num_responses, nack_reason);
        break;
      case E120_SOFTWARE_VERSION_LABEL:
        res = get_software_version_label(param_data, param_data_len, resp_data_list, num_responses, nack_reason);
        break;
      case E137_7_ENDPOINT_LIST:
        res = get_endpoint_list(param_data, param_data_len, resp_data_list, num_responses, nack_reason);
        break;
      case E137_7_ENDPOINT_RESPONDERS:
        res = get_endpoint_responders(param_data, param_data_len, resp_data_list, num_responses, nack_reason);
        break;
      default:
        *nack_reason = E120_NR_UNKNOWN_PID;
        break;
    }
    lwpa_rwlock_readunlock(&prop_lock);
  }
  return res;
}

void identify_thread(void *arg)
{
  (void)arg;

  while (prop_data.identifying)
  {
    Beep(440, 1000);
    lwpa_thread_sleep(1000);
  }
}

bool set_identify_device(const uint8_t *param_data, uint8_t param_data_len, uint16_t *nack_reason,
                         bool *requires_reconnect)
{
  if (param_data_len >= 1)
  {
    bool new_identify_setting = (bool)(*param_data);
    if (new_identify_setting && !prop_data.identifying)
    {
      LwpaThreadParams ithread_params;

      ithread_params.thread_priority = LWPA_THREAD_DEFAULT_PRIORITY;
      ithread_params.stack_size = LWPA_THREAD_DEFAULT_STACK;
      ithread_params.thread_name = "Identify Thread";
      ithread_params.platform_data = NULL;

      lwpa_thread_create(&prop_data.identify_thread, &ithread_params, identify_thread, NULL);
    }
    prop_data.identifying = new_identify_setting;
    *requires_reconnect = false;
    return true;
  }
  else
  {
    *nack_reason = E120_NR_FORMAT_ERROR;
    return false;
  }
}

bool set_device_label(const uint8_t *param_data, uint8_t param_data_len, uint16_t *nack_reason,
                      bool *requires_reconnect)
{
  if (param_data_len >= 1)
  {
    if (param_data_len > DEVICE_LABEL_MAX_LEN)
      param_data_len = DEVICE_LABEL_MAX_LEN;
    memcpy(prop_data.device_label, param_data, param_data_len);
    prop_data.device_label[param_data_len] = '\0';
    *requires_reconnect = false;
    return true;
  }
  else
  {
    *nack_reason = E120_NR_FORMAT_ERROR;
    return false;
  }
}

bool set_component_scope(const uint8_t *param_data, uint8_t param_data_len, uint16_t *nack_reason,
                         bool *requires_reconnect)
{
  if (param_data_len == 66)
  {
    if (upack_16b(param_data) == 1)
    {
      if (param_data_len == strlen(prop_data.rdmnet_params.scope) &&
          strncmp((char *)&param_data[2], prop_data.rdmnet_params.scope, E133_SCOPE_STRING_PADDED_LENGTH) == 0)
      {
        /* Same scope as current */
        *requires_reconnect = false;
      }
      else
      {
        memset(prop_data.rdmnet_params.scope, 0, E133_SCOPE_STRING_PADDED_LENGTH);
        strncpy(prop_data.rdmnet_params.scope, (char *)&param_data[2], E133_SCOPE_STRING_PADDED_LENGTH - 1);
        *requires_reconnect = true;
      }
      return true;
    }
    else
      *nack_reason = E120_NR_DATA_OUT_OF_RANGE;
  }
  else
    *nack_reason = E120_NR_FORMAT_ERROR;
  return false;
}

bool set_broker_static_config_ipv4(const uint8_t *param_data, uint8_t param_data_len, uint16_t *nack_reason,
  bool *requires_reconnect)
{
  uint8_t *cur_ptr = param_data[0];

  *requires_reconnect = false;

  if (param_data_len == 4 + 2 + E133_SCOPE_STRING_PADDED_LENGTH)
  {
    LwpaIpAddr new_ipv4;
    lwpaip_set_v4_address(&new_ipv4, upack_32b(cur_ptr));

    cur_ptr += 4;
    uint16_t new_port = upack_16b(cur_ptr);

    cur_ptr += 2;
    if (strncmp((char *)param_data, prop_data.rdmnet_params.scope, E133_SCOPE_STRING_PADDED_LENGTH) == 0)
    {
      /* setting one field to zero, but not the other is invalid */
      if (!((lwpaip_v4_address(&new_ipv4) == 0 && new_port != 0) || (lwpaip_v4_address(&new_ipv4) != 0 && new_port == 0)))
      {
        /* when both fields are set to zero, remove static config */
        if (lwpaip_v4_address(&new_ipv4) == 0 && new_port == 0)
        {
          lwpaip_set_invalid(&prop_data.rdmnet_params.broker_static_addr.ip);
        }
        else
        {
          lwpaip_set_v4_address(&prop_data.rdmnet_params.broker_static_addr.ip, &new_ipv4);
          prop_data.rdmnet_params.broker_static_addr.port = new_port;
        }
        *requires_reconnect = true;
        return true;
      }
      else
        *nack_reason = E120_NR_DATA_OUT_OF_RANGE;
    }
    else
      *nack_reason = E133_NR_UNKNOWN_SCOPE;
  }
  else
    *nack_reason = E120_NR_FORMAT_ERROR;

  return false;
  }

bool set_search_domain(const uint8_t *param_data, uint8_t param_data_len, uint16_t *nack_reason,
                       bool *requires_reconnect)
{
  if (param_data_len <= E133_DOMAIN_STRING_PADDED_LENGTH)
  {

    if (param_data_len > 0 || strcmp("", (char *)param_data) != 0)
    {
      if (strncmp(prop_data.rdmnet_params.search_domain, (char *)param_data, E133_DOMAIN_STRING_PADDED_LENGTH) == 0)
      {
        /* Same domain as current */
        *requires_reconnect = false;
      }
      else
      {
        strncpy(prop_data.rdmnet_params.search_domain, (char *)param_data, E133_DOMAIN_STRING_PADDED_LENGTH);
        *requires_reconnect = true;
      }
    }
    else
    {
      lwpaip_set_invalid(&prop_data.rdmnet_params.broker_static_addr.ip);
      *requires_reconnect = true;
    }
    return true;
  }
  else
    *nack_reason = E120_NR_FORMAT_ERROR;

  return false;
}

bool set_tcp_comms_status(const uint8_t *param_data, uint8_t param_data_len, uint16_t *nack_reason,
                          bool *requires_reconnect)
{
  *requires_reconnect = false;

  if (param_data_len == E133_SCOPE_STRING_PADDED_LENGTH)
  {
    if (strncmp((char *)param_data, prop_data.rdmnet_params.scope, E133_SCOPE_STRING_PADDED_LENGTH) == 0)
    {
      /* Same scope as current */
      prop_data.tcp_unhealthy_counter = 0;
      return true;
    }
    else
      *nack_reason = E133_NR_UNKNOWN_SCOPE;
  }
  else
    *nack_reason = E120_NR_FORMAT_ERROR;

  return false;
}

bool get_identify_device(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                         size_t *num_responses, uint16_t *nack_reason)
{
  (void)param_data;
  (void)param_data_len;
  (void)nack_reason;

  resp_data_list[0].data[0] = prop_data.identifying ? 1 : 0;
  resp_data_list[0].datalen = 1;
  *num_responses = 1;
  return true;
}

bool get_device_label(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                      size_t *num_responses, uint16_t *nack_reason)
{
  (void)param_data;
  (void)param_data_len;
  (void)nack_reason;

  strncpy((char *)resp_data_list[0].data, prop_data.device_label, DEVICE_LABEL_MAX_LEN);
  resp_data_list[0].datalen = (uint8_t)strnlen(prop_data.device_label, DEVICE_LABEL_MAX_LEN);
  *num_responses = 1;
  return true;
}

bool get_component_scope(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                         size_t *num_responses, uint16_t *nack_reason)
{
  if (param_data_len >= 2)
  {
    if (upack_16b(param_data) == 1)
    {
      pack_16b(resp_data_list[0].data, 1);
      memset(&resp_data_list[0].data[2], 0, E133_SCOPE_STRING_PADDED_LENGTH);
      strncpy((char *)&resp_data_list[0].data[2], prop_data.rdmnet_params.scope, E133_SCOPE_STRING_PADDED_LENGTH - 1);
      resp_data_list[0].datalen = 2 + E133_SCOPE_STRING_PADDED_LENGTH;
      *num_responses = 1;
      return true;
    }
    else
      *nack_reason = E120_NR_DATA_OUT_OF_RANGE;
  }
  else
    *nack_reason = E120_NR_FORMAT_ERROR;
  return false;
}

bool get_broker_static_config_ipv4(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                                   size_t *num_responses, uint16_t *nack_reason)
{
  (void)param_data;
  (void)param_data_len;
  (void)nack_reason;
  uint8_t *cur_ptr = resp_data_list[0].data;
  if (lwpaip_is_invalid(&prop_data.rdmnet_params.broker_static_addr.ip))
  {
    /* Set all 0's for the static IPv4 address and port */
    memset(cur_ptr, 0, 6);
    cur_ptr += 6;
  }
  else
  {
    /* Copy the static IPv4 address and port */
    pack_32b(cur_ptr, lwpaip_v4_address(&prop_data.rdmnet_params.broker_static_addr.ip));
    cur_ptr += 4;
    pack_16b(cur_ptr, prop_data.rdmnet_params.broker_static_addr.port);
    cur_ptr += 2;
  }
  /* Copy the scope string */
  memset(cur_ptr, 0, E133_SCOPE_STRING_PADDED_LENGTH);
  strncpy((char *)cur_ptr, prop_data.rdmnet_params.scope, E133_SCOPE_STRING_PADDED_LENGTH);
  cur_ptr += E133_SCOPE_STRING_PADDED_LENGTH;
  resp_data_list[0].datalen = (uint8_t)(cur_ptr - resp_data_list[0].data);
  *num_responses = 1;
  return true;
}

bool get_search_domain(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                       size_t *num_responses, uint16_t *nack_reason)
{
  (void)param_data;
  (void)param_data_len;
  (void)nack_reason;
  strncpy((char *)resp_data_list[0].data, prop_data.rdmnet_params.search_domain, E133_DOMAIN_STRING_PADDED_LENGTH);
  resp_data_list[0].datalen = (uint8_t)strlen(prop_data.rdmnet_params.search_domain);
  *num_responses = 1;
  return true;
}

bool get_tcp_comms_status(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                          size_t *num_responses, uint16_t *nack_reason)
{
  (void)param_data;
  (void)param_data_len;
  (void)nack_reason;
  uint8_t *cur_ptr = resp_data_list[0].data;

  memcpy(cur_ptr, prop_data.rdmnet_params.scope, E133_SCOPE_STRING_PADDED_LENGTH);
  cur_ptr += E133_SCOPE_STRING_PADDED_LENGTH;
  if (lwpaip_is_v4(&prop_data.cur_broker_addr.ip))
  {
    pack_32b(cur_ptr, lwpaip_v4_address(&prop_data.cur_broker_addr.ip));
    cur_ptr += 4;
    memset(cur_ptr, 0, IPV6_BYTES);
    cur_ptr += IPV6_BYTES;
  }
  else
  {
    pack_32b(cur_ptr, 0);
    cur_ptr += 4;
    memcpy(cur_ptr, lwpaip_v6_address(&prop_data.cur_broker_addr.ip), IPV6_BYTES);
    cur_ptr += IPV6_BYTES;
  }
  pack_16b(cur_ptr, prop_data.cur_broker_addr.port);
  cur_ptr += 2;
  pack_16b(cur_ptr, prop_data.tcp_unhealthy_counter);
  cur_ptr += 2;
  resp_data_list[0].datalen = (uint8_t)(cur_ptr - resp_data_list[0].data);
  *num_responses = 1;
  return true;
}

bool get_supported_parameters(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                              size_t *num_responses, uint16_t *nack_reason)
{
  size_t list_index = 0;
  uint8_t *cur_ptr = resp_data_list[0].data;
  size_t i;

  (void)param_data;
  (void)param_data_len;
  (void)nack_reason;

  for (i = 0; i < NUM_SUPPORTED_PIDS; ++i)
  {
    pack_16b(cur_ptr, kSupportedPIDList[i]);
    cur_ptr += 2;
    if ((cur_ptr - resp_data_list[list_index].data) >= RDM_MAX_PDL - 1)
    {
      resp_data_list[list_index].datalen = (uint8_t)(cur_ptr - resp_data_list[list_index].data);
      cur_ptr = resp_data_list[++list_index].data;
    }
  }
  resp_data_list[list_index].datalen = (uint8_t)(cur_ptr - resp_data_list[list_index].data);
  *num_responses = list_index + 1;
  return true;
}

bool get_device_info(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                     size_t *num_responses, uint16_t *nack_reason)
{
  (void)param_data;
  (void)param_data_len;
  (void)nack_reason;
  memcpy(resp_data_list[0].data, kDeviceInfo, sizeof kDeviceInfo);
  resp_data_list[0].datalen = sizeof kDeviceInfo;
  *num_responses = 1;
  return true;
}

bool get_manufacturer_label(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                            size_t *num_responses, uint16_t *nack_reason)
{
  (void)param_data;
  (void)param_data_len;
  (void)nack_reason;

  strcpy((char *)resp_data_list[0].data, MANUFACTURER_LABEL);
  resp_data_list[0].datalen = sizeof(MANUFACTURER_LABEL) - 1;
  *num_responses = 1;
  return true;
}

bool get_device_model_description(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                                  size_t *num_responses, uint16_t *nack_reason)
{
  (void)param_data;
  (void)param_data_len;
  (void)nack_reason;

  strcpy((char *)resp_data_list[0].data, DEVICE_MODEL_DESCRIPTION);
  resp_data_list[0].datalen = sizeof(DEVICE_MODEL_DESCRIPTION) - 1;
  *num_responses = 1;
  return true;
}

bool get_software_version_label(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                                size_t *num_responses, uint16_t *nack_reason)
{
  (void)param_data;
  (void)param_data_len;
  (void)nack_reason;

  strcpy((char *)resp_data_list[0].data, SOFTWARE_VERSION_LABEL);
  resp_data_list[0].datalen = sizeof(SOFTWARE_VERSION_LABEL) - 1;
  *num_responses = 1;
  return true;
}

bool get_endpoint_list(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                       size_t *num_responses, uint16_t *nack_reason)
{
  (void)param_data;
  (void)param_data_len;
  (void)nack_reason;

  uint8_t *cur_ptr = resp_data_list[0].data;

  /* Hardcoded: no endpoints other than NULL_ENDPOINT. NULL_ENDPOINT is not
   * reported in this response. */
  resp_data_list[0].datalen = 4;
  pack_32b(cur_ptr, prop_data.endpoint_list_change_number);
  *num_responses = 1;
  return true;
}

bool get_endpoint_responders(const uint8_t *param_data, uint8_t param_data_len, param_data_list_t resp_data_list,
                             size_t *num_responses, uint16_t *nack_reason)
{
  (void)param_data;
  (void)param_data_len;
  (void)resp_data_list;
  (void)num_responses;

  if (param_data_len >= 2)
  {
    /* We have no valid endpoints for this message */
    *nack_reason = E137_7_NR_ENDPOINT_NUMBER_INVALID;
  }
  else
    *nack_reason = E120_NR_FORMAT_ERROR;
  return false;
}