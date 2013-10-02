/*
 * %CopyrightBegin%
 *
 * Copyright Ericsson AB 2013. All Rights Reserved.
 *
 * The contents of this file are subject to the Erlang Public License,
 * Version 1.1, (the "License"); you may not use this file except in
 * compliance with the License. You should have received a copy of the
 * Erlang Public License along with this software. If not, it can be
 * retrieved online at http://www.erlang.org/.
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * %CopyrightEnd%
 *
 * Author: Björn-Egil Dahlberg
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "sys.h"
#include "erl_vm.h"
#include "global.h"
#include "erl_process.h"
#include "error.h"
#include "bif.h"

#include "erl_map.h"

/* BIFs
 *
 * DONE:
 * - erlang:is_map/1
 * - erlang:map_size/1
 *
 * - map:get/2
 * - map:is_key/2
 * - map:keys/1
 * - map:new/0
 * - map:put/3
 * - map:remove/2
 * - map:to_list/1
 * - map:update/3
 * - map:values/1
 *
 * TODO:
 * - map:from_list/1
 * - map:find/2
 * - map:foldl/3
 * - map:foldr/3
 * - map:map/3
 * - map:size/1
 * - map:without/2
 * - map:merge/2
 *
 */

/* erlang:map_size/1
 * the corresponding instruction is implemented in:
 *     beam/erl_bif_guard.c
 */

BIF_RETTYPE map_size_1(BIF_ALIST_1) {
    if (is_map(BIF_ARG_1)) {
	Eterm *hp;
	Uint hsz  = 0;
	map_t *mp = (map_t*)map_val(BIF_ARG_1);
	Uint n    = map_get_size(mp);

	erts_bld_uint(NULL, &hsz, n);
	hp = HAlloc(BIF_P, hsz);
	BIF_RET(erts_bld_uint(&hp, NULL, n));
    }

    BIF_ERROR(BIF_P, BADARG);
}

/* map:to_list/1
 */

BIF_RETTYPE map_to_list_1(BIF_ALIST_1) {
    if (is_map(BIF_ARG_1)) {
	Uint n;
	Eterm* hp;
	Eterm *ks,*vs, res, tup;
	map_t *mp = (map_t*)map_val(BIF_ARG_1);

	ks  = map_get_keys(mp);
	vs  = map_get_values(mp);
	n   = map_get_size(mp);
	hp  = HAlloc(BIF_P, (2 + 3) * n);
	res = NIL;

	while(n--) {
	    tup = TUPLE2(hp, ks[n], vs[n]); hp += 3;
	    res = CONS(hp, tup, res); hp += 2;
	}

	BIF_RET(res);
    }

    BIF_ERROR(BIF_P, BADARG);
}

/* map:get/2
 */

BIF_RETTYPE map_get_2(BIF_ALIST_2) {
    if (is_map(BIF_ARG_2)) {
	Eterm *hp, *ks,*vs, key, error;
	map_t *mp;
	Uint n,i;
	char *s_error;

	mp  = (map_t*)map_val(BIF_ARG_2);
	key = BIF_ARG_1;
	n   = map_get_size(mp);
	
	if (n == 0)
	    goto error;

	ks  = map_get_keys(mp);
	vs  = map_get_values(mp);

	if (is_immed(key)) {
	    for( i = 0; i < n; i++) {
		if (ks[i] == key) {
		    BIF_RET(vs[i]);
		}
	    }
	}

	for( i = 0; i < n; i++) {
	    if (eq(ks[i], key)) {
		BIF_RET(vs[i]);
	    }
	}
error:

	s_error = "bad_key";
	error = am_atom_put(s_error, sys_strlen(s_error));

	hp = HAlloc(BIF_P, 3);
	BIF_P->fvalue = TUPLE2(hp, error, key);
	BIF_ERROR(BIF_P, EXC_ERROR_2);
    }
    BIF_ERROR(BIF_P, BADARG);
}

/* map:is_key/2
 */

BIF_RETTYPE map_is_key_2(BIF_ALIST_2) {
    if (is_map(BIF_ARG_2)) {
	Eterm *ks, key;
	map_t *mp;
	Uint n,i;

	mp  = (map_t*)map_val(BIF_ARG_2);
	key = BIF_ARG_1;
	n   = map_get_size(mp);
	ks  = map_get_keys(mp);

	if (n == 0)
	    BIF_RET(am_false);

	if (is_immed(key)) {
	    for( i = 0; i < n; i++) {
		if (ks[i] == key) {
		    BIF_RET(am_true);
		}
	    }
	}

	for( i = 0; i < n; i++) {
	    if (eq(ks[i], key)) {
		BIF_RET(am_true);
	    }
	}
	BIF_RET(am_false);
    }
    BIF_ERROR(BIF_P, BADARG);
}

