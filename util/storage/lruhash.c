/*
 * util/storage/lrulash.c - hashtable, hash function, LRU keeping.
 *
 * Copyright (c) 2007, NLnet Labs. All rights reserved.
 *
 * This software is open source.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * Neither the name of the NLNET LABS nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *
 * This file contains a hashtable with LRU keeping of entries.
 *
 */

#include "config.h"
#include "util/storage/lruhash.h"

/* ------ local helper functions ------------- */

/** init the hash bins for the table. */
static void bin_init(struct lruhash_bin* array, size_t size);

/** delete the hash bin and entries inside it */
static void bin_delete(struct lruhash* table, struct lruhash_bin* bin);

/** find entry in hash bin. You must have locked the bin.
 * @param table: hash table with function pointers.
 * @param bin: hash bin to look into.
 * @param hash: hash value to look for.
 * @param key: key to look for.
 * @return: the entry or NULL if not found.
 */
static struct lruhash_entry* bin_find_entry(struct lruhash* table, 
	struct lruhash_bin* bin, hashvalue_t hash, void* key);

/**
 * Remove entry from bin overflow chain.
 * You must have locked the bin.
 * @param bin: hash bin to look into.
 * @param entry: entry ptr that needs removal.
 */
static void bin_overflow_remove(struct lruhash_bin* bin, 
	struct lruhash_entry* entry);

/**
 * Split hash bin into two new ones. Based on increased size_mask.
 * Caller must hold hash table lock.
 * At the end the routine acquires all hashbin locks (in the old array).
 * This makes it wait for other threads to finish with the bins.
 * So the bins are ready to be deleted after this function.
 * @param table: hash table with function pointers.
 * @param newa: new increased array.
 * @param newmask: new lookup mask.
 */
static void bin_split(struct lruhash* table, struct lruhash_bin* newa, 
	int newmask);

/** 
 * Try to make space available by deleting old entries.
 * Assumes that the lock on the hashtable is being held by caller.
 * Caller must not hold bin locks.
 * @param table: hash table.
 * @param list: list of entries that are to be deleted later.
 *	Entries have been removed from the hash table and writelock is held.
 */
static void reclaim_space(struct lruhash* table, struct lruhash_entry** list);

/**
 * Grow the table lookup array. Becomes twice as large.
 * Caller must hold the hash table lock. Must not hold any bin locks.
 * Tries to grow, on malloc failure, nothing happened.
 * @param table: hash table.
 */
static void table_grow(struct lruhash* table);

/**
 * Touch entry, so it becomes the most recently used in the LRU list.
 * Caller must hold hash table lock. The entry must be inserted already.
 * @param table: hash table.
 * @param entry: entry to make first in LRU.
 */
static void lru_touch(struct lruhash* table, struct lruhash_entry* entry);

/**
 * Put entry at front of lru. entry must be unlinked from lru.
 * Caller must hold hash table lock.
 * @param table: hash table with lru head and tail.
 * @param entry: entry to make most recently used.
 */
static void lru_front(struct lruhash* table, struct lruhash_entry* entry);

/**
 * Remove entry from lru list.
 * Caller must hold hash table lock.
 * @param table: hash table with lru head and tail.
 * @param entry: entry to remove from lru.
 */
static void lru_remove(struct lruhash* table, struct lruhash_entry* entry);

/* ------ end local helper functions --------- */

static void
bin_init(struct lruhash_bin* array, size_t size)
{
	size_t i;
	for(i=0; i<size; i++) {
		lock_quick_init(&array[i].lock);
		lock_protect(&array[i].lock, &array[i], 
			sizeof(struct lruhash_bin));
	}
}

struct lruhash* lruhash_create(size_t start_size, size_t maxmem,
        lruhash_sizefunc_t sizefunc, lruhash_compfunc_t compfunc,
	lruhash_delkeyfunc_t delkeyfunc, lruhash_deldatafunc_t deldatafunc,
	void* arg)
{
	struct lruhash* table = (struct lruhash*)calloc(1, 
		sizeof(struct lruhash));
	if(!table)
		return NULL;
	lock_quick_init(&table->lock);
	table->sizefunc = sizefunc;
	table->compfunc = compfunc;
	table->delkeyfunc = delkeyfunc;
	table->deldatafunc = deldatafunc;
	table->cb_arg = arg;
	table->size = start_size;
	table->size_mask = (int)(start_size-1);
	table->lru_start = NULL;
	table->lru_end = NULL;
	table->num = 0;
	table->space_used = 0;
	table->space_max = maxmem;
	table->array = calloc(table->size, sizeof(struct lruhash_bin));
	if(!table->array) {
		lock_quick_destroy(&table->lock);
		free(table);
		return NULL;
	}
	bin_init(table->array, table->size);
	lock_protect(&table->lock, table, sizeof(*table));
	lock_protect(&table->lock, table->array, 
		table->size*sizeof(struct lruhash_bin));
	return table;
}

