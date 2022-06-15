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

/*
 * Description: Erlang Distribution Protocol accounting.
 *
 *              We keep track of the different states that the
 *              Erlang VM threads are in, in order to provide
 *              performance/debugging statistics. There is a
 *              small overhead in enabling this, but in the big
 *              scheme of things it should be negligible.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#define ERTS_STRINGS_FOR_DISTACC_MSG_ACTIONS 1
#define ERTS_STRINGS_FOR_DISTACC_MSG_TYPES 1

#include "sys.h"
#include "global.h"
#include "erl_threads.h"
#include "erl_bif_unique.h"
#include "erl_map.h"
#include "erl_distacc.h"
#include "erl_bif_table.h"

#ifdef ERTS_ENABLE_DISTACC

static Eterm erts_distacc_gather_stats(ErtsHeapFactory *hfact, const DistEntry *dep, const ErtsDistAcc *distacc);
static void erts_distacc_reset_single(ErtsDistAcc *distacc);

#ifndef ERTS_DISTACC_ALWAYS_ON
int ERTS_WRITE_UNLIKELY(erts_distacc_enabled);
#endif

static Eterm *erts_distacc_msg_type_atoms = NULL;
static Eterm *erts_distacc_msg_action_atoms = NULL;

void
erts_distacc_init(void)
{
    int i;

#ifndef ERTS_DISTACC_ALWAYS_ON
    erts_distacc_enabled = 0;
#endif

    erts_distacc_msg_type_atoms = erts_alloc(ERTS_ALC_T_DISTACC,
                                             sizeof(Eterm) * ERTS_NUM_OF_DISTACC_MSG_TYPES);
    for (i = 0; i < ERTS_NUM_OF_DISTACC_MSG_TYPES; i++)
        erts_distacc_msg_type_atoms[i] = am_atom_put(erts_distacc_msg_types[i],
                                                     sys_strlen(erts_distacc_msg_types[i]));

    erts_distacc_msg_action_atoms = erts_alloc(ERTS_ALC_T_DISTACC,
                                               sizeof(Eterm) * ERTS_NUM_OF_DISTACC_MSG_ACTIONS);
    for (i = 0; i < ERTS_NUM_OF_DISTACC_MSG_ACTIONS; i++)
        erts_distacc_msg_action_atoms[i] = am_atom_put(erts_distacc_msg_actions[i],
                                                       sys_strlen(erts_distacc_msg_actions[i]));
}

Eterm
erts_distacc_gather_stats(ErtsHeapFactory *hfact, const DistEntry *dep, const ErtsDistAcc *distacc)
{
    int type;
    int action;
    Eterm type_vals[ERTS_NUM_OF_DISTACC_MSG_TYPES];
    Eterm action_vals[ERTS_NUM_OF_DISTACC_MSG_ACTIONS];
    Eterm *hp = NULL;
    Uint sz = 0;
    Eterm counters = THE_NON_VALUE;
    Eterm res = THE_NON_VALUE;

    for (type = 0; type < ERTS_NUM_OF_DISTACC_MSG_TYPES; type++) {
        hp = NULL;
        sz = 0;
        for (action = 0; action < ERTS_NUM_OF_DISTACC_MSG_ACTIONS; action++)
            action_vals[action] = erts_bld_uint64(NULL, &sz, distacc->counters[type][action]);
        hp = erts_produce_heap(hfact, sz, 0);
        for (action = 0; action < ERTS_NUM_OF_DISTACC_MSG_ACTIONS; action++)
            action_vals[action] = erts_bld_uint64(&hp, NULL, distacc->counters[type][action]);
        type_vals[type] = erts_map_from_sorted_ks_and_vs(hfact, erts_distacc_msg_action_atoms, action_vals, (Uint) ERTS_NUM_OF_DISTACC_MSG_ACTIONS, NULL);
    }
    counters = erts_map_from_sorted_ks_and_vs(hfact, erts_distacc_msg_type_atoms, type_vals, (Uint) ERTS_NUM_OF_DISTACC_MSG_TYPES, NULL);
    if (dep == NULL) {
        hp = erts_produce_heap(hfact, MAP1_SZ, 0);
        res = MAP1(hp, am_counters, counters);
        hp += MAP1_SZ;
    } else {
        hp = erts_produce_heap(hfact, MAP2_SZ, 0);
        res = MAP2(hp, am_counters, counters, am_name, dep->sysname);
        hp += MAP2_SZ;
    }

    return res;
}

static void
erts_distacc_reset_single(ErtsDistAcc *distacc)
{
    int type;
    int action;

    for (type = 0; type < ERTS_NUM_OF_DISTACC_MSG_TYPES; type++)
        for (action = 0; action < ERTS_NUM_OF_DISTACC_MSG_ACTIONS; action++)
            distacc->counters[type][action] = 0;
}

#endif /* ERTS_ENABLE_DISTACC */

