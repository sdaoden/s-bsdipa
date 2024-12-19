/*@ s-bsdipa-io: I/O (compression) layer for s-bsdipa-lib.
 *@ Define s_BSDIPA_IO to s_BSDIPA_IO_(NONE|ZLIB) and include this header.
 *@ It then provides the according s_BSDIPA_IO_NAME preprocessor literal
 *@ and s_bsdipa_io_{read,write}(), which (are) fe(e)d data to/from hooks.
 *@ The functions have s_BSDIPA_IO_LINKAGE storage, or static if not defined.
 *@ There may be additional static helper functions.
 *@
 *@ Notes:
 *@ - it is up to the user to provide according linker flags, like -lz!
 *@ - this is not a step-by-step filter: a complete s_bsdipa_diff() result
 *@   is serialized, or serialized data is turned into a complete data set
 *@   that then can be fed into s_bsdipa_patch().
 *@   (A custom I/O (compression) layer may be less memory hungry.)
 *@ - s_BSDIPA_IO == s_BSDIPA_IO_ZLIB:
 *@   -- s_BSDIPA_IO_ZLIB_LEVEL may be defined as the "level" argument of
 *@      zlib's deflateInit() (default is 9).
 *@
 *@ Remarks:
 *@ - code requires ISO STD C99 (for now).
 *
 * Copyright (c) 2024 Steffen Nurpmeso <steffen@sdaoden.eu>.
 *
 * SPDX-License-Identifier: ISC
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef s_BSDIPA_IO_H
#define s_BSDIPA_IO_H

#include <s-bsdipa-lib.h>

/* Compression types (preprocessor so sources can adapt) */
#define s_BSDIPA_IO_NONE 0
#define s_BSDIPA_IO_ZLIB 1

#ifndef s_BSDIPA_IO_LINKAGE
# define s_BSDIPA_IO_LINKAGE static
#endif

/* I/O write hook. If len<=0, dat is to be ignored: if 0 output should be flushed, if it is -1 this is EOF
 * and last call (flushing may be desirable then, too).  Any return other than BSDIPA_OK causes stop. */
typedef enum s_bsdipa_state (*s_bsdipa_io_write_ptf)(void *user_cookie, uint8_t const *dat, s_bsdipa_off_t len);
s_BSDIPA_IO_LINKAGE enum s_bsdipa_state s_bsdipa_io_write(struct s_bsdipa_diff_ctx const *dcp,
		s_bsdipa_io_write_ptf hook, void *user_cookie);

/* I/O read hook.  It is assumed that pcp->pc_patch_dat and .pc_patch_len represent the entire (constant) patch data.
 * Output will be allocated via .pc_mem and stored in .pc_restored_dat and .pc_restored_len as a continuous chunk.
 * On error that memory, if any, will be freed, and .pc_restored_dat will be NULL.
 * On success .pc_header is filled in, and .pc_ctrl_dat, .pc_diff_dat and .pc_extra_dat are set accordingly;
 * it is up to the user to release .pc_patch_dat and set it to NULL before calling s_bsdipa_patch().
 * (Of course .pc_restored_dat will be overwritten by s_bsdipa_patch()..). */
s_BSDIPA_IO_LINKAGE enum s_bsdipa_state s_bsdipa_io_read(struct s_bsdipa_patch_ctx *pcp);

#undef s_BSDIPA_IO_NAME
#if !defined s_BSDIPA_IO || s_BSDIPA_IO == s_BSDIPA_IO_NONE /* {{{ */
# undef s_BSDIPA_IO
# define s_BSDIPA_IO s_BSDIPA_IO_NONE
# define s_BSDIPA_IO_NAME "NONE"

# include <assert.h>

s_BSDIPA_IO_LINKAGE enum s_bsdipa_state
s_bsdipa_io_write(struct s_bsdipa_diff_ctx const *dcp, s_bsdipa_io_write_ptf hook, void *user_cookie){
	struct s_bsdipa_ctrl_chunk *ccp;
	enum s_bsdipa_state rv;

	if((rv = (*hook)(user_cookie, dcp->dc_header, sizeof(dcp->dc_header))) != s_BSDIPA_OK)
		goto jleave;

	s_BSDIPA_DIFF_CTX_FOREACH_CTRL(dcp, ccp){
		if((rv = (*hook)(user_cookie, ccp->cc_dat, ccp->cc_len)) != s_BSDIPA_OK)
			goto jleave;
	}

	if(dcp->dc_diff_len > 0 && (rv = (*hook)(user_cookie, dcp->dc_diff_dat, dcp->dc_diff_len)) != s_BSDIPA_OK)
		goto jleave;

	if(dcp->dc_extra_len > 0 && (rv = (*hook)(user_cookie, dcp->dc_extra_dat, dcp->dc_extra_len)) != s_BSDIPA_OK)
		goto jleave;

	rv = (*hook)(user_cookie, NULL, -1);
jleave:
	return rv;
}

s_BSDIPA_IO_LINKAGE enum s_bsdipa_state
s_bsdipa_io_read(struct s_bsdipa_patch_ctx *pcp){
	enum s_bsdipa_state rv;
	uint64_t pl;
	uint8_t const *pd;
	uint8_t *rd;

	rd = NULL;
	pd = pcp->pc_patch_dat;
	pl = pcp->pc_patch_len;

	if(pl < sizeof(struct s_bsdipa_header)){
		rv = s_BSDIPA_INVAL;
		goto jleave;
	}
	rv = s_bsdipa_patch_parse_header(&pcp->pc_header, pd);
	if(rv != s_BSDIPA_OK)
		goto jleave;

	pl -= sizeof(pcp->pc_header);
	pd += sizeof(pcp->pc_header);

	/* Not truly right, the latter at least, but good enough for now */
	if(pl > s_BSDIPA_OFF_MAX - 1 || pl != (size_t)pl){
		rv = s_BSDIPA_FBIG;
		goto jleave;
	}

	if(pl != (uint64_t)(pcp->pc_header.h_ctrl_len + pcp->pc_header.h_diff_len + pcp->pc_header.h_extra_len)){
		rv = s_BSDIPA_INVAL;
		goto jleave;
	}

	rd = (pcp->pc_mem.mc_alloc != NULL) ? (*pcp->pc_mem.mc_alloc)((size_t)pl)
			: (*pcp->pc_mem.mc_custom_alloc)(pcp->pc_mem.mc_custom_cookie, (size_t)pl);
	if(rd == NULL){
		rv = s_BSDIPA_NOMEM;
		goto jleave;
	}

	memcpy(rd, pd, (size_t)pl);

	rv = s_BSDIPA_OK;
jleave:
	if(rv == s_BSDIPA_OK){
		pcp->pc_restored_dat = rd;
		pcp->pc_restored_len = (s_bsdipa_off_t)pl;
	}else{
		if(rd != NULL)
			(pcp->pc_mem.mc_alloc != NULL) ? (*pcp->pc_mem.mc_free)(rd)
				: (*pcp->pc_mem.mc_custom_free)(pcp->pc_mem.mc_custom_cookie, rd);
		pcp->pc_restored_dat = NULL;
	}

	return rv;
}
/* }}} */

#elif s_BSDIPA_IO == s_BSDIPA_IO_ZLIB /* _IO_NONE {{{ */
# define s_BSDIPA_IO_NAME "ZLIB"

# include <assert.h>

# include <zlib.h>

# ifndef s_BSDIPA_IO_ZLIB_LEVEL
#  define s_BSDIPA_IO_ZLIB_LEVEL 9
# endif

 /* For testing purposes */
# define s__BSDIPA_IO_LIMIT (INT32_MAX - 1)

static voidpf s__bsdipa_io_alloc(voidpf my_cookie, uInt no, uInt size);
static void s__bsdipa_io_free(voidpf my_cookie, voidpf dat);

s_BSDIPA_IO_LINKAGE enum s_bsdipa_state
s_bsdipa_io_write(struct s_bsdipa_diff_ctx const *dcp, s_bsdipa_io_write_ptf hook, void *user_cookie){
	z_stream zs;
	struct s_bsdipa_ctrl_chunk *ccp;
	char x;
	enum s_bsdipa_state rv;
	uint8_t *obuf;
	size_t olen;
	s_bsdipa_off_t diflen, extlen;
	z_streamp zsp;

	zsp = &zs;
	zs.zalloc = &s__bsdipa_io_alloc;
	zs.zfree = &s__bsdipa_io_free;
	zs.opaque = (void*)&dcp->dc_mem;

	switch(deflateInit(zsp, s_BSDIPA_IO_ZLIB_LEVEL)){
	case Z_OK: break;
	case Z_MEM_ERROR: rv = s_BSDIPA_NOMEM; goto jleave;
	default: rv = s_BSDIPA_INVAL; goto jleave;
	}

	diflen = dcp->dc_diff_len;
	extlen = dcp->dc_extra_len;

	olen = dcp->dc_ctrl_len + diflen + extlen; /* Guaranteed to work! */
	if(olen <= 1000 * 150)
		olen = 4096 * 4;
	else if(olen <= 1000 * 1000)
		olen = 4096 * 31;
	else
		olen = 4096 * 244;

	obuf = s__bsdipa_io_alloc((void*)&dcp->dc_mem, 1, olen);
	if(obuf == NULL){
		rv = s_BSDIPA_NOMEM;
		goto jdone;
	}

	zsp->next_out = obuf;
	zsp->avail_out = (uInt)olen;
	ccp = dcp->dc_ctrl;

	for(x = 0;;){
		int flusht;

		flusht = Z_NO_FLUSH;
		if(x == 0){
			zsp->next_in = (Bytef*)&dcp->dc_header;
			zsp->avail_in = sizeof(dcp->dc_header);
			x = 1;
		}else if(x == 1){
			if(ccp != NULL){
				zsp->next_in = ccp->cc_dat;
				zsp->avail_in = (uInt)ccp->cc_len;
				ccp = ccp->cc_next;
			}
			if(ccp == NULL)
				x = 2;
		}else if(x < 4){
			if(x == 2)
				zsp->next_in = dcp->dc_diff_dat;
			if(diflen > s__BSDIPA_IO_LIMIT){
				zsp->avail_in = s__BSDIPA_IO_LIMIT;
				diflen -= s__BSDIPA_IO_LIMIT;
				x = 3;
			}else{
				zsp->avail_in = (uInt)diflen;
				x = 4;
			}
		}else if(x < 6){
			if(x == 4)
				zsp->next_in = dcp->dc_extra_dat;
			if(extlen > s__BSDIPA_IO_LIMIT){
				zsp->avail_in = s__BSDIPA_IO_LIMIT;
				extlen -= s__BSDIPA_IO_LIMIT;
				x = 5;
			}else{
				zsp->avail_in = (uInt)extlen;
				x = 6;
			}
		}else{
			zsp->avail_in = 0; /* xxx redundant */
			flusht = Z_FINISH;
			x = 7;
		}

		if(zsp->avail_in > 0 || flusht == Z_FINISH) for(;;){
			s_bsdipa_off_t z;
			int y;

			y = deflate(zsp, flusht);

			switch(y){
			case Z_OK: break;
			case Z_STREAM_END: break;
			case Z_BUF_ERROR: assert(zsp->avail_out == 0); break;
			default:
			case Z_STREAM_ERROR: rv = s_BSDIPA_INVAL; goto jdone;
			}

			z = olen - zsp->avail_out;
			if(z > 0 && (y == Z_STREAM_END || zsp->avail_out == 0)){
				if((rv = (*hook)(user_cookie, obuf, z)) != s_BSDIPA_OK)
					goto jdone;
				zsp->next_out = obuf;
				zsp->avail_out = (uInt)olen;
			}

			if(flusht == Z_FINISH){
				if(y == Z_STREAM_END)
					goto jeof;/* break;break; */
			}else if(zsp->avail_in == 0)
				break;
			/* Different to documentation this happens! */
		}
		assert(x != 7);
	}

jeof:
	rv = (*hook)(user_cookie, NULL, -1);
jdone:
	if(obuf != NULL)
		s__bsdipa_io_free((void*)&dcp->dc_mem, obuf);

	deflateEnd(zsp);
jleave:
	return rv;
}

s_BSDIPA_IO_LINKAGE enum s_bsdipa_state
s_bsdipa_io_read(struct s_bsdipa_patch_ctx *pcp){
	uint8_t hbuf[sizeof(struct s_bsdipa_header)];
	z_stream zs;
	s_bsdipa_off_t reslen;
	enum s_bsdipa_state rv;
	z_streamp zsp;
	uint64_t patlen;

	pcp->pc_restored_dat = NULL;
	patlen = pcp->pc_patch_len;

	/* make inflateEnd() callable */
	zsp = &zs;
	zs.next_in = (Bytef*)pcp->pc_patch_dat;
	zs.avail_in = (patlen > s__BSDIPA_IO_LIMIT) ? s__BSDIPA_IO_LIMIT : (int)patlen;
	patlen -= (unsigned int)zs.avail_in;
	zs.zalloc = &s__bsdipa_io_alloc;
	zs.zfree = &s__bsdipa_io_free;
	zs.opaque = (void*)&pcp->pc_mem;

	switch(inflateInit(zsp)){
	case Z_OK: break;
	case Z_MEM_ERROR: rv = s_BSDIPA_NOMEM; goto jdone;
	default: rv = s_BSDIPA_INVAL; goto jdone;
	}

	zsp->next_out = hbuf;
	zsp->avail_out = sizeof(hbuf);

	switch(inflate(zsp, Z_SYNC_FLUSH)){
	case Z_OK: break;
	case Z_STREAM_END: break;
	case Z_MEM_ERROR: rv = s_BSDIPA_NOMEM; goto jdone;
	default: rv = s_BSDIPA_INVAL; goto jdone;
	}
	if(zsp->avail_out != 0){
		rv = s_BSDIPA_INVAL;
		goto jdone;
	}

	rv = s_bsdipa_patch_parse_header(&pcp->pc_header, hbuf);
	if(rv != s_BSDIPA_OK)
		goto jdone;

	reslen = pcp->pc_header.h_ctrl_len + pcp->pc_header.h_diff_len + pcp->pc_header.h_extra_len;

	pcp->pc_restored_len = reslen;
	pcp->pc_restored_dat = s__bsdipa_io_alloc(&pcp->pc_mem, 1, reslen);
	if(pcp->pc_restored_dat == NULL){
		rv = s_BSDIPA_NOMEM;
		goto jdone;
	}

	zsp->next_out = pcp->pc_restored_dat;
	zsp->avail_out = (reslen > s__BSDIPA_IO_LIMIT) ? s__BSDIPA_IO_LIMIT : (int)reslen;
	reslen -= zsp->avail_out;

	for(;;){
		int x, y;

		x = (reslen == 0 && patlen == 0) ? Z_FINISH : Z_NO_FLUSH;
		y = inflate(zsp, x);

		switch(y){
		case Z_OK: break;
		case Z_BUF_ERROR:
			if(x == Z_FINISH){
				rv = s_BSDIPA_INVAL;
				goto jdone;
			}
			break;
		case Z_STREAM_END:
			if(x == Z_FINISH){
				rv = s_BSDIPA_OK;
				goto jdone;
			}
			break;
		case Z_MEM_ERROR: rv = s_BSDIPA_NOMEM; goto jdone;
		default: rv = s_BSDIPA_INVAL; goto jdone;
		}

		if(zsp->avail_out == 0){
			zsp->avail_out = (reslen > s__BSDIPA_IO_LIMIT) ? s__BSDIPA_IO_LIMIT : (int)reslen;
			reslen -= (unsigned int)zsp->avail_out;
		}
		if(zsp->avail_in == 0){
			zsp->avail_in = (patlen > s__BSDIPA_IO_LIMIT) ? s__BSDIPA_IO_LIMIT : (int)patlen;
			patlen -= (unsigned int)zsp->avail_in;
		}
	}

jdone:
	inflateEnd(zsp);

	if(rv != s_BSDIPA_OK && pcp->pc_restored_dat != NULL){
		s__bsdipa_io_free(&pcp->pc_mem, pcp->pc_restored_dat);
		pcp->pc_restored_dat = NULL;
	}

	return rv;
}

static voidpf
s__bsdipa_io_alloc(voidpf my_cookie, uInt no, uInt size){
	voidpf rv;
	size_t memsz;
	struct s_bsdipa_memory_ctx *mcp;

	mcp = (struct s_bsdipa_memory_ctx*)my_cookie;
	memsz = (size_t)no * size;

	rv = (mcp->mc_alloc != NULL) ? (*mcp->mc_alloc)(memsz) : (*mcp->mc_custom_alloc)(mcp->mc_custom_cookie, memsz);

	return rv;
}

static void
s__bsdipa_io_free(voidpf my_cookie, voidpf dat){
	struct s_bsdipa_memory_ctx *mcp;

	mcp = (struct s_bsdipa_memory_ctx*)my_cookie;

	(mcp->mc_alloc != NULL) ? (*mcp->mc_free)(dat) : (*mcp->mc_custom_free)(mcp->mc_custom_cookie, dat);
}
/* }}} */

#else /* _IO_ZLIB */
# error Unknown s_BSDIPA_IO value
#endif

#endif /* !s_BSDIPA_IO_H */
/* s-itt-mode */
