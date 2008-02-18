/*
 * util/data/dname.h - domain name handling
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
 * This file contains domain name handling functions.
 */

#include "config.h"
#include "util/data/dname.h"
#include "util/data/msgparse.h"
#include "util/log.h"
#include "util/storage/lookup3.h"

/* determine length of a dname in buffer, no compression pointers allowed */
size_t
query_dname_len(ldns_buffer* query)
{
	size_t len = 0;
	size_t labellen;
	while(1) {
		if(ldns_buffer_remaining(query) < 1)
			return 0; /* parse error, need label len */
		labellen = ldns_buffer_read_u8(query);
		if(labellen&0xc0)
			return 0; /* no compression allowed in queries */
		len += labellen + 1;
		if(len > LDNS_MAX_DOMAINLEN)
			return 0; /* too long */
		if(labellen == 0)
			return len;
		if(ldns_buffer_remaining(query) < labellen)
			return 0; /* parse error, need content */
		ldns_buffer_skip(query, (ssize_t)labellen);
	}
}

size_t 
dname_valid(uint8_t* dname, size_t maxlen)
{
	size_t len = 0;
	size_t labellen;
	labellen = *dname++;
	while(labellen) {
		if(labellen&0xc0)
			return 0; /* no compression ptrs allowed */
		len += labellen + 1;
		if(len > LDNS_MAX_DOMAINLEN)
			return 0; /* too long */
		if(len > maxlen)
			return 0; /* does not fit in memory allocation */
		dname += labellen;
		labellen = *dname++;
	}
	len += 1;
	if(len > maxlen)
		return 0; /* does not fit in memory allocation */
	return len;
}

/** compare uncompressed, noncanonical, registers are hints for speed */
int 
query_dname_compare(register uint8_t* d1, register uint8_t* d2)
{
	register uint8_t lab1, lab2;
	log_assert(d1 && d2);
	lab1 = *d1++;
	lab2 = *d2++;
	while( lab1 != 0 || lab2 != 0 ) {
		/* compare label length */
		/* if one dname ends, it has labellength 0 */
		if(lab1 != lab2) {
			if(lab1 < lab2)
				return -1;
			return 1;
		}
		log_assert(lab1 == lab2 && lab1 != 0);
		/* compare lowercased labels. */
		while(lab1--) {
			/* compare bytes first for speed */
			if(*d1 != *d2 && 
				tolower((int)*d1) != tolower((int)*d2)) {
				if(tolower((int)*d1) < tolower((int)*d2))
					return -1;
				return 1;
			}
			d1++;
			d2++;
		}
		/* next pair of labels. */
		lab1 = *d1++;
		lab2 = *d2++;
	}
	return 0;
}

void 
query_dname_tolower(uint8_t* dname)
{
	/* the dname is stored uncompressed */
	uint8_t labellen;
	labellen = *dname;
	while(labellen) {
		dname++;
		while(labellen--) {
			*dname = (uint8_t)tolower((int)*dname);
			dname++;
		}
		labellen = *dname;
	}
}

/** maximum compression pointer position pointed to */
#define MAX_COMPRESS_POS 16384
/** size of bitmap for loop detection */
#define LOOP_BITMAP_SIZE (MAX_COMPRESS_POS/8)

/** check bit in bitmap for loop detection, then set it for next check */
static uint8_t 
loopcheck(uint8_t loop[], size_t pos)
{
	const uint8_t bits[8] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
	uint8_t ret;
	log_assert(pos < MAX_COMPRESS_POS);
	ret = loop[ pos / 8 ] & bits[ pos & 0x7 ];
	loop[ pos / 8 ] |= bits[ pos & 0x7 ];	
	return ret;
}


