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

/*
 * rdmnet_mock/discovery.h
 * Mocking the functions of rdmnet/discovery.h
 */

#ifndef RDMNET_MOCK_DISCOVERY_H_
#define RDMNET_MOCK_DISCOVERY_H_

#include "rdmnet/discovery.h"
#include "fff.h"

#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VOID_FUNC(rdmnet_disc_init_broker_info, RdmnetBrokerDiscInfo*);
DECLARE_FAKE_VALUE_FUNC(etcpal_error_t,
                        rdmnet_disc_start_monitoring,
                        const RdmnetScopeMonitorConfig*,
                        rdmnet_scope_monitor_t*,
                        int*);
DECLARE_FAKE_VOID_FUNC(rdmnet_disc_stop_monitoring, rdmnet_scope_monitor_t);
DECLARE_FAKE_VOID_FUNC(rdmnet_disc_stop_monitoring_all);
DECLARE_FAKE_VALUE_FUNC(etcpal_error_t,
                        rdmnet_disc_register_broker,
                        const RdmnetBrokerRegisterConfig*,
                        rdmnet_registered_broker_t*);
DECLARE_FAKE_VOID_FUNC(rdmnet_disc_unregister_broker, rdmnet_registered_broker_t);

void rdmnet_disc_reset_all_fakes(void);

#ifdef __cplusplus
}
#endif

#endif /* RDMNET_MOCK_DISCOVERY_H_ */
