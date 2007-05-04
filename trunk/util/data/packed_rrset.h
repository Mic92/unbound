/*
 * util/data/packed_rrset.h - data storage for a set of resource records.
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
 * This file contains the data storage for RRsets.
 */

#ifndef UTIL_DATA_PACKED_RRSET_H
#define UTIL_DATA_PACKED_RRSET_H
#include "util/storage/lruhash.h"
struct alloc_cache;

/** type used to uniquely identify rrsets. Cannot be reused without
 * clearing the cache. */
typedef uint64_t rrset_id_t;

/** this rrset is NSEC/NSEC3 and is at zone apex (at child size of zonecut) */
#define PACKED_RRSET_NSEC_AT_APEX 0x1
/** this rrset was obtained and is used for checking-disabled replies. */
#define PACKED_RRSET_CD 0x2

/**
 * The identifying information for an RRset.
 */
struct packed_rrset_key {
	/**
	 * The domain name. If not null (for id=0) it is allocated, and
	 * contains the wireformat domain name.
	 * This dname is not canonicalized.
	 * After the dname uint16_t type and uint16_t class is stored 
	 * in wireformat.
	 * Use accessor functions to get type and class values.
	 */
	uint8_t* dname;
	/** 
	 * Length of the domain name, including last 0 root octet. 
	 * The value+sizeof(uint16_t)*2 is actually allocated. 
	 */
	size_t dname_len;
	/**
	 * Flags. 32bit to be easy for hashing:
	 * 	o PACKED_RRSET_NSEC_AT_APEX
	 * 	o PACKED_RRSET_CD
	 */
	uint32_t flags;
};

/**
 * This structure contains an RRset. A set of resource records that
 * share the same domain name, type and class.
 *
 * Due to memory management and threading, the key structure cannot be
 * deleted, although the data can be. The id can be set to 0 to store and the
 * structure can be recycled with a new id.
 */
struct ub_packed_rrset_key {
	/** 
	 * entry into hashtable. Note the lock is never destroyed,
	 *  even when this key is retired to the cache. 
	 * the data pointer (if not null) points to a struct packed_rrset.
	 */
	struct lruhash_entry entry;
	/** 
	 * the ID of this rrset. unique, based on threadid + sequenceno. 
	 * ids are not reused, except after flushing the cache.
	 * zero is an unused entry, and never a valid id.
	 * Check this value after getting entry.lock.
	 * The other values in this struct may only be altered after changing
	 * the id (which needs a writelock on entry.lock).
	 */
	rrset_id_t id;
	/** key data: dname, type and class */
	struct packed_rrset_key rk;
};

/**
 * RRset trustworthiness. Bigger value is more trust. RFC 2181.
 * The rrset_trust_add_noAA, rrset_trust_auth_noAA, rrset_trust_add_AA,
 * are mentioned as the same trustworthiness in 2181, but split up here
 * for ease of processing.
 *
 * rrset_trust_nonauth_ans_AA, rrset_trust_ans_noAA
 * are also mentioned as the same trustworthiness in 2181, but split up here
 * for ease of processing.
 *
 * Added trust_none for a sane initial value, smaller than anything else.
 * Added validated and ultimate trust for keys and rrsig validated content.
 */
enum rrset_trust {
	/** initial value for trust */
	rrset_trust_none = 0,
	/** Additional information from non-authoritative answers */
	rrset_trust_add_noAA,
	/** Data from the authority section of a non-authoritative answer */
	rrset_trust_auth_noAA,
	/** Additional information from an authoritative answer */
	rrset_trust_add_AA,
	/** non-authoritative data from the answer section of authoritative
	 * answers */
	rrset_trust_nonauth_ans_AA,
	/** Data from the answer section of a non-authoritative answer */
	rrset_trust_ans_noAA,
	/** Glue from a primary zone, or glue from a zone transfer */
	rrset_trust_glue,
	/** Data from the authority section of an authoritative answer */
	rrset_trust_auth_AA,
	/** The authoritative data included in the answer section of an
	 *  authoritative reply */
	rrset_trust_ans_AA,
	/** Data from a zone transfer, other than glue */
	rrset_trust_sec_noglue,
	/** Data from a primary zone file, other than glue data */
	rrset_trust_prim_noglue,
	/** DNSSEC(rfc4034) validated with trusted keys */
	rrset_trust_validated,
	/** ultimately trusted, no more trust is possible; 
	 * trusted keys from the unbound configuration setup. */
	rrset_trust_ultimate
};