/* map:keys/1
 */

BIF_RETTYPE map_keys_1(BIF_ALIST_1) {
    if (is_map(BIF_ARG_1)) {
	Eterm *hp, *ks, res = NIL;
	map_t *mp;
	Uint n;

	mp  = (map_t*)map_val(BIF_ARG_1);
	n   = map_get_size(mp);

	if (n == 0)
	    BIF_RET(res);

	hp  = HAlloc(BIF_P, (2 * n));
	ks  = map_get_keys(mp);

	while(n--) {
	    res = CONS(hp, ks[n], res); hp += 2;
	}

	BIF_RET(res);
    }
    BIF_ERROR(BIF_P, BADARG);
}

/* map:new/2
 */

BIF_RETTYPE map_new_0(BIF_ALIST_0) {
    Eterm* hp;
    Eterm tup;
    map_t *mp;

    hp    = HAlloc(BIF_P, (3 + 1));
    tup   = make_tuple(hp);
    *hp++ = make_arityval(0);

    mp    = (map_t*)hp;
    mp->thing_word = MAP_HEADER;
    mp->size = 0;
    mp->keys = tup;

    BIF_RET(make_map(mp));
}

/* map:put/3
 */

BIF_RETTYPE map_put_3(BIF_ALIST_3) {
    if (is_map(BIF_ARG_3)) {
	Sint n,i;
	Sint c = 0;
	Eterm* hp, *shp;
	Eterm *ks,*vs, res, key, tup;
	map_t *mp = (map_t*)map_val(BIF_ARG_3);

	key = BIF_ARG_1;
	n   = map_get_size(mp);

	if (n == 0) {
	    hp    = HAlloc(BIF_P, 4 + 2);
	    tup   = make_tuple(hp);
	    *hp++ = make_arityval(1);
	    *hp++ = key;
	    res   = make_map(hp);
	    *hp++ = MAP_HEADER;
	    *hp++ = 1;
	    *hp++ = tup;
	    *hp++ = BIF_ARG_2;

	    BIF_RET(res);
	}

	ks  = map_get_keys(mp);
	vs  = map_get_values(mp);
	/* only allocate for values,
	 * assume key-tuple will be intact
	 */

	hp  = HAlloc(BIF_P, 3 + n);
	shp = hp; /* save hp, used if optimistic update fails */
	res = make_map(hp);
	*hp++ = MAP_HEADER;
	*hp++ = n;
	*hp++ = mp->keys;

	if (is_immed(key)) {
	    for( i = 0; i < n; i ++) {
		if (ks[i] == key) {
		    *hp++ = BIF_ARG_2;
		    vs++;
		    c = 1;
		} else {
		    *hp++ = *vs++;
		}
	    }
	} else {
	    for( i = 0; i < n; i ++) {
		if (eq(ks[i], key)) {
		    *hp++ = BIF_ARG_2;
		    vs++;
		    c = 1;
		} else {
		    *hp++ = *vs++;
		}
	    }
	}

	if (c)
	    BIF_RET(res);

	/* need to make a new tuple,
	 * use old hp since it needs to be recreated anyway.
	 */
	tup    = make_tuple(shp);
	*shp++ = make_arityval(n+1);

	hp    = HAlloc(BIF_P, 3 + n + 1);
	res   = make_map(hp);
	*hp++ = MAP_HEADER;
	*hp++ = n + 1;
	*hp++ = tup;

	ks  = map_get_keys(mp);
	vs  = map_get_values(mp);

	ASSERT(n >= 0);

	/* copy map in order */
	while (n && ((c = cmp(*ks, key)) < 0)) {
	    *shp++ = *ks++;
	    *hp++  = *vs++;
	    n--;
	}

	*shp++ = key;
	*hp++  = BIF_ARG_2;

	ASSERT(n >= 0);

	while(n--) {
	    *shp++ = *ks++;
	    *hp++  = *vs++;
	}
	/* we have one word remaining
	 * this will work out fine once we get the size word
	 * in the header.
	 */
	*shp = make_pos_bignum_header(0);
	BIF_RET(res);
    }

    BIF_ERROR(BIF_P, BADARG);
}

