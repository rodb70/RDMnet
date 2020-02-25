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

#ifndef RDMNET_CPP_LLRP_MANAGER_H_
#define RDMNET_CPP_LLRP_MANAGER_H_

#include <cstdint>
#include "etcpal/cpp/error.h"
#include "etcpal/cpp/uuid.h"
#include "rdmnet/llrp_manager.h"
#include "rdmnet/cpp/core.h"
#include "rdmnet/cpp/message.h"

namespace rdmnet
{
namespace llrp
{
using ManagerHandle = llrp_manager_t;

class Manager;

class ManagerNotifyHandler
{
public:
  virtual void HandleLlrpTargetDiscovered(ManagerHandle handle, const DiscoveredLlrpTarget& target) = 0;
  virtual void HandleLlrpDiscoveryFinished(ManagerHandle handle) = 0;
  virtual void HandleLlrpRdmResponseReceived(ManagerHandle handle, const RemoteRdmResponse& resp) = 0;
};

struct ManagerData
{
  RdmnetMcastNetintId netint{};
  uint16_t manufacturer_id{};
  etcpal::Uuid cid;

  /// Create an empty, invalid data structure by default.
  ManagerData() = default;
  ManagerData(etcpal_iptype_t ip_type, unsigned int netint_index, uint16_t manufacturer_id_in,
              const etcpal::Uuid cid_in = etcpal::Uuid::OsPreferred());

  bool IsValid() const;
};

inline ManagerData::ManagerData(etcpal_iptype_t ip_type, unsigned int netint_index, uint16_t manufacturer_id_in,
                                const etcpal::Uuid cid_in = etcpal::Uuid::OsPreferred())
    : netint{ip_type, netint_index}, manufacturer_id(manufacturer_id_in), cid(cid_in)
{
}

bool ManagerData::IsValid() const
{
  return ((netint.ip_type == kEtcPalIpTypeV4 || netint.ip_type == kEtcPalIpTypeV6) && (netint.index != 0) &&
          (manufacturer_id != 0 && manufacturer_id != 0xffff) && !cid.IsNull());
}

class Manager
{
public:
  Manager(const Manager& other) = delete;
  Manager& operator=(const Manager& other) = delete;

  etcpal::Error Startup(ManagerNotifyHandler& notify_handler, const ManagerData& data);
  void Shutdown();

  etcpal::Error StartDiscovery(uint16_t filter);
  etcpal::Error StopDiscovery();
  etcpal::Expected<uint32_t> SendRdmCommand(const LocalRdmCommand& cmd);
};

};  // namespace llrp
};  // namespace rdmnet

#endif  // RDMNET_CPP_LLRP_MANAGER_H_
