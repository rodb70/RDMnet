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

// A class for logging messages from the Broker on macOS.
#ifndef MACOS_BROKER_LOG_H_
#define MACOS_BROKER_LOG_H_

#include <fstream>
#include <string>
#include "etcpal/cpp/log.h"

class MacBrokerLog : public etcpal::LogMessageHandler
{
public:
  bool Startup(int log_mask);
  void Shutdown();

  void                 HandleLogMessage(const EtcPalLogStrings& strings) override;
  etcpal::LogTimestamp GetLogTimestamp() override;

  etcpal::Logger& log_instance() { return logger_; }

private:
  etcpal::Logger logger_;

  std::fstream file_;
};

#endif  // MACOS_BROKER_LOG_
