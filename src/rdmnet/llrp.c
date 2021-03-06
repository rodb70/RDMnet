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

#include "rdmnet/llrp.h"

#include "etcpal/common.h"

/*************************** Function definitions ****************************/

// clang-format off
static const char* kLlrpCompTypeStrings[] =
{
  "RDMnet Device",
  "RDMnet Controller",
  "RDMnet Broker",
  "LLRP Only"
};
#define NUM_LLRP_COMP_TYPE_STRINGS (sizeof(kLlrpCompTypeStrings) / sizeof(const char*))
// clang-format on

/**
 * @brief Get a string description of an LLRP component type.
 *
 * LLRP component types describe the type of RDMnet component with which an LLRP target is
 * associated.
 *
 * @param type Type code.
 * @return String, or NULL if type is invalid.
 */
const char* llrp_component_type_to_string(llrp_component_t type)
{
  if (type >= 0 && type < NUM_LLRP_COMP_TYPE_STRINGS)
    return kLlrpCompTypeStrings[type];
  return "Invalid LLRP Component Type";
}
