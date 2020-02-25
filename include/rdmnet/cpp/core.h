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

#ifndef RDMNET_CPP_CORE_H_
#define RDMNET_CPP_CORE_H_

/// \file rdmnet/cpp/core.h
/// \brief C++ wrapper for the RDMnet Core Library init/deinit functions

#include "etcpal/cpp/error.h"
#include "rdmnet/core.h"

namespace rdmnet
{
etcpal::Error Init(const EtcPalLogParams* log_params = nullptr, const RdmnetNetintConfig* netint_config = nullptr);
etcpal::Error Init(const EtcPalLogParams* log_params = nullptr, const std::vector<RdmnetMcastNetintId>& mcast_netints);
etcpal::Error Init(const etcpal::Logger& logger, const RdmnetNetintConfig* netint_config = nullptr);
etcpal::Error Init(const etcpal::Logger& logger, const std::vector<RdmnetMcastNetintId>& mcast_netints);

void Deinit();
};  // namespace rdmnet

#endif  // RDMNET_CPP_CORE_H_