static void bin_delete(struct lruhash* table, struct lruhash_bin* bin)
{
	struct lruhash_entry* p, *np;
	if(!bin)
		return;
	lock_quick_destroy(&bin->lock);
	p = bin->overflow_list;
	bin->overflow_list = NULL;
	while(p) {
		np = p->overflow_next;
		(*table->delkeyfunc)(p->key, table->cb_arg);
		(*table->deldatafunc)(p->data, table->cb_arg);
		p = np;
	}
}

static void 
bin_split(struct lruhash* table, struct lruhash_bin* newa, 
	int newmask)
{
	size_t i;
	struct lruhash_entry *p, *np;
	struct lruhash_bin* newbin;
	/* move entries to new table. Notice that since hash x is mapped to
	 * bin x & mask, and new mask uses one more bit, so all entries in
	 * one bin will go into the old bin or bin | newbit */
	/* newbit = newmask - table->size_mask; */
	/* so, really, this task could also be threaded, per bin. */
	/* LRU list is not changed */
	for(i=0; i<table->size; i++)
	{
		lock_quick_lock(&table->array[i].lock);
		p = table->array[i].overflow_list;
		while(p) {
			np = p->overflow_next;
			/* link into correct new bin */
			newbin = &newa[p->hash & newmask];
			p->overflow_next = newbin->overflow_list;
			newbin->overflow_list = p;
			p=np;
		}
	}
}

void lruhash_delete(struct lruhash* table)
{
	size_t i;
	if(!table)
		return;
	/* delete lock on hashtable to force check its OK */
	lock_quick_destroy(&table->lock);
	for(i=0; i<table->size; i++)
		bin_delete(table, &table->array[i]);
	free(table->array);
	free(table);
}

static void 
bin_overflow_remove(struct lruhash_bin* bin, struct lruhash_entry* entry)
{
	struct lruhash_entry* p = bin->overflow_list;
	struct lruhash_entry** prevp = &bin->overflow_list;
	while(p) {
		if(p == entry) {
			*prevp = p->overflow_next;
			return;
		}
		prevp = &p->overflow_next;
		p = p->overflow_next;
	}
}

static void 
reclaim_space(struct lruhash* table, struct lruhash_entry** list)
{
	struct lruhash_entry* d;
	struct lruhash_bin* bin;
	log_assert(table);
	/* does not delete MRU entry, so table will not be empty. */
	while(table->num > 1 && table->space_used > table->space_max) {
		/* notice that since we hold the hashtable lock, nobody
		   can change the lru chain. So it cannot be deleted underneath
		   us. We still need the hashbin and entry write lock to make 
		   sure we flush all users away from the entry. 
		   which is unlikely, since it is LRU, if someone got a rdlock
		   it would be moved to front, but to be sure. */
		d = table->lru_end;
		/* specialised, delete from end of double linked list,
		   and we know num>1, so there is a previous lru entry. */
		log_assert(d && d->lru_prev);
		table->lru_end = d->lru_prev;
		d->lru_prev->lru_next = NULL;
		/* schedule entry for deletion */
		bin = &table->array[d->hash & table->size_mask];
		table->num --;
		lock_quick_lock(&bin->lock);
		bin_overflow_remove(bin, d);
		d->overflow_next = *list;
		*list = d;
		lock_rw_wrlock(&d->lock);
		table->space_used -= table->sizefunc(d->key, d->data);
		lock_quick_unlock(&bin->lock);
	}
}

static struct lruhash_entry* 
bin_find_entry(struct lruhash* table, 
	struct lruhash_bin* bin, hashvalue_t hash, void* key)
{
	struct lruhash_entry* p = bin->overflow_list;
	while(p) {
		if(p->hash == hash && table->compfunc(p->key, key) == 0)
			return p;
		p = p->overflow_next;
	}
	return NULL;
}