size_t
pkt_dname_len(ldns_buffer* pkt)
{
	size_t len = 0;
	uint8_t loop[LOOP_BITMAP_SIZE]; /* loopcheck array. */
	uint8_t labellen;
	size_t endpos = 0;

	/* read dname and determine length */
	/* check compression pointers, loops, out of bounds */
	memset(loop, 0, sizeof(loop));

	while(1) {
		/* read next label */
		if(ldns_buffer_remaining(pkt) < 1)
			return 0;
		labellen = ldns_buffer_read_u8(pkt);
		if(LABEL_IS_PTR(labellen)) {
			/* compression ptr */
			uint16_t ptr;
			if(ldns_buffer_remaining(pkt) < 1)
				return 0;
			ptr = PTR_OFFSET(labellen, ldns_buffer_read_u8(pkt));
			if(loopcheck(loop, ptr))
				return 0; /* loop! */
			if(ldns_buffer_limit(pkt) <= ptr)
				return 0; /* out of bounds! */
			if(!endpos)
				endpos = ldns_buffer_position(pkt);
			ldns_buffer_set_position(pkt, ptr);
		} else {
			/* label contents */
			if(labellen > 0x3f)
				return 0; /* label too long */
			len += 1 + labellen;
			if(len > LDNS_MAX_DOMAINLEN)
				return 0;
			if(labellen == 0) {
				/* end of dname */
				break;
			}
			if(ldns_buffer_remaining(pkt) < labellen)
				return 0;
			ldns_buffer_skip(pkt, (ssize_t)labellen);
		}
	}
	if(endpos)
		ldns_buffer_set_position(pkt, endpos);

	return len;
}

int 
dname_pkt_compare(ldns_buffer* pkt, uint8_t* d1, uint8_t* d2)
{
	uint8_t len1, len2;
	log_assert(pkt && d1 && d2);
	len1 = *d1++;
	len2 = *d2++;
	while( len1 != 0 || len2 != 0 ) {
		/* resolve ptrs */
		if(LABEL_IS_PTR(len1)) {
			d1 = ldns_buffer_at(pkt, PTR_OFFSET(len1, *d1));
			len1 = *d1++;
			continue;
		}
		if(LABEL_IS_PTR(len2)) {
			d2 = ldns_buffer_at(pkt, PTR_OFFSET(len2, *d2));
			len2 = *d2++;
			continue;
		}
		/* check label length */
		log_assert(len1 <= LDNS_MAX_LABELLEN);
		log_assert(len2 <= LDNS_MAX_LABELLEN);
		if(len1 != len2) {
			if(len1 < len2) return -1;
			return 1;
		}
		log_assert(len1 == len2 && len1 != 0);
		/* compare labels */
		while(len1--) {
			if(tolower((int)*d1++) != tolower((int)*d2++)) {
				if(tolower((int)d1[-1]) < tolower((int)d2[-1]))
					return -1;
				return 1;
			}
		}
		len1 = *d1++;
		len2 = *d2++;
	}
	return 0;
}

hashvalue_t 
dname_query_hash(uint8_t* dname, hashvalue_t h)
{
	uint8_t labuf[LDNS_MAX_LABELLEN+1];
	uint8_t lablen;
	int i;

	/* preserve case of query, make hash label by label */
	lablen = *dname++;
	while(lablen) {
		log_assert(lablen <= LDNS_MAX_LABELLEN);
		labuf[0] = lablen;
		i=0;
		while(lablen--)
			labuf[++i] = (uint8_t)tolower((int)*dname++);
		h = hashlittle(labuf, labuf[0] + 1, h);
		lablen = *dname++;
	}

	return h;
}

hashvalue_t 
dname_pkt_hash(ldns_buffer* pkt, uint8_t* dname, hashvalue_t h)
{
	uint8_t labuf[LDNS_MAX_LABELLEN+1];
	uint8_t lablen;
	int i;

	/* preserve case of query, make hash label by label */
	lablen = *dname++;
	while(lablen) {
		if(LABEL_IS_PTR(lablen)) {
			/* follow pointer */
			dname = ldns_buffer_at(pkt, PTR_OFFSET(lablen, *dname));
			lablen = *dname++;
			continue;
		}
		log_assert(lablen <= LDNS_MAX_LABELLEN);
		labuf[0] = lablen;
		i=0;
		while(lablen--)
			labuf[++i] = (uint8_t)tolower((int)*dname++);
		h = hashlittle(labuf, labuf[0] + 1, h);
		lablen = *dname++;
	}

	return h;
}

