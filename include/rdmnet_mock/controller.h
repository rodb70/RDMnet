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

#ifndef RDMNET_MOCK_CONTROLLER_H_
#define RDMNET_MOCK_CONTROLLER_H_

#include "rdmnet/controller.h"
#include "fff.h"

DECLARE_FAKE_VALUE_FUNC(etcpal_error_t, rdmnet_controller_init, const EtcPalLogParams*, const RdmnetNetintConfig*);
DECLARE_FAKE_VOID_FUNC(rdmnet_controller_deinit);
DECLARE_FAKE_VOID_FUNC(rdmnet_controller_config_init, RdmnetControllerConfig*, uint16_t);
DECLARE_FAKE_VALUE_FUNC(etcpal_error_t, rdmnet_controller_create, const RdmnetControllerConfig*, rdmnet_controller_t*);
DECLARE_FAKE_VALUE_FUNC(etcpal_error_t, rdmnet_controller_destroy, rdmnet_controller_t, rdmnet_disconnect_reason_t);
DECLARE_FAKE_VALUE_FUNC(etcpal_error_t, rdmnet_controller_add_scope, rdmnet_controller_t, const RdmnetScopeConfig*,
                        rdmnet_client_scope_t*);
DECLARE_FAKE_VALUE_FUNC(etcpal_error_t, rdmnet_controller_add_default_scope, rdmnet_controller_t,
                        rdmnet_client_scope_t*);
DECLARE_FAKE_VALUE_FUNC(etcpal_error_t, rdmnet_controller_remove_scope, rdmnet_controller_t, rdmnet_client_scope_t,
                        rdmnet_disconnect_reason_t);
DECLARE_FAKE_VALUE_FUNC(etcpal_error_t, rdmnet_controller_get_scope, rdmnet_controller_t, rdmnet_client_scope_t,
                        RdmnetScopeConfig*);
DECLARE_FAKE_VALUE_FUNC(etcpal_error_t, rdmnet_controller_send_rdm_command, rdmnet_controller_t, rdmnet_client_scope_t,
                        const RdmnetLocalRdmCommand*, uint32_t*);
DECLARE_FAKE_VALUE_FUNC(etcpal_error_t, rdmnet_controller_send_rdm_response, rdmnet_controller_t, rdmnet_client_scope_t,
                        const RdmnetLocalRdmResponse*);
DECLARE_FAKE_VALUE_FUNC(etcpal_error_t, rdmnet_controller_send_llrp_response, rdmnet_controller_t,
                        const LlrpLocalRdmResponse*);
DECLARE_FAKE_VALUE_FUNC(etcpal_error_t, rdmnet_controller_request_client_list, rdmnet_controller_t,
                        rdmnet_client_scope_t);

#endif /* RDMNET_MOCK_CONTROLLER_H_ */