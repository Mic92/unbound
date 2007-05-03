/*
 * util/data/msgparse.h - parse wireformat DNS messages.
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
 * Contains message parsing data structures.
 * These point back into the packet buffer.
 *
 * During parsing RRSIGS are put together with the rrsets they (claim to) sign.
 * This process works as follows:
 *	o if RRSIG follows the data rrset, it is added to the rrset rrsig list.
 *	o if no matching data rrset is found, the RRSIG becomes a new rrset.
 *	o If the data rrset later follows the RRSIG
 *		o See if the RRSIG rrset contains multiple types, and needs to
 *		  have the rrsig(s) for that data type split off.
 *		o Put the data rr as data type in the rrset and rrsig in list.
 *	o RRSIGs are allowed to move to a different section. The section of
 *	  the data item is used for the final rrset.
 *	o multiple signatures over an RRset are possible.
 *
 * For queries of qtype=RRSIG, some special handling is needed, to avoid
 * splitting the RRSIG in the answer section.
 *	o duplicate, not split, RRSIGs from the answer section, if qtype=RRSIG.
 *	o check for doubles in the rrsig list when adding an RRSIG to data,
 *	  so that a data rrset is signed by RRSIGs with different rdata.
 *	  when qtype=RRSIG.
 * This will move the RRSIG from the answer section to sign the data further
 * in the packet (if possible). If then after that, more RRSIGs are found
 * that sign the data as well, doubles are removed.
 */

#ifndef UTIL_DATA_MSGPARSE_H
#define UTIL_DATA_MSGPARSE_H
#include "util/storage/lruhash.h"
struct rrset_parse;
struct rr_parse;
struct region;

/** number of buckets in parse rrset hash table. Must be power of 2. */
#define PARSE_TABLE_SIZE 1024
/** Maximum TTL that is allowed. */
#define MAX_TTL	3600*24*365*10 /* ten years */
/** Negative cache time (for entries without any RRs.) */
#define NORR_TTL 5 /* seconds */

/**
 * Data stored in scratch pad memory during parsing.
 * Stores the data that will enter into the msgreply and packet result.
 */
struct msg_parse {
	/** id from message, network format. */
	uint16_t id;
	/** flags from message, host format. */
	uint16_t flags;
	/** count of RRs, host format */
	uint16_t qdcount;
	/** count of RRs, host format */
	uint16_t ancount;
	/** count of RRs, host format */
	uint16_t nscount;
	/** count of RRs, host format */
	uint16_t arcount;
	/** count of RRsets per section. */
	size_t an_rrsets;
	/** count of RRsets per section. */
	size_t ns_rrsets; 
	/** count of RRsets per section. */
	size_t ar_rrsets;
	/** total number of rrsets found. */
	size_t rrset_count;

	/** query dname (pointer to start location in packet, NULL if none */
	uint8_t* qname;
	/** length of query dname in octets, 0 if none */
	size_t qname_len;
	/** query type, host order. 0 if qdcount=0 */
	uint16_t qtype;
	/** query class, host order. 0 if qdcount=0 */
	uint16_t qclass;

	/**
	 * Hash table array used during parsing to lookup rrset types.
	 * Based on name, type, class.  Same hash value as in rrset cache.
	 */
	struct rrset_parse* hashtable[PARSE_TABLE_SIZE];
	
	/** linked list of rrsets that have been found (in order). */
	struct rrset_parse* rrset_first;
	/** last element of rrset list. */
	struct rrset_parse* rrset_last;
};

/**
 * Data stored for an rrset during parsing.
 */
