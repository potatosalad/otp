/*
 * %CopyrightBegin%
 *
 * Copyright 2022 WhatsApp Inc. and its affiliates. All rights reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * %CopyrightEnd%
 */

#ifndef ERL_DISTACC_H__
#define ERL_DISTACC_H__

/* Uncomment this to make distacc to always be on,
   this reduces overhead a little bit when profiling */
/* #define ERTS_DISTACC_ALWAYS_ON 1 */

#define ERTS_DISTACC_DISABLE 0
#define ERTS_DISTACC_ENABLE  1
#define ERTS_DISTACC_RESET   2
#define ERTS_DISTACC_GATHER  3

/*
 * When adding a new state, you have to:
 * * Add it here
 * * Increment ERTS_DISTACC_STATE_COUNT
 * * Add string value to erts_distacc_states
 * * Have to be in alphabetical order!
 * * Only add states to the non-extended section after
 *   careful benchmarking to make sure the overhead
 *   when disabled is minimal.
 */

// #ifndef ERTS_DISTACC_EXTENDED_STATES
// #define ERTS_DISTACC_STATE_AUX       0
// #define ERTS_DISTACC_STATE_CHECK_IO  1
// #define ERTS_DISTACC_STATE_EMULATOR  2
// #define ERTS_DISTACC_STATE_GC        3
// #define ERTS_DISTACC_STATE_OTHER     4
// #define ERTS_DISTACC_STATE_PORT      5
// #define ERTS_DISTACC_STATE_SLEEP     6

// #define ERTS_DISTACC_STATE_COUNT 7

/*
 * Message types used in distribution messages.
 * 
 * Keep in sync with the `dop' enum type in `dist.h'
 */
// typedef enum {
//     /* DOP_ALIAS_SEND, DOP_ALIAS_SEND_TT */
//     ERTS_DISTACC_MSG_TYPE_ALIAS_SEND = 0,
//     /* DOP_DEMONITOR_P */
//     ERTS_DISTACC_MSG_TYPE_DEMONITOR,
//     /* DOP_EXIT, DOP_EXIT_TT, DOP_PAYLOAD_EXIT, DOP_PAYLOAD_EXIT_TT */
//     ERTS_DISTACC_MSG_TYPE_EXIT,
//     /* DOP_EXIT2, DOP_EXIT2_TT, DOP_PAYLOAD_EXIT2, DOP_PAYLOAD_EXIT2_TT */
//     ERTS_DISTACC_MSG_TYPE_EXIT2,
//     /* DOP_GROUP_LEADER */
//     ERTS_DISTACC_MSG_TYPE_GROUP_LEADER,
//     /* DOP_LINK */
//     ERTS_DISTACC_MSG_TYPE_LINK,
//     /* DOP_MONITOR_P */
//     ERTS_DISTACC_MSG_TYPE_MONITOR,
//     /* DOP_MONITOR_P_EXIT, DOP_PAYLOAD_MONITOR_P_EXIT */
//     ERTS_DISTACC_MSG_TYPE_MONITOR_EXIT,
//     /* DOP_REG_SEND, DOP_REG_SEND_TT */
//     ERTS_DISTACC_MSG_TYPE_REG_SEND,
//     /* DOP_SEND, DOP_SEND_TT, DOP_SEND_SENDER, DOP_SEND_SENDER_TT */
//     ERTS_DISTACC_MSG_TYPE_SEND,
//     /* DOP_SPAWN_REPLY, DOP_SPAWN_REPLY_TT */
//     ERTS_DISTACC_MSG_TYPE_SPAWN_REPLY,
//     /* DOP_SPAWN_REQUEST, DOP_SPAWN_REQUEST_TT */
//     ERTS_DISTACC_MSG_TYPE_SPAWN_REQUEST,
//     /* DOP_UNLINK, DOP_UNLINK_ID */
//     ERTS_DISTACC_MSG_TYPE_UNLINK,
//     /* DOP_UNLINK_ID_ACK */
//     ERTS_DISTACC_MSG_TYPE_UNLINK_ACK,
// } ErtsDistaccMsgType;

/* DOP_ALIAS_SEND, DOP_ALIAS_SEND_TT */
#define ERTS_DISTACC_MSG_TYPE_ALIAS_SEND 0
/* DOP_DEMONITOR_P */
#define ERTS_DISTACC_MSG_TYPE_DEMONITOR 1
/* DOP_EXIT, DOP_EXIT_TT, DOP_PAYLOAD_EXIT, DOP_PAYLOAD_EXIT_TT */
#define ERTS_DISTACC_MSG_TYPE_EXIT 2
/* DOP_EXIT2, DOP_EXIT2_TT, DOP_PAYLOAD_EXIT2, DOP_PAYLOAD_EXIT2_TT */
#define ERTS_DISTACC_MSG_TYPE_EXIT2 3
/* DOP_GROUP_LEADER */
#define ERTS_DISTACC_MSG_TYPE_GROUP_LEADER 4
/* DOP_LINK */
#define ERTS_DISTACC_MSG_TYPE_LINK 5
/* DOP_MONITOR_P */
#define ERTS_DISTACC_MSG_TYPE_MONITOR 6
/* DOP_MONITOR_P_EXIT, DOP_PAYLOAD_MONITOR_P_EXIT */
#define ERTS_DISTACC_MSG_TYPE_MONITOR_EXIT 7
/* DOP_REG_SEND, DOP_REG_SEND_TT */
#define ERTS_DISTACC_MSG_TYPE_REG_SEND 8
/* DOP_SEND, DOP_SEND_TT, DOP_SEND_SENDER, DOP_SEND_SENDER_TT */
#define ERTS_DISTACC_MSG_TYPE_SEND 9
/* DOP_SPAWN_REPLY, DOP_SPAWN_REPLY_TT */
#define ERTS_DISTACC_MSG_TYPE_SPAWN_REPLY 10
/* DOP_SPAWN_REQUEST, DOP_SPAWN_REQUEST_TT */
#define ERTS_DISTACC_MSG_TYPE_SPAWN_REQUEST 11
/* DOP_UNLINK, DOP_UNLINK_ID */
#define ERTS_DISTACC_MSG_TYPE_UNLINK 12
/* DOP_UNLINK_ID_ACK */
#define ERTS_DISTACC_MSG_TYPE_UNLINK_ACK 13