void dname_pkt_copy(ldns_buffer* pkt, uint8_t* to, uint8_t* dname)
{
	/* copy over the dname and decompress it at the same time */
	size_t len = 0;
	uint8_t lablen;
	lablen = *dname++;
	while(lablen) {
		if(LABEL_IS_PTR(lablen)) {
			/* follow pointer */
			dname = ldns_buffer_at(pkt, PTR_OFFSET(lablen, *dname));
			lablen = *dname++;
			continue;
		}
		log_assert(lablen <= LDNS_MAX_LABELLEN);
		len += (size_t)lablen+1;
		if(len >= LDNS_MAX_DOMAINLEN) {
			*to = 0; /* end the result prematurely */
			log_err("bad dname in dname_pkt_copy");
			return;
		}
		*to++ = lablen;
		memmove(to, dname, lablen);
		dname += lablen;
		to += lablen;
		lablen = *dname++;
	}
	/* copy last \0 */
	*to = 0;
}

void dname_print(FILE* out, ldns_buffer* pkt, uint8_t* dname)
{
	uint8_t lablen;
	if(!out) out = stdout;
	if(!dname) return;

	lablen = *dname++;
	if(!lablen) 
		fputc('.', out);
	while(lablen) {
		if(LABEL_IS_PTR(lablen)) {
			/* follow pointer */
			if(!pkt) {
				fputs("??compressionptr??", out);
				return;
			}
			dname = ldns_buffer_at(pkt, PTR_OFFSET(lablen, *dname));
			lablen = *dname++;
			continue;
		}
		if(lablen > LDNS_MAX_LABELLEN) {
			fputs("??extendedlabel??", out);
			return;
		}
		while(lablen--)
			fputc((int)*dname++, out);
		fputc('.', out);
		lablen = *dname++;
	}
}

int 
dname_count_labels(uint8_t* dname)
{
	uint8_t lablen;
	int labs = 1;

	lablen = *dname++;
	while(lablen) {
		labs++;
		dname += lablen;
		lablen = *dname++;
	}
	return labs;
}

int 
dname_count_size_labels(uint8_t* dname, size_t* size)
{	
	uint8_t lablen;
	int labs = 1;
	*size = 1;

	lablen = *dname++;
	while(lablen) {
		labs++;
		*size += lablen+1;
		dname += lablen;
		lablen = *dname++;
	}
	return labs;

}

/**
 * Compare labels in memory, lowercase while comparing.
 * @param p1: label 1
 * @param p2: label 2
 * @param len: number of bytes to compare.
 * @return: 0, -1, +1 comparison result.
 */
static int
memlowercmp(uint8_t* p1, uint8_t* p2, uint8_t len)
{
	while(len--) {
		if(*p1 != *p2 && tolower((int)*p1) != tolower((int)*p2)) {
			if(tolower((int)*p1) < tolower((int)*p2))
				return -1;
			return 1;
		}
		p1++;
		p2++;
	}
	return 0;
}


int 
dname_lab_cmp(uint8_t* d1, int labs1, uint8_t* d2, int labs2, int* mlabs)
{
	uint8_t len1, len2;
	int atlabel = labs1;
	int lastmlabs;
	int lastdiff = 0;
	int c;
	/* first skip so that we compare same label. */
	if(labs1 > labs2) {
		while(atlabel > labs2) {
			len1 = *d1++;
			d1 += len1;
			atlabel--;
		}
		log_assert(atlabel == labs2);
	} else if(labs1 < labs2) {
		atlabel = labs2;
		while(atlabel > labs1) {
			len2 = *d2++;
			d2 += len2;
			atlabel--;
		}
		log_assert(atlabel == labs1);
	}
	lastmlabs = atlabel+1;
	/* now at same label in d1 and d2, atlabel */
	/* www.example.com.                  */
	/* 4   3       2  1   atlabel number */
	/* repeat until at root label (which is always the same) */
	while(atlabel > 1) {
		len1 = *d1++;
		len2 = *d2++;
		if(len1 != len2) {
			log_assert(len1 != 0 && len2 != 0);
			if(len1<len2)
				lastdiff = -1;
			else	lastdiff = 1;
			lastmlabs = atlabel;
		} else if((c=memlowercmp(d1, d2, len1)) != 0) { 
			lastdiff = c;
			lastmlabs = atlabel;
		}

		d1 += len1;
		d2 += len2;
		atlabel--;
	}
	/* last difference atlabel number, so number of labels matching,
	 * at the right side, is one less. */
	*mlabs = lastmlabs-1;
	if(lastdiff == 0) {
		/* all labels compared were equal, check if one has more
		 * labels, so that example.com. > com. */
		if(labs1 > labs2)
			return 1;
		else if(labs1 < labs2)
			return -1;
	}
	return lastdiff;
}

