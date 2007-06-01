/*
 * iterator/iter_scrub.c - scrubbing, normalization, sanitization of DNS msgs.
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
 * This file has routine(s) for cleaning up incoming DNS messages from 
 * possible useless or malicious junk in it.
 */
#include "config.h"
#include "iterator/iter_scrub.h"
#include "util/log.h"
#include "util/net_help.h"
#include "util/region-allocator.h"
#include "util/data/msgparse.h"
#include "util/data/dname.h"
#include "util/data/msgreply.h"

/** RRset flag used during scrubbing. The RRset is OK. */
#define RRSET_SCRUB_OK	0x80

/** remove rrset, update loop variables */
static void
remove_rrset(const char* str, ldns_buffer* pkt, struct msg_parse* msg, 
	struct rrset_parse* prev, struct rrset_parse** rrset)
{
	if(verbosity >= VERB_ALGO 
		&& (*rrset)->dname_len <= LDNS_MAX_DOMAINLEN) {
		uint8_t buf[LDNS_MAX_DOMAINLEN+1];
		dname_pkt_copy(pkt, buf, (*rrset)->dname);
		log_nametypeclass(str, buf, 
			(*rrset)->type, ntohs((*rrset)->rrset_class));
	}
	if(prev)
		prev->rrset_all_next = (*rrset)->rrset_all_next;
	else	msg->rrset_first = (*rrset)->rrset_all_next;
	if(msg->rrset_last == *rrset)
		msg->rrset_last = prev;
	msg->rrset_count --;
	switch((*rrset)->section) {
		case LDNS_SECTION_ANSWER: msg->an_rrsets--; break;
		case LDNS_SECTION_AUTHORITY: msg->ns_rrsets--; break;
		case LDNS_SECTION_ADDITIONAL: msg->ar_rrsets--; break;
		default: log_assert(0);
	}
	msgparse_bucket_remove(msg, *rrset);
	*rrset = (*rrset)->rrset_all_next;
}

/** return true if rr type has additional names in it */
static int
has_additional(uint16_t t)
{
	switch(t) {
		case LDNS_RR_TYPE_MB:
		case LDNS_RR_TYPE_MD:
		case LDNS_RR_TYPE_MF:
		case LDNS_RR_TYPE_NS:
		case LDNS_RR_TYPE_MX:
		case LDNS_RR_TYPE_KX:
		case LDNS_RR_TYPE_SRV:
			return 1;
		case LDNS_RR_TYPE_NAPTR:
			/* TODO: NAPTR not supported, glue stripped off */
			return 0;
		default:
			return 0;
	}
	return 0;
}

/** get additional name from rrset RR, return false if no name present */
static int
get_additional_name(struct rrset_parse* rrset, struct rr_parse* rr, 
	uint8_t** nm, size_t* nmlen, ldns_buffer* pkt) 
{
	size_t offset = 0;
	size_t len, oldpos;
	switch(rrset->type) {
		case LDNS_RR_TYPE_MB:
		case LDNS_RR_TYPE_MD:
		case LDNS_RR_TYPE_MF:
		case LDNS_RR_TYPE_NS:
			offset = 0;
			break;
		case LDNS_RR_TYPE_MX:
		case LDNS_RR_TYPE_KX:
			offset = 2;
			break;
		case LDNS_RR_TYPE_SRV:
			offset = 6;
			break;
		case LDNS_RR_TYPE_NAPTR:
			/* TODO: NAPTR not supported, glue stripped off */
			return 0;
		default:
			return 0;
	}
	len = ldns_read_uint16(rr->ttl_data+sizeof(uint32_t));
	if(len < offset+1)
		return 0; /* rdata field too small */
	*nm = rr->ttl_data+sizeof(uint32_t)+sizeof(uint16_t)+offset;
	oldpos = ldns_buffer_position(pkt);
	ldns_buffer_set_position(pkt, (size_t)(*nm - ldns_buffer_begin(pkt)));
	*nmlen = pkt_dname_len(pkt);
	ldns_buffer_set_position(pkt, oldpos);
	if(*nmlen == 0)
		return 0;
	return 1;
}

/** Place mark on rrsets in additional section they are OK */
static void
mark_additional_rrset(ldns_buffer* pkt, struct msg_parse* msg, 
	struct rrset_parse* rrset)
{
	/* Mark A and AAAA for NS as appropriate additional section info. */
	uint8_t* nm = NULL;
	size_t nmlen = 0;
	struct rr_parse* rr;