struct rrset_parse {
	/** next in hash bucket */
	struct rrset_parse* rrset_bucket_next;
	/** next in list of all rrsets */
	struct rrset_parse* rrset_all_next;
	/** hash value of rrset */
	hashvalue_t hash;
	/** which section was it found in: one of
	 * LDNS_SECTION_ANSWER, LDNS_SECTION_AUTHORITY, LDNS_SECTION_ADDITIONAL
	 */
	ldns_pkt_section section;
	/** start of (possibly compressed) dname in packet */
	uint8_t* dname;
	/** length of the dname uncompressed wireformat */
	size_t dname_len;
	/** type, host order. */
	uint16_t type;
	/** class, network order. var name so that it is not a c++ keyword. */
	uint16_t rrset_class;
	/** the flags for the rrset, like for packedrrset */
	uint32_t flags;
	/** number of RRs in the rr list */
	size_t rr_count;
	/** sum of RR rdata sizes */
	size_t size;
	/** linked list of RRs in this rrset. */
	struct rr_parse* rr_first;
	/** last in list of RRs in this rrset. */
	struct rr_parse* rr_last;
	/** number of RRSIGs over this rrset. */
	size_t rrsig_count;
	/** linked list of RRsig RRs over this rrset. */
	struct rr_parse* rrsig_first;
	/** last in list of RRSIG RRs over this rrset. */
	struct rr_parse* rrsig_last;
};

/**
 * Data stored for an RR during parsing.
 */
struct rr_parse {
	/** 
	 * Pointer to the RR. Points to start of TTL value in the packet.
	 * Rdata length and rdata follow it.
	 * its dname, type and class are the same and stored for the rrset.
	 */
	uint8_t* ttl_data;
	/** the length of the rdata if allocated (with no dname compression)*/
	size_t size;
	/** next in list of RRs. */
	struct rr_parse* next;
};

/** Check if label length is first octet of a compression pointer, pass u8. */
#define LABEL_IS_PTR(x) ( ((x)&0xc0) == 0xc0 )
/** Calculate destination offset of a compression pointer. pass first and
 * second octets of the compression pointer. */
#define PTR_OFFSET(x, y) ( ((x)&0x3f)<<8 | (y) )
/** create a compression pointer to the given offset. */
#define PTR_CREATE(offset) ((uint16_t)(0xc000 | (offset)))

/** error codes, extended with EDNS, so > 15. */
#define EDNS_RCODE_BADVERS	16	/** bad EDNS version */
/** largest valid compression offset */
#define PTR_MAX_OFFSET 	0x3fff

/**
 * EDNS data storage
 * EDNS rdata is ignored.
 */
struct edns_data {
	/** if EDNS OPT record was present */
	int edns_present;
	/** Extended RCODE */
	uint8_t ext_rcode;
	/** The EDNS version number */
	uint8_t edns_version;
	/** the EDNS bits field from ttl (host order): Z */
	uint16_t bits;
	/** UDP reassembly size. */
	uint16_t udp_size;
};

/**
 * Obtain size in the packet of an rr type, that is before dname type.
 * Do TYPE_DNAME, and type STR, yourself. Gives size for most regular types.
 * @param rdf: the rdf type from the descriptor.
 * @return: size in octets. 0 on failure.
 */
size_t get_rdf_size(ldns_rdf_type rdf);

/**
 * Parse the packet.
 * @param pkt: packet, position at call must be at start of packet.
 *	at end position is after packet.
 * @param msg: where to store results.
 * @param region: how to alloc results.
 * @return: 0 if OK, or rcode on error.
 */
int parse_packet(ldns_buffer* pkt, struct msg_parse* msg, 
	struct region* region);

/**
 * After parsing the packet, extract EDNS data from packet.
 * If not present this is noted in the data structure.
 * If a parse error happens, an error code is returned.
 *
 * Quirks:
 *	o ignores OPT rdata.
 *	o ignores OPT owner name.
 *	o ignores extra OPT records, except the last one in the packet.
 *
 * @param msg: parsed message structure. Modified on exit, if EDNS was present
 * 	it is removed from the additional section.
 * @param edns: the edns data is stored here. Does not have to be initialised.
 * @return: 0 on success. or an RCODE on an error.
 *	RCODE formerr if OPT in wrong section, and so on.
 */
int parse_extract_edns(struct msg_parse* msg, struct edns_data* edns);

#endif /* UTIL_DATA_MSGPARSE_H */