int 
dname_buffer_write(ldns_buffer* pkt, uint8_t* dname)
{
	uint8_t lablen;

	if(ldns_buffer_remaining(pkt) < 1)
		return 0;
	lablen = *dname++;
	ldns_buffer_write_u8(pkt, lablen);
	while(lablen) {
		if(ldns_buffer_remaining(pkt) < (size_t)lablen+1)
			return 0;
		ldns_buffer_write(pkt, dname, lablen);
		dname += lablen;
		lablen = *dname++;
		ldns_buffer_write_u8(pkt, lablen);
	}
	return 1;
}

void dname_str(uint8_t* dname, char* str)
{
	size_t len = 0;
	uint8_t lablen = 0;
	char* s = str;
	if(!dname || !*dname) {
		*s++ = '.';
		*s = 0;
		return;
	}
	lablen = *dname++;
	while(lablen) {
		if(lablen > LDNS_MAX_LABELLEN) {
			*s++ = '#';
			*s = 0;
			return;
		}
		len += lablen+1;
		if(len >= LDNS_MAX_DOMAINLEN-1) {
			*s++ = '&';
			*s = 0;
			return;
		}
		while(lablen--) {
			if(isalnum((int)*dname) 
				|| *dname == '-' || *dname == '_' 
				|| *dname == '*')
				*s++ = *(char*)dname++;
			else	{
				*s++ = '?';
				dname++;
			}
		}
		*s++ = '.';
		lablen = *dname++;
	}
	*s = 0;
}

int 
dname_strict_subdomain(uint8_t* d1, int labs1, uint8_t* d2, int labs2)
{
	int m;
	/* check subdomain: d1: www.example.com. and d2: example.com. */
	if(labs2 >= labs1) 
		return 0;
	if(dname_lab_cmp(d1, labs1, d2, labs2, &m) > 0) {
		/* subdomain if all labels match */
		return (m == labs2);
	}
	return 0;
}

int 
dname_strict_subdomain_c(uint8_t* d1, uint8_t* d2)
{
	return dname_strict_subdomain(d1, dname_count_labels(d1), d2,
		dname_count_labels(d2));
}

int 
dname_subdomain_c(uint8_t* d1, uint8_t* d2)
{
	int m;
	/* check subdomain: d1: www.example.com. and d2: example.com. */
	/*  	or 	    d1: example.com. and d2: example.com. */
	int labs1 = dname_count_labels(d1);
	int labs2 = dname_count_labels(d2);
	if(labs2 > labs1) 
		return 0;
	if(dname_lab_cmp(d1, labs1, d2, labs2, &m) < 0) {
		/* must have been example.com , www.example.com - wrong */
		/* or otherwise different dnames */
		return 0;
	}
	return (m == labs2);
}

int 
dname_is_root(uint8_t* dname)
{
	uint8_t len;
	log_assert(dname);
	len = dname[0];
	log_assert(!LABEL_IS_PTR(len));
	return (len == 0);
}

void 
dname_remove_label(uint8_t** dname, size_t* len)
{
	size_t lablen;
	log_assert(dname && *dname && len);
	lablen = (*dname)[0];
	log_assert(!LABEL_IS_PTR(lablen));
	log_assert(*len > lablen);
	if(lablen == 0)
		return; /* do not modify root label */
	*len -= lablen+1;
	*dname += lablen+1;
}