	if(!has_additional(rrset->type))
		return;
	for(rr = rrset->rr_first; rr; rr = rr->next) {
		if(get_additional_name(rrset, rr, &nm, &nmlen, pkt)) {
			/* mark A */
			hashvalue_t h = pkt_hash_rrset(pkt, nm, LDNS_RR_TYPE_A, 
				rrset->rrset_class, 0);
			struct rrset_parse* r = msgparse_hashtable_lookup(
				msg, pkt, h, 0, nm, nmlen, 
				LDNS_RR_TYPE_A, rrset->rrset_class);
			if(r && r->section == LDNS_SECTION_ADDITIONAL) {
				r->flags |= RRSET_SCRUB_OK;
			}
			
			/* mark AAAA */
			h = pkt_hash_rrset(pkt, nm, LDNS_RR_TYPE_AAAA, 
				rrset->rrset_class, 0);
			r = msgparse_hashtable_lookup(msg, pkt, h, 0, nm, 
				nmlen, LDNS_RR_TYPE_AAAA, rrset->rrset_class);
			if(r && r->section == LDNS_SECTION_ADDITIONAL) {
				r->flags |= RRSET_SCRUB_OK;
			}
		}
	}
}

/** Get target name of a CNAME */
static int
parse_get_cname_target(struct rrset_parse* rrset, uint8_t** sname, 
	size_t* snamelen)
{
	if(rrset->rr_count != 1) {
		verbose(VERB_ALGO, "Found CNAME rrset with "
			"size > 1: %d", rrset->rr_count);
		return 0;
	}
	if(rrset->rr_first->size < sizeof(uint16_t)+1)
		return 0; /* CNAME rdata too small */
	*sname = rrset->rr_first->ttl_data + sizeof(uint32_t)
		+ sizeof(uint16_t); /* skip ttl, rdatalen */
	*snamelen = rrset->rr_first->size - sizeof(uint16_t);
	return 1;
}

/** Synthesize CNAME from DNAME, false if too long */
static int 
synth_cname(uint8_t* qname, size_t qnamelen, struct rrset_parse* dname_rrset, 
	uint8_t* alias, size_t* aliaslen, ldns_buffer* pkt)
{
	/* we already know that sname is a strict subdomain of DNAME owner */
	uint8_t* dtarg = NULL;
	size_t dtarglen;
	if(!parse_get_cname_target(dname_rrset, &dtarg, &dtarglen))
		return 0; 
	log_assert(qnamelen > dname_rrset->dname_len);
	/* DNAME from com. to net. with qname example.com. -> example.net. */
	/* so: \3com\0 to \3net\0 and qname \7example\3com\0 */
	*aliaslen = qnamelen + dtarglen - dname_rrset->dname_len;
	if(*aliaslen > LDNS_MAX_DOMAINLEN)
		return 0; /* should have been RCODE YXDOMAIN */
	/* decompress dnames into buffer, we know it fits */
	dname_pkt_copy(pkt, alias, qname);
	dname_pkt_copy(pkt, alias+(qnamelen-dname_rrset->dname_len), dtarg);
	return 1;
}

/** synthesize a CNAME rrset */
static struct rrset_parse*
synth_cname_rrset(uint8_t** sname, size_t* snamelen, uint8_t* alias, 
	size_t aliaslen, struct region* region, struct msg_parse* msg, 
	struct rrset_parse* rrset, struct rrset_parse* prev,
	struct rrset_parse* nx, ldns_buffer* pkt)
{
	struct rrset_parse* cn = (struct rrset_parse*)region_alloc(region,
		sizeof(struct rrset_parse));
	if(!cn)
		return NULL;
	memset(cn, 0, sizeof(*cn));
	cn->rr_first = (struct rr_parse*)region_alloc(region, 
		sizeof(struct rr_parse));
	if(!cn->rr_first)
		return NULL;
	cn->rr_last = cn->rr_first;
	/* CNAME from sname to alias */
	cn->dname = (uint8_t*)region_alloc_init(region, *sname, *snamelen);
	cn->dname_len = *snamelen;
	cn->type = LDNS_RR_TYPE_CNAME;
	cn->section = rrset->section;
	cn->rrset_class = rrset->rrset_class;
	cn->rr_count = 1;
	cn->size = sizeof(uint16_t) + aliaslen;
	cn->hash=pkt_hash_rrset(pkt, cn->dname, cn->type, cn->rrset_class, 0);
	/* allocate TTL + rdatalen + uncompressed dname */
	memset(cn->rr_first, 0, sizeof(struct rr_parse));
	cn->rr_first->ttl_data = (uint8_t*)region_alloc(region, 
		sizeof(uint32_t)+sizeof(uint16_t)+aliaslen);
	if(!cn->rr_first->ttl_data)
		return NULL;
	ldns_write_uint32(cn->rr_first->ttl_data, 0); /* TTL = 0 */
	ldns_write_uint16(cn->rr_first->ttl_data+4, aliaslen);
	memmove(cn->rr_first->ttl_data+6, alias, aliaslen);
	cn->rr_first->size = sizeof(uint16_t)+aliaslen;