static void 
table_grow(struct lruhash* table)
{
	struct lruhash_bin* newa;
	int newmask;
	size_t i;
	if(table->size_mask == (int)(((size_t)-1)>>1)) {
		log_err("hash array malloc: size_t too small");
		return;
	}
	/* try to allocate new array, if not fail */
	newa = calloc(table->size*2, sizeof(struct lruhash_bin));
	if(!newa) {
		log_err("hash grow: malloc failed");
		/* continue with smaller array. Though its slower. */
		return;
	}
	bin_init(newa, table->size*2);
	newmask = (table->size_mask << 1) | 1;
	bin_split(table, newa, newmask);
	/* delete the old bins */
	lock_unprotect(&table->lock, table->array);
	for(i=0; i<table->size; i++) {
		lock_quick_unlock(&table->array[i].lock);
		lock_quick_destroy(&table->array[i].lock);
	}
	free(table->array);
	
	table->size *= 2;
	table->size_mask = newmask;
	table->array = newa;
	lock_protect(&table->lock, table->array, 
		table->size*sizeof(struct lruhash_bin));
	return;
}

static void 
lru_front(struct lruhash* table, struct lruhash_entry* entry)
{
	entry->lru_prev = NULL;
	entry->lru_next = table->lru_start;
	if(!table->lru_start)
		table->lru_end = entry;
	else	table->lru_start->lru_prev = entry;
	table->lru_start = entry;
}

static void 
lru_remove(struct lruhash* table, struct lruhash_entry* entry)
{
	if(entry->lru_prev)
		entry->lru_prev->lru_next = entry->lru_next;
	else	table->lru_start = entry->lru_next;
	if(entry->lru_next)
		entry->lru_next->lru_prev = entry->lru_prev;
	else	table->lru_end = entry->lru_prev;
}

static void 
lru_touch(struct lruhash* table, struct lruhash_entry* entry)
{
	log_assert(table && entry);
	if(entry == table->lru_start)
		return; /* nothing to do */
	/* remove from current lru position */
	lru_remove(table, entry);
	/* add at front */
	lru_front(table, entry);
}

void lruhash_insert(struct lruhash* table, hashvalue_t hash,
        struct lruhash_entry* entry, void* data)
{
	struct lruhash_bin* bin;
	struct lruhash_entry* found, *reclaimlist=NULL;
	size_t need_size;
	need_size = table->sizefunc(entry->key, data);

	/* find bin */
	lock_quick_lock(&table->lock);
	bin = &table->array[hash & table->size_mask];
	lock_quick_lock(&bin->lock);

	/* see if entry exists already */
	if(!(found=bin_find_entry(table, bin, hash, entry->key))) {
		/* if not: add to bin */
		entry->overflow_next = bin->overflow_list;
		bin->overflow_list = entry;
		lru_front(table, entry);
		table->num++;
		table->space_used += need_size;
	} else {
		/* if so: update data */
		table->space_used += need_size -
			(*table->sizefunc)(found->key, found->data);
		(*table->delkeyfunc)(entry->key, table->cb_arg);
		(*table->deldatafunc)(found->data, table->cb_arg);
		found->data = data;
		lru_touch(table, found);
	}
	lock_quick_unlock(&bin->lock);
	if(table->space_used > table->space_max)
		reclaim_space(table, &reclaimlist);
	if(table->num >= table->size)
		table_grow(table);
	lock_quick_unlock(&table->lock);

	/* finish reclaim if any (outside of critical region) */
	while(reclaimlist) {
		struct lruhash_entry* n = reclaimlist->overflow_next;
		lock_rw_unlock(&reclaimlist->lock);
		(*table->delkeyfunc)(reclaimlist->key, table->cb_arg);
		(*table->deldatafunc)(reclaimlist->data, table->cb_arg);
		reclaimlist = n;
	}
}

struct lruhash_entry* lruhash_lookup(struct lruhash* table, hashvalue_t hash,
	void* key, int wr)
{
	struct lruhash_entry* entry = NULL;
	struct lruhash_bin* bin;

	lock_quick_lock(&table->lock);
	bin = &table->array[hash & table->size_mask];
	lock_quick_lock(&bin->lock);
	if((entry=bin_find_entry(table, bin, hash, key)))
		lru_touch(table, entry);
	lock_quick_unlock(&table->lock);

	if(entry) {
		if(wr)
			lock_rw_wrlock(&entry->lock);
		else	lock_rw_rdlock(&entry->lock);
	}
	lock_quick_unlock(&bin->lock);
	return entry;
}

void lruhash_remove(struct lruhash* table, void* key)
{
}