void 
dname_remove_labels(uint8_t** dname, size_t* len, int n)
{
	int i;
	for(i=0; i<n; i++)
		dname_remove_label(dname, len);
}

int 
dname_signame_label_count(uint8_t* dname)
{
	uint8_t lablen;
	int count = 0;
	if(!*dname)
		return 0;
	if(dname[0] == 1 && dname[1] == '*')
		dname += 2;
	lablen = dname[0];
	while(lablen) {
		count++;
		dname += lablen;
		dname += 1;
		lablen = dname[0];
	}
	return count;
}

int 
dname_is_wild(uint8_t* dname)
{
	return (dname[0] == 1 && dname[1] == '*');
}

/**
 * Compare labels in memory, lowercase while comparing.
 * Returns canonical order for labels. If all is equal, the
 * shortest is first.
 *
 * @param p1: label 1
 * @param len1: length of label 1.
 * @param p2: label 2
 * @param len2: length of label 2.
 * @return: 0, -1, +1 comparison result.
 */
static int
memcanoncmp(uint8_t* p1, uint8_t len1, uint8_t* p2, uint8_t len2)
{
	uint8_t min = (len1<len2)?len1:len2;
	int c = memlowercmp(p1, p2, min);
	if(c != 0)
		return c;
	/* equal, see who is shortest */
	if(len1 < len2)
		return -1;
	if(len1 > len2)
		return 1;
	return 0;
}


int 
dname_canon_lab_cmp(uint8_t* d1, int labs1, uint8_t* d2, int labs2, int* mlabs)
{
	/* like dname_lab_cmp, but with different label comparison,
	 * empty character sorts before \000.
	 * So   ylyly is before z. */
	uint8_t len1, len2;
	int atlabel = labs1;
	int lastmlabs;
	int lastdiff = 0;
	int c;
	/* first skip so that we compare same label. */
	if(labs1 > labs2) {
		while(atlabel > labs2) {
			len1 = *d1++;
			d1 += len1;
			atlabel--;
		}
		log_assert(atlabel == labs2);
	} else if(labs1 < labs2) {
		atlabel = labs2;
		while(atlabel > labs1) {
			len2 = *d2++;
			d2 += len2;
			atlabel--;
		}
		log_assert(atlabel == labs1);
	}
	lastmlabs = atlabel+1;
	/* now at same label in d1 and d2, atlabel */
	/* www.example.com.                  */
	/* 4   3       2  1   atlabel number */
	/* repeat until at root label (which is always the same) */
	while(atlabel > 1) {
		len1 = *d1++;
		len2 = *d2++;

		if((c=memcanoncmp(d1, len1, d2, len2)) != 0) {
			if(c<0)
				lastdiff = -1;
			else	lastdiff = 1;
			lastmlabs = atlabel;
		}

		d1 += len1;
		d2 += len2;
		atlabel--;
	}
	/* last difference atlabel number, so number of labels matching,
	 * at the right side, is one less. */
	*mlabs = lastmlabs-1;
	if(lastdiff == 0) {
		/* all labels compared were equal, check if one has more
		 * labels, so that example.com. > com. */
		if(labs1 > labs2)
			return 1;
		else if(labs1 < labs2)
			return -1;
	}
	return lastdiff;
}

int
dname_canonical_compare(uint8_t* d1, uint8_t* d2)
{
	int labs1, labs2, m;
	labs1 = dname_count_labels(d1);
	labs2 = dname_count_labels(d2);
	return dname_canon_lab_cmp(d1, labs1, d2, labs2, &m);
}

uint8_t* dname_get_shared_topdomain(uint8_t* d1, uint8_t* d2)
{
	int labs1, labs2, m;
	size_t len = LDNS_MAX_DOMAINLEN;
	labs1 = dname_count_labels(d1);
	labs2 = dname_count_labels(d2);
	(void)dname_lab_cmp(d1, labs1, d2, labs2, &m);
	dname_remove_labels(&d1, &len, labs1-m);
	return d1;
}