/*
 * This function is responsible for enabling, disabling, resetting and
 * gathering data related to microstate accounting.
 *
 * Managed threads and unmanaged threads are handled differently.
 *   - managed threads get a misc_aux job telling them to switch on msacc
 *   - unmanaged have some fields protected by a mutex that has to be taken
 *     before any values can be updated
 *
 * For performance reasons there is also a global value erts_msacc_enabled
 * that controls the state of all threads. Statistics gathering is only on
 * if erts_msacc_enabled && msacc is true.
 */

#ifdef ERTS_ENABLE_DISTACC

static void distacc_enable_before_runlock_cb(Process *c_p, void *state);
static void distacc_enable_foreach_cb(Process *c_p, DistEntry *dep, void *state);
static Eterm distacc_enable_finalize_cb(Process *c_p, void *state);

void
distacc_enable_before_runlock_cb(Process *c_p, void *state)
{
    (void) c_p;
    (void) state;
#ifndef ERTS_DISTACC_ALWAYS_ON
    /* enable the global value */
    erts_distacc_enabled = 1;
#endif
}

void
distacc_enable_foreach_cb(Process *c_p, DistEntry *dep, void *state)
{
    (void) c_p;
    (void) state;

    (void) erts_de_rwlock(dep);
    if (dep->distacc == NULL) {
        dep->distacc = erts_alloc(ERTS_ALC_T_DISTACC, sizeof(ErtsDistAcc));
        (void) sys_memzero((void *) dep->distacc, sizeof(ErtsDistAcc));
        (void) erts_de_rwunlock(dep);
        /* keep DistEntry around longer while distacc is attached */
        (void) erts_ref_dist_entry(dep);
    } else {
        (void) erts_de_rwunlock(dep);
    }
}

Eterm
distacc_enable_finalize_cb(Process *c_p, void *state)
{
    (void) c_p;
    (void) state;

    return am_true;
}

#endif

Eterm
erts_distacc_enable(Process *c_p)
{
#ifdef ERTS_ENABLE_DISTACC
    DistEntryForEachInit init = {
        .before_runlock_cb = distacc_enable_before_runlock_cb,
        .foreach_cb = distacc_enable_foreach_cb,
        .finalize_cb = distacc_enable_finalize_cb,
        .dtor_cb = NULL,
        .state = NULL,
        .flags = ERTS_DE_ITER_FLAG_KNOWN,
    };
    return erts_dist_entry_foreach_blocking(c_p, &init);
#else
    return THE_NON_VALUE;
#endif
}

#ifdef ERTS_ENABLE_DISTACC

static void distacc_disable_before_runlock_cb(Process *c_p, void *state);
static void distacc_disable_foreach_cb(Process *c_p, DistEntry *dep, void *state);
static Eterm distacc_disable_finalize_cb(Process *c_p, void *state);

void
distacc_disable_before_runlock_cb(Process *c_p, void *state)
{
    (void) c_p;
    (void) state;
#ifndef ERTS_DISTACC_ALWAYS_ON
    /* disable the global value */
    erts_distacc_enabled = 0;
#endif
}

void
distacc_disable_foreach_cb(Process *c_p, DistEntry *dep, void *state)
{
    (void) c_p;
    (void) state;

    (void) erts_de_rwlock(dep);
    if (dep->distacc != NULL) {
        (void) sys_memzero((void *) dep->distacc, sizeof(ErtsDistAcc));
        (void) erts_free(ERTS_ALC_T_DISTACC, (void *)dep->distacc);
        dep->distacc = NULL;
        (void) erts_de_rwunlock(dep);
        /* release DistEntry now that distacc is detached */
        (void) erts_deref_dist_entry(dep);
    } else {
        (void) erts_de_rwunlock(dep);
    }
}

Eterm
distacc_disable_finalize_cb(Process *c_p, void *state)
{
    (void) c_p;
    (void) state;

    return am_true;
}

#endif

Eterm
erts_distacc_disable(Process *c_p)
{
#ifdef ERTS_ENABLE_DISTACC
    DistEntryForEachInit init = {
        .before_runlock_cb = distacc_disable_before_runlock_cb,
        .foreach_cb = distacc_disable_foreach_cb,
        .finalize_cb = distacc_disable_finalize_cb,
        .dtor_cb = NULL,
        .state = NULL,
        .flags = ERTS_DE_ITER_FLAG_KNOWN,
    };
    return erts_dist_entry_foreach_blocking(c_p, &init);
#else
    return THE_NON_VALUE;
#endif
}

#ifdef ERTS_ENABLE_DISTACC

static void distacc_reset_foreach_cb(Process *c_p, DistEntry *dep, void *state);
static Eterm distacc_reset_finalize_cb(Process *c_p, void *state);

void
distacc_reset_foreach_cb(Process *c_p, DistEntry *dep, void *state)
{
    (void) c_p;
    (void) state;

    (void) erts_de_rwlock(dep);
    if (dep->distacc != NULL)
        (void) erts_distacc_reset_single(dep->distacc);
    (void) erts_de_rwunlock(dep);
}

Eterm
distacc_reset_finalize_cb(Process *c_p, void *state)
{
    (void) c_p;
    (void) state;

    return am_true;
}

#endif