/**
 * RRset data.
 *
 * The data is packed, stored contiguously in memory.
 * memory layout:
 *	o base struct
 *	o rr_len size_t array
 *	o rr_ttl uint32_t array
 *	o rr_data uint8_t* array
 *	o rr_data rdata wireformats
 *	o rrsig_data rdata wireformat(s)
 *
 * Rdata is stored in wireformat. The dname is stored in wireformat.
 * TTLs are stored as absolute values (and could be expired).
 *
 * RRSIGs are stored in the arrays after the regular rrs.
 *
 * You need the packed_rrset_key to know dname, type, class of the
 * resource records in this RRset. (if signed the rrsig gives the type too).
 *
 * On the wire an RR is:
 *	name, type, class, ttl, rdlength, rdata.
 * So we need to send the following per RR:
 *	key.dname, ttl, rr_data[i].
 *	since key.dname ends with type and class.
 *	and rr_data starts with the rdlength.
 *	the ttl value to send changes due to time.
 */
struct packed_rrset_data {
	/** TTL (in seconds like time()) of the rrset.
	 * Same for all RRs see rfc2181(5.2).  */
	uint32_t ttl;
	/** number of rrs. */
	size_t count;
	/** number of rrsigs, if 0 no rrsigs */
	size_t rrsig_count;
	/** the trustworthiness of the rrset data */
	enum rrset_trust trust; 
	/** length of every rr's rdata, rr_len[i] is size of rr_data[i]. */
	size_t* rr_len;
	/** ttl of every rr. rr_ttl[i] ttl of rr i. */
	uint32_t *rr_ttl;
	/** 
	 * Array of pointers to every rr's rdata. 
	 * The rr_data[i] rdata is stored in uncompressed wireformat. 
	 * The first uint16_t of rr_data[i] is network format rdlength.
	 *
	 * rr_data[count] to rr_data[count+rrsig_count] contain the rrsig data.
	 */
	uint8_t** rr_data;
};

/**
 * An RRset can be represented using both key and data together.
 * Split into key and data structures to simplify implementation of
 * caching schemes.
 */
struct packed_rrset {
	/** domain name, type and class */
	struct packed_rrset_key* k;
	/** ttl, count and rdatas (and rrsig) */
	struct packed_rrset_data* d;
};

/**
 * Delete packed rrset key and data, not entered in hashtables yet.
 * Used during parsing.
 * @param pkey: rrset key structure with locks, key and data pointers.
 * @param alloc: where to return the unfree-able key structure.
 */
void ub_packed_rrset_parsedelete(struct ub_packed_rrset_key* pkey,
	struct alloc_cache* alloc);

/**
 * Calculate memory size of rrset entry. For hash table usage.
 * @param key: struct ub_packed_rrset_key*.
 * @param data: struct packed_rrset_data*.
 * @return size in bytes.
 */
size_t ub_rrset_sizefunc(void* key, void* data);

/**
 * compares two rrset keys.
 * @param k1: struct ub_packed_rrset_key*.
 * @param k2: struct ub_packed_rrset_key*.
 * @return 0 if equal.
 */
int ub_rrset_compare(void* k1, void* k2);

/**
 * compare two rrset data structures.
 * Compared rdata and rrsigdata, not the trust or ttl value.
 * @param d1: data to compare.
 * @param d2: data to compare.
 * @return 1 if equal.
 */
int rrsetdata_equal(struct packed_rrset_data* d1, struct packed_rrset_data* d2);

/**
 * Old key to be deleted. RRset keys are recycled via alloc.
 * @param key: struct ub_packed_rrset_key*.
 * @param userdata: alloc structure to use for recycling.
 */
void ub_rrset_key_delete(void* key, void* userdata);

/**
 * Old data to be deleted.
 * @param data: what to delete.
 * @param userdata: user data ptr.
 */
void rrset_data_delete(void* data, void* userdata);

#endif /* UTIL_DATA_PACKED_RRSET_H */