/* map:remove/3
 */

BIF_RETTYPE map_remove_2(BIF_ALIST_2) {
    if (is_map(BIF_ARG_2)) {
	Sint n;
	Sint found = 0;
	Uint need;
	Eterm *thp, *mhp;
	Eterm *ks, *vs, res, key,tup;
	map_t *mp = (map_t*)map_val(BIF_ARG_2);

	key = BIF_ARG_1;
	n   = map_get_size(mp);

	if (n == 0)
	    BIF_RET(BIF_ARG_2);

	ks  = map_get_keys(mp);
	vs  = map_get_values(mp);

	/* Assume key exists.
	 * Release allocated if it didn't.
	 * Allocate key tuple first.
	 */

	need   = n + 1 - 1 + 3 + n - 1; /* tuple - 1 + map - 1 */
	thp    = HAlloc(BIF_P, need);
	mhp    = thp + n;               /* offset with tuple heap size */

	tup    = make_tuple(thp);
	*thp++ = make_arityval(n - 1);

	res    = make_map(mhp);
	*mhp++ = MAP_HEADER;
	*mhp++ = n - 1;
	*mhp++ = tup;

	if (is_immed(key)) {
	    while(n--) {
		if (*ks == key) {
		    ks++;
		    vs++;
		    found = 1;
		} else {
		    *mhp++ = *vs++;
		    *thp++ = *ks++;
		}
	    }
	} else {
	    while(n--) {
		if (eq(*ks, key)) {
		    ks++;
		    vs++;
		    found = 1;
		} else {
		    *mhp++ = *vs++;
		    *thp++ = *ks++;
		}
	    }
	}

	if (found)
	    BIF_RET(res);

	/* Not found, remove allocated memory
	 * and return previous map.
	 */
	HRelease(BIF_P, thp + need, thp);
	BIF_RET(BIF_ARG_2);
    }
    BIF_ERROR(BIF_P, BADARG);
}

/* map:update/3
 */

BIF_RETTYPE map_update_3(BIF_ALIST_3) {
    if (is_map(BIF_ARG_3)) {
	Sint n,i;
	Sint found = 0;
	Eterm* hp,*shp;
	Eterm *ks,*vs, res, key;
	map_t *mp = (map_t*)map_val(BIF_ARG_3);

	key = BIF_ARG_1;
	n   = map_get_size(mp);

	if (n == 0) {
	    BIF_ERROR(BIF_P, BADARG);
	}

	ks  = map_get_keys(mp);
	vs  = map_get_values(mp);

	/* only allocate for values,
	 * assume key-tuple will be intact
	 */

	hp  = HAlloc(BIF_P, 3 + n);
	shp = hp;
	res = make_map(hp);
	*hp++ = MAP_HEADER;
	*hp++ = n;
	*hp++ = mp->keys;

	if (is_immed(key)) {
	    for( i = 0; i < n; i ++) {
		if (ks[i] == key) {
		    *hp++ = BIF_ARG_2;
		    vs++;
		    found = 1;
		} else {
		    *hp++ = *vs++;
		}
	    }
	} else {
	    for( i = 0; i < n; i ++) {
		if (eq(ks[i], key)) {
		    *hp++ = BIF_ARG_2;
		    vs++;
		    found = 1;
		} else {
		    *hp++ = *vs++;
		}
	    }
	}

	if (found)
	    BIF_RET(res);

	HRelease(BIF_P, shp + 3 + n, shp);
    }
    BIF_ERROR(BIF_P, BADARG);
}


/* map:values/1
 */

BIF_RETTYPE map_values_1(BIF_ALIST_1) {
    if (is_map(BIF_ARG_1)) {
	Eterm *hp, *vs, res = NIL;
	map_t *mp;
	Uint n;

	mp  = (map_t*)map_val(BIF_ARG_1);
	n   = map_get_size(mp);

	if (n == 0)
	    BIF_RET(res);

	hp  = HAlloc(BIF_P, (2 * n));
	vs  = map_get_values(mp);

	while(n--) {
	    res = CONS(hp, vs[n], res); hp += 2;
	}

	BIF_RET(res);
    }
    BIF_ERROR(BIF_P, BADARG);
}
