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
 * @file rdmnet/core/msg_buf.h
 * @brief Utilities to do piece-wise parsing of an RDMnet message.
 */

#ifndef RDMNET_CORE_MSG_BUF_H_
#define RDMNET_CORE_MSG_BUF_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "etcpal/log.h"
#include "etcpal/uuid.h"
#include "etcpal/socket.h"
#include "rdmnet/core/common.h"
#include "rdmnet/core/message.h"
#include "rdmnet/core/opts.h"

typedef enum
{
  kRCParseResNoData,
  kRCParseResPartialBlockParseOk,
  kRCParseResPartialBlockProtErr,
  kRCParseResFullBlockParseOk,
  kRCParseResFullBlockProtErr
} rc_parse_result_t;

// Tracks state while parsing an ACN PDU block from a byte stream.
// Typically INIT_PDU_BLOCK_STATE() will be called from the parent function of the function that
// parses the PDU block.
typedef struct PduBlockState
{
  size_t block_size;
  size_t size_parsed;
  bool   consuming_bad_block;
  // Whether a header has been parsed for a PDU in this block.
  bool parsed_header;
} PduBlockState;

#define INIT_PDU_BLOCK_STATE(blockstateptr, blocksize) \
  do                                                   \
  {                                                    \
    (blockstateptr)->block_size = blocksize;           \
    (blockstateptr)->size_parsed = 0;                  \
    (blockstateptr)->consuming_bad_block = false;      \
    (blockstateptr)->parsed_header = false;            \
  } while (0)

typedef struct GenericListState
{
  size_t full_list_size;
  size_t size_parsed;
} GenericListState;

#define INIT_GENERIC_LIST_STATE(liststateptr, list_size) \
  do                                                     \
  {                                                      \
    (liststateptr)->full_list_size = list_size;          \
    (liststateptr)->size_parsed = 0;                     \
  } while (0)

typedef struct RdmListState
{
  bool          parsed_request_notif_header;
  PduBlockState block;
} RdmListState;

#define INIT_RDM_LIST_STATE(rlstateptr, blocksize, rmsgptr) \
  do                                                        \
  {                                                         \
    (rlstateptr)->parsed_request_notif_header = false;      \
    INIT_PDU_BLOCK_STATE(&(rlstateptr)->block, blocksize);  \
    RPT_GET_RDM_BUF_LIST(rmsgptr)->rdm_buffers = NULL;      \
    RPT_GET_RDM_BUF_LIST(rmsgptr)->num_rdm_buffers = 0;     \
    RPT_GET_RDM_BUF_LIST(rmsgptr)->more_coming = false;     \
  } while (0)

typedef struct RptStatusState
{
  PduBlockState block;
} RptStatusState;

#define INIT_RPT_STATUS_STATE(rsstateptr, blocksize)       \
  do                                                       \
  {                                                        \
    INIT_PDU_BLOCK_STATE(&(rsstateptr)->block, blocksize); \
  } while (0)

typedef struct RptState
{
  PduBlockState block;
  union
  {
    RdmListState   rdm_list;
    RptStatusState status;
    PduBlockState  unknown;
  } data;
} RptState;

#define INIT_RPT_STATE(rstateptr, blocksize)              \
  do                                                      \
  {                                                       \
    INIT_PDU_BLOCK_STATE(&(rstateptr)->block, blocksize); \
  } while (0)

typedef struct ClientEntryState
{
  size_t            enclosing_block_size;
  bool              parsed_entry_header;
  client_protocol_t client_protocol;
  PduBlockState     entry_data;  // This is only for use with consume_bad_block()
} ClientEntryState;

#define INIT_CLIENT_ENTRY_STATE(cstateptr, blocksize)      \
  do                                                       \
  {                                                        \
    (cstateptr)->enclosing_block_size = (blocksize);       \
    (cstateptr)->parsed_entry_header = false;              \
    (cstateptr)->client_protocol = kClientProtocolUnknown; \
  } while (0)