Eterm
erts_distacc_reset(Process *c_p)
{
#ifdef ERTS_ENABLE_DISTACC
    DistEntryForEachInit init = {
        .before_runlock_cb = NULL,
        .foreach_cb = distacc_reset_foreach_cb,
        .finalize_cb = distacc_reset_finalize_cb,
        .dtor_cb = NULL,
        .state = NULL,
        .flags = ERTS_DE_ITER_FLAG_KNOWN,
    };
    return erts_dist_entry_foreach_blocking(c_p, &init);
#else
    return THE_NON_VALUE;
#endif
}

#ifdef ERTS_ENABLE_DISTACC

typedef struct distacc_gather_ctx_s {
    int global;
    ErtsDistAcc distacc;
    ErtsHeapFactory factory;
    Eterm result;
} DistAccGatherCtx;

static void distacc_gather_foreach_cb(Process *c_p, DistEntry *dep, void *state);
static Eterm distacc_gather_finalize_cb(Process *c_p, void *state);
static void distacc_gather_dtor_cb(Process *c_p, void *state);

void
distacc_gather_foreach_cb(Process *c_p, DistEntry *dep, void *state)
{
    DistAccGatherCtx *ctx = (void *) state;
    int type;
    int action;

    (void) c_p;

    (void) erts_de_rlock(dep);
    if (dep->distacc != NULL) {
        if (ctx->global) {
            for (type = 0; type < ERTS_NUM_OF_DISTACC_MSG_TYPES; type++)
                for (action = 0; action < ERTS_NUM_OF_DISTACC_MSG_ACTIONS; action++)
                    ctx->distacc.counters[type][action] += dep->distacc->counters[type][action];
        } else {
            Eterm *hp = erts_produce_heap(&ctx->factory, 2, 0);
            Eterm stats = erts_distacc_gather_stats(&ctx->factory, dep, dep->distacc);
            ctx->result = CONS(hp, stats, ctx->result);
        }
    }
    (void) erts_de_runlock(dep);
}

Eterm
distacc_gather_finalize_cb(Process *c_p, void *state)
{
    DistAccGatherCtx *ctx = (void *) state;
    if (ctx->global) {
        return erts_distacc_gather_stats(&ctx->factory, NULL, &ctx->distacc);
    } else {
        return ctx->result;
    }
}

void
distacc_gather_dtor_cb(Process *c_p, void *state)
{
    DistAccGatherCtx *ctx = (void *) state;
    (void) c_p;
    if (ctx) {
        (void) erts_factory_close(&ctx->factory);
        (void) erts_free(ERTS_ALC_T_DISTACC, (void *) ctx);
    }
}

#endif

Eterm
erts_distacc_gather(Process *c_p, int global)
{
#ifdef ERTS_ENABLE_DISTACC
    DistAccGatherCtx *ctx = erts_alloc(ERTS_ALC_T_DISTACC, sizeof(DistAccGatherCtx));
    DistEntryForEachInit init = {
        .before_runlock_cb = NULL,
        .foreach_cb = distacc_gather_foreach_cb,
        .finalize_cb = distacc_gather_finalize_cb,
        .dtor_cb = distacc_gather_dtor_cb,
        .state = (void *) ctx,
        .flags = ERTS_DE_ITER_FLAG_KNOWN,
    };
    (void) sys_memzero((void *) ctx, sizeof(DistAccGatherCtx));
    ctx->global = global;
    ctx->result = (ctx->global) ? THE_NON_VALUE : NIL;
    (void) erts_factory_proc_init(&ctx->factory, c_p);
    return erts_dist_entry_foreach_blocking(c_p, &init);
#else
    return THE_NON_VALUE;
#endif
}

Eterm
erts_distacc_node_stats(Process *c_p, Eterm node_name)
{
#ifdef ERTS_ENABLE_DISTACC
    int type;
    int action;
    ErtsHeapFactory factory;
    DistEntry *dep = NULL;
    ErtsDistAcc distacc;
    Eterm res = THE_NON_VALUE;

    if (ERTS_DISTACC_IS_ENABLED()) {
        (void) sys_memzero((void *) &distacc, sizeof(ErtsDistAcc));
        dep = erts_find_dist_entry(node_name);
        if (dep != NULL) {
            (void) erts_ref_dist_entry(dep);
            (void) erts_de_rlock(dep);
            if (dep->distacc != NULL)
                for (type = 0; type < ERTS_NUM_OF_DISTACC_MSG_TYPES; type++)
                    for (action = 0; action < ERTS_NUM_OF_DISTACC_MSG_ACTIONS; action++)
                        distacc.counters[type][action] += dep->distacc->counters[type][action];
            (void) erts_factory_proc_init(&factory, c_p);
            res = erts_distacc_gather_stats(&factory, dep, &distacc);
            (void) erts_factory_close(&factory);
            (void) erts_de_runlock(dep);
            (void) erts_deref_dist_entry(dep);
        }
    }

    if (is_non_value(res))
        res = am_undefined;

    return res;
#else
    return THE_NON_VALUE;
#endif
}