#define ERTS_NUM_OF_DISTACC_MSG_TYPES      (14)

#if defined(ERTS_STRINGS_FOR_DISTACC_MSG_TYPES) && defined(ERTS_ENABLE_DISTACC)
/*
 * Keep this list lexicographically sorted so the maps are happy.
 */
static char *erts_distacc_msg_types[] = {
    "alias_send",
    "demonitor",
    "exit",
    "exit2",
    "group_leader",
    "link",
    "monitor",
    "monitor_exit",
    "reg_send",
    "send",
    "spawn_reply",
    "spawn_request",
    "unlink",
    "unlink_ack"
};
#endif

// typedef enum {
//     ERTS_DISTACC_MSG_ACTION_RECV = 0,
//     ERTS_DISTACC_MSG_ACTION_SEND,
// } ErtsDistaccMsgAction;

#define ERTS_DISTACC_MSG_ACTION_RECV 0
#define ERTS_DISTACC_MSG_ACTION_SEND 1

#define ERTS_NUM_OF_DISTACC_MSG_ACTIONS    (2)

#if defined(ERTS_STRINGS_FOR_DISTACC_MSG_ACTIONS) && defined(ERTS_ENABLE_DISTACC)
/*
 * Keep this list lexicographically sorted so the maps are happy.
 */
static char *erts_distacc_msg_actions[] = {
    "recv",
    "send"
};
#endif

typedef struct erl_distacc_t_ {
    Uint64 counters[ERTS_NUM_OF_DISTACC_MSG_TYPES][ERTS_NUM_OF_DISTACC_MSG_ACTIONS];
} ErtsDistAcc;

#ifdef ERTS_ENABLE_DISTACC

#ifdef ERTS_DISTACC_ALWAYS_ON
#define erts_distacc_enabled 1
#else
extern int ERTS_WRITE_UNLIKELY(erts_distacc_enabled);
#endif

extern void erts_distacc_init(void);
// extern Eterm erts_distacc_gather_stats(ErtsHeapFactory *hfact, const ErtsDistAcc *distacc);
extern Eterm erts_distacc_enable(Process *c_p);
extern Eterm erts_distacc_disable(Process *c_p);
extern Eterm erts_distacc_reset(Process *c_p);
extern Eterm erts_distacc_gather(Process *c_p, int global);
extern Eterm erts_distacc_node_stats(Process *c_p, Eterm node_name);
// extern Eterm erts_bld_dist_msg_stats(ErtsHeapFactory *hfact, const ErtsDistMsgStats *stats);

#define ERTS_DISTACC_IS_ENABLED() ERTS_UNLIKELY(erts_distacc_enabled)

#else

#define ERTS_DISTACC_IS_ENABLED() 0
#define erts_distacc_init() do {} while(0)
// #define erts_msacc_init_thread(type, id, liberty) do {} while(0)
// #define ERTS_MSACC_PUSH_STATE() do {} while(0)
// #define ERTS_MSACC_PUSH_STATE_CACHED() do {} while(0)
// #define ERTS_MSACC_POP_STATE() do {} while(0)
// #define ERTS_MSACC_SET_STATE(state) do {} while(0)
// #define ERTS_MSACC_SET_STATE_CACHED(state) do {} while(0)
// #define ERTS_MSACC_PUSH_AND_SET_STATE(state) do {} while(0)
// #define ERTS_MSACC_PUSH_AND_SET_STATE_CACHED(state) do {} while(0)
// #define ERTS_MSACC_UPDATE_CACHE() do {} while(0)
// #define ERTS_MSACC_IS_ENABLED_CACHED() do {} while(0)
// #define ERTS_MSACC_DECLARE_CACHE() 
// #define ERTS_MSACC_PUSH_STATE_M() do {} while(0)
// #define ERTS_MSACC_PUSH_STATE_CACHED_M() do {} while(0)
// #define ERTS_MSACC_SET_STATE_CACHED_M(state) do {} while(0)
// #define ERTS_MSACC_POP_STATE_M() do {} while(0)
// #define ERTS_MSACC_PUSH_AND_SET_STATE_M(state) do {} while(0)
// #define ERTS_MSACC_SET_BIF_STATE_CACHED_X(Mod,Addr) do {} while(0)

#endif /* ERTS_ENABLE_DISTACC */

#endif /* ERL_DISTACC_H__ */