typedef struct ClientListState
{
  PduBlockState    block;
  ClientEntryState entry;
} ClientListState;

#define INIT_CLIENT_LIST_STATE(clstateptr, blocksize, bmsgptr)                           \
  do                                                                                     \
  {                                                                                      \
    INIT_PDU_BLOCK_STATE(&(clstateptr)->block, blocksize);                               \
    BROKER_GET_CLIENT_LIST(bmsgptr)->client_protocol = kClientProtocolUnknown;           \
    BROKER_GET_RPT_CLIENT_LIST(BROKER_GET_CLIENT_LIST(bmsgptr))->client_entries = NULL;  \
    BROKER_GET_RPT_CLIENT_LIST(BROKER_GET_CLIENT_LIST(bmsgptr))->num_client_entries = 0; \
    BROKER_GET_RPT_CLIENT_LIST(BROKER_GET_CLIENT_LIST(bmsgptr))->more_coming = false;    \
    BROKER_GET_EPT_CLIENT_LIST(BROKER_GET_CLIENT_LIST(bmsgptr))->client_entries = NULL;  \
    BROKER_GET_EPT_CLIENT_LIST(BROKER_GET_CLIENT_LIST(bmsgptr))->num_client_entries = 0; \
    BROKER_GET_EPT_CLIENT_LIST(BROKER_GET_CLIENT_LIST(bmsgptr))->more_coming = false;    \
  } while (0)

typedef struct ClientConnectState
{
  size_t           pdu_data_size;
  bool             common_data_parsed;
  ClientEntryState entry;
} ClientConnectState;

#define INIT_CLIENT_CONNECT_STATE(cstateptr, blocksize, bmsgptr) \
  do                                                             \
  {                                                              \
    (cstateptr)->pdu_data_size = blocksize;                      \
    (cstateptr)->common_data_parsed = false;                     \
  } while (0)

typedef struct ClientEntryUpdateState
{
  size_t           pdu_data_size;
  bool             common_data_parsed;
  ClientEntryState entry;
} ClientEntryUpdateState;

#define INIT_CLIENT_ENTRY_UPDATE_STATE(ceustateptr, blocksize, bmsgptr) \
  do                                                                    \
  {                                                                     \
    (ceustateptr)->pdu_data_size = blocksize;                           \
    (ceustateptr)->common_data_parsed = false;                          \
  } while (0)

typedef struct BrokerState
{
  PduBlockState block;
  union
  {
    GenericListState       data_list;
    ClientListState        client_list;
    ClientConnectState     client_connect;
    ClientEntryUpdateState update;
    PduBlockState          unknown;
  } data;
} BrokerState;

#define INIT_BROKER_STATE(bstateptr, blocksize, msgptr) INIT_PDU_BLOCK_STATE(&(bstateptr)->block, blocksize)

typedef struct RlpState
{
  PduBlockState block;
  union
  {
    BrokerState   broker;
    RptState      rpt;
    PduBlockState unknown;
  } data;
} RlpState;

#define INIT_RLP_STATE(rlpstateptr, blocksize) INIT_PDU_BLOCK_STATE(&(rlpstateptr)->block, blocksize)

#define RC_MSG_BUF_SIZE (RDMNET_RECV_DATA_MAX_SIZE * 2)

typedef struct RCMsgBuf
{
  uint8_t       buf[RC_MSG_BUF_SIZE];
  size_t        cur_data_size;
  RdmnetMessage msg;

  bool     have_preamble;
  RlpState rlp_state;

  const EtcPalLogParams* lparams;
} RCMsgBuf;

#ifdef __cplusplus
extern "C" {
#endif

void           rc_msg_buf_init(RCMsgBuf* msg_buf);
etcpal_error_t rc_msg_buf_recv(RCMsgBuf* buf, etcpal_socket_t socket);
etcpal_error_t rc_msg_buf_parse_data(RCMsgBuf* msg_buf);

#ifdef __cplusplus
}
#endif

#endif /* RDMNET_CORE_MSG_BUF_H_ */