	/* link it in */
	cn->rrset_all_next = nx;
	if(prev)
		prev->rrset_all_next = cn;
	else	msg->rrset_first = cn;
	if(nx == NULL)
		msg->rrset_last = cn;
	msg->rrset_count ++;
	msg->an_rrsets++;
	/* it is not inserted in the msg hashtable. */

	*sname = cn->rr_first->ttl_data + sizeof(uint32_t)+sizeof(uint16_t);
	*snamelen = aliaslen;
	return cn;
}

/** check if DNAME applies to a name */
static int
pkt_strict_sub(ldns_buffer* pkt, uint8_t* sname, uint8_t* dr)
{
	uint8_t buf[LDNS_MAX_DOMAINLEN+1];
	/* decompress DNAME name */
	dname_pkt_copy(pkt, buf, dr);
	return dname_strict_subdomain_c(sname, buf);
}

/** check subdomain with decompression */
static int
pkt_sub(ldns_buffer* pkt, uint8_t* sname, uint8_t* dr)
{
	uint8_t buf[LDNS_MAX_DOMAINLEN+1];
	/* decompress DNAME name */
	dname_pkt_copy(pkt, buf, dr);
	return dname_subdomain_c(sname, buf);
}

/**
 * This routine normalizes a response. This includes removing "irrelevant"
 * records from the answer and additional sections and (re)synthesizing
 * CNAMEs from DNAMEs, if present.
 *
 * @param pkt: packet.
 * @param msg: msg to normalize.
 * @param qinfo: original query.
 * @param region: where to allocate synthesized CNAMEs.
 * @return 0 on error.
 */
static int
scrub_normalize(ldns_buffer* pkt, struct msg_parse* msg, 
	struct query_info* qinfo, struct region* region)
{
	uint8_t* sname = qinfo->qname;
	size_t snamelen = qinfo->qname_len;
	struct rrset_parse* rrset, *prev;

	if(FLAGS_GET_RCODE(msg->flags) != LDNS_RCODE_NOERROR &&
		FLAGS_GET_RCODE(msg->flags) != LDNS_RCODE_NXDOMAIN)
		return 1;

	/* For the ANSWER section, remove all "irrelevant" records and add
	 * synthesized CNAMEs from DNAMEs
	 * This will strip out-of-order CNAMEs as well. */

	/* walk through the parse packet rrset list, keep track of previous
	 * for insert and delete ease, and examine every RRset */
	prev = NULL;
	rrset = msg->rrset_first;
	while(rrset && rrset->section == LDNS_SECTION_ANSWER) {
		if(rrset->type == LDNS_RR_TYPE_DNAME && 
			pkt_strict_sub(pkt, sname, rrset->dname)) {
			/* check if next rrset is correct CNAME. else,
			 * synthesize a CNAME */
			struct rrset_parse* nx = rrset->rrset_all_next;
			uint8_t alias[LDNS_MAX_DOMAINLEN+1];
			size_t aliaslen = 0;
			if(rrset->rr_count != 1) {
				verbose(VERB_ALGO, "Found DNAME rrset with "
					"size > 1: %d", rrset->rr_count);
				return 0;
			}
			if(!synth_cname(sname, snamelen, rrset, alias, 
				&aliaslen, pkt)) {
				verbose(VERB_ALGO, "synthesized CNAME "
					"too long");
				return 0;
			}
			if(nx && nx->type == LDNS_RR_TYPE_CNAME && 
			   dname_pkt_compare(pkt, sname, nx->dname) == 0) {
				/* check next cname */
				uint8_t* t = NULL;
				size_t tlen = 0;
				if(!parse_get_cname_target(rrset, &t, &tlen))
					return 0;
				if(dname_pkt_compare(pkt, alias, t) == 0) {
					/* it's OK and better capitalized */
					prev = rrset;
					rrset = nx;
					continue;
				}
				/* synth ourselves */
			}
			/* synth a CNAME rrset */
			prev = synth_cname_rrset(&sname, &snamelen, alias, 
				aliaslen, region, msg, rrset, prev, nx, pkt);
			if(!prev) {
				log_err("out of memory synthesizing CNAME");
				return 0;
			}
			/* FIXME: resolve the conflict between synthesized 
			 * CNAME ttls and the cache. */
			rrset = nx;
			continue;

		}

		/* The only records in the ANSWER section not allowed to */
		if(dname_pkt_compare(pkt, sname, rrset->dname) != 0) {
			remove_rrset("normalize: removing irrelevant RRset:", 
				pkt, msg, prev, &rrset);
			continue;
		}

		/* Follow the CNAME chain. */
		if(rrset->type == LDNS_RR_TYPE_CNAME) {
			if(!parse_get_cname_target(rrset, &sname, &snamelen))
				return 0;
			prev = rrset;
			rrset = rrset->rrset_all_next;
			continue;
		}

		/* Otherwise, make sure that the RRset matches the qtype. */
		if(qinfo->qtype != LDNS_RR_TYPE_ANY && 
			qinfo->qtype != rrset->type) {
			remove_rrset("normalize: removing irrelevant RRset:", 
				pkt, msg, prev, &rrset);
			continue;
		}

		/* Mark the additional names from relevant rrset as OK. */
		mark_additional_rrset(pkt, msg, rrset);
		
		prev = rrset;
		rrset = rrset->rrset_all_next;
	}

	/* Mark additional names from AUTHORITY */
	while(rrset && rrset->section == LDNS_SECTION_AUTHORITY) {
		mark_additional_rrset(pkt, msg, rrset);
		prev = rrset;
		rrset = rrset->rrset_all_next;
	}

	/* For each record in the additional section, remove it if it is an
	 * address record and not in the collection of additional names 
	 * found in ANSWER and AUTHORITY. */
	/* These records have not been marked OK previously */
	while(rrset && rrset->section == LDNS_SECTION_ADDITIONAL) {
		/* FIXME: what about other types? */
		if(rrset->type==LDNS_RR_TYPE_A || 
			rrset->type==LDNS_RR_TYPE_AAAA) 
		{
			if((rrset->flags & RRSET_SCRUB_OK)) {
				/* remove flag to clean up flags variable */
				rrset->flags &= ~RRSET_SCRUB_OK;
			} else {
				remove_rrset("normalize: removing irrelevant "
					"RRset:", pkt, msg, prev, &rrset);
				continue;
			}
		}
		prev = rrset;
		rrset = rrset->rrset_all_next;
	}
	
	return 1;
}

/**
 * Given a response event, remove suspect RRsets from the response.
 * "Suspect" rrsets are potentially poison. Note that this routine expects
 * the response to be in a "normalized" state -- that is, all "irrelevant"
 * RRsets have already been removed, CNAMEs are in order, etc.
 *
 * @param pkt: packet.
 * @param msg: msg to normalize.
 * @param zonename: name of server zone.
 * @return 0 on error.
 */
static int
scrub_sanitize(ldns_buffer* pkt, struct msg_parse* msg, uint8_t* zonename)
{
	struct rrset_parse* rrset, *prev;
	prev = NULL;
	rrset = msg->rrset_first;

	/* At this point, we brutally remove ALL rrsets that aren't 
	 * children of the originating zone. The idea here is that, 
	 * as far as we know, the server that we contacted is ONLY 
	 * authoritative for the originating zone. It, of course, MAY 
	 * be authoriative for any other zones, and of course, MAY 
	 * NOT be authoritative for some subdomains of the originating 
	 * zone. */
	while(rrset) {
		
		/* skip DNAME records -- they will always be followed by a 
		 * synthesized CNAME, which will be relevant.
		 * FIXME: should this do something differently with DNAME 
		 * rrsets NOT in Section.ANSWER? */
		/* But since DNAME records are also subdomains of the zone,
		 * same check can be used */

		if(!pkt_sub(pkt, rrset->dname, zonename)) {
			remove_rrset("sanitize: removing potential poison "
				"RRset:", pkt, msg, prev, &rrset);
			continue;
		}
		prev = rrset;
		rrset = rrset->rrset_all_next;
	}
	return 1;
}

int 
scrub_message(ldns_buffer* pkt, struct msg_parse* msg, 
	struct query_info* qinfo, uint8_t* zonename, struct region* region)
{
	/* things to check:
	 * if qdcount > 0 : qinfo.
	 * from normalize() from NSclient.
	 * from sanitize() from iterator.
	 */
	/* basic sanity checks */
	if(msg->qdcount > 1)
		return 0;
	if( !(msg->flags&BIT_QR) )
		return 0;
	
	/* if a query is echoed back, make sure it is correct. Otherwise,
	 * this may be not a reply to our query. */
	if(msg->qdcount == 1) {
		if(dname_pkt_compare(pkt, msg->qname, qinfo->qname) != 0)
			return 0;
		if(msg->qtype != qinfo->qtype || msg->qclass != qinfo->qclass)
			return 0;
	}

	/* normalize the response, this cleans up the additional.  */
	if(!scrub_normalize(pkt, msg, qinfo, region))
		return 0;
	/* delete all out-of-zone information */
	if(!scrub_sanitize(pkt, msg, zonename))
		return 0;
	return 1;
}
