/*@ s-bsdipa: create or apply binary difference patch.
 *@ (See ../lib/ for more.)
 *
 * Copyright (c) 2024 - 2025 Steffen Nurpmeso <steffen@sdaoden.eu>.
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

/* Say 0 to disable */
#define a_STATS 1

#define _POSIX_C_SOURCE 202405L
#define _GNU_SOURCE

#include <sys/mman.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef _POSIX_TIMERS
# define a_CLOCK_TS
#else
# undef a_CLOCK_TS
# include <sys/time.h>
#endif

#include "s-bsdipa-lib.h"
#define s_BSDIPA_IO s_BSDIPA_IO_ZLIB
/*#define s_BSDIPA_IO_ZLIB_LEVEL 9*/
#define s_BSDIPA_IO_READ
#define s_BSDIPA_IO_WRITE
#include "s-bsdipa-io.h"
#define a_IO_READ s_bsdipa_io_read_zlib
#define a_IO_WRITE s_bsdipa_io_write_zlib

#ifndef O_BINARY
# define O_BINARY 0
#endif

#define a_EX_OK 0
#define a_EX_USAGE 64
#define a_EX_DATAERR 65
#define a_EX_IOERR 74
#define a_EX_TEMPFAIL 75

#ifdef s_BSDIPA_32
# define a_NAME "s-bsdipa32"
# define a_MAGIC "SBSDIPA/32/" s_BSDIPA_IO_NAME "/"
#else
# define a_NAME "s-bsdipa"
# define a_MAGIC "SBSDIPA/64/" s_BSDIPA_IO_NAME "/"
#endif

/* */
#if !a_STATS
# define a_RESLEN(X)
# define a_CLOCK(X)
#else
# define a_RESLEN(X) a_reslen += X
# ifdef a_CLOCK_TS
#  define a_CLOCK_T struct timespec
#  define a_CLOCK_SEC(X) (X)->tv_sec
#  define a_CLOCK_SSEC_2_1000th(X) ((X)->tv_nsec / (1000000000l / 1000))
#  define a_CLOCK(X) clock_gettime(CLOCK_MONOTONIC, X)
#  define a_CLOCK_SUB(X, Y) \
	(X)->tv_sec -= (Y)->tv_sec;\
	(X)->tv_nsec -= (Y)->tv_nsec;\
	if((X)->tv_nsec < 0){\
		--(X)->tv_sec;\
		(X)->tv_nsec += 1000000000l;\
	}
# else
#  define a_CLOCK_T struct timeval
#  define a_CLOCK_SEC(X) (X)->tv_sec
#  define a_CLOCK_SSEC_2_1000th(X) ((X)->tv_usec / (1000000l / 1000))
#  define a_CLOCK(X) gettimeofday(X, NULL)
#  define a_CLOCK_SUB(X, Y) \
	(X)->tv_sec -= (Y)->tv_sec;\
	(X)->tv_usec -= (Y)->tv_usec;\
	if((X)->tv_usec < 0){\
		--(X)->tv_sec;\
		(X)->tv_usec += 1000000l;\
	}
# endif
#endif /* a_STATS */

#if a_STATS
struct a_mem{
	struct a_mem *m_last;
	size_t m_size;
	void *m_vp;
};

/* a_mem_curr only !NDEBUG */
struct a_mem *a_mem_list;
static size_t a_mem_peek, a_mem_all, a_mem_allno, a_mem_curr;
static s_bsdipa_off_t a_reslen;
#endif

#if a_STATS
static void *a_alloc(size_t size);
static void a_free(void *vp);
#else
# define a_alloc malloc
# define a_free free
#endif

static int a_mmap(char const *file, char const *id, uint64_t *lenp, uint8_t const **datp);

static enum s_bsdipa_state a_hook_write(void *cookie, uint8_t const *dat, s_bsdipa_off_t len, s_bsdipa_off_t is_last);

#if a_STATS
static void *
a_alloc(size_t size){
	struct a_mem *mp;
	void *vp;

	vp = NULL;

	mp = (struct a_mem*)malloc(sizeof(struct a_mem));
	if(mp == NULL)
		goto jleave;

	mp->m_size = size;
	mp->m_vp = vp = malloc(size);

	if(vp != NULL){
		mp->m_last = a_mem_list;
		a_mem_list = mp;

		a_mem_all += size;
		a_mem_curr += size;
		if(a_mem_curr > a_mem_peek)
			a_mem_peek = a_mem_curr;
		++a_mem_allno;
	}else
		free(mp);

jleave:
	return vp;
}

static void
a_free(void *vp){
	struct a_mem **mpp;

	for(mpp = &a_mem_list;; mpp = &(*mpp)->m_last){
		if((*mpp)->m_vp == vp){
			free(vp);
			a_mem_curr -= (*mpp)->m_size;
			vp = *mpp;
			*mpp = (*mpp)->m_last;
			free(vp);
			break;
		}
	}
}
#endif /* a_STATS */

static int
a_mmap(char const *file, char const *id, uint64_t *lenp, uint8_t const **datp){
	char const *emsg;
	off_t xoff;
	int fd;

	if((fd = open(file, O_RDONLY | O_BINARY, 0)) == -1){
		emsg = "cannot open";
		goto jerr;
	}

	if((xoff = lseek(fd, 0, SEEK_END)) == -1){
		emsg = "cannot seek to end";
		goto jerr;
	}
	if(xoff > s_BSDIPA_OFF_MAX - 1 || xoff != (off_t)(size_t)xoff){
		emsg = "excess";
		errno = EFBIG;
		goto jerr;
	}

	*lenp = (uint64_t)xoff;
	*datp = (uint8_t const*)mmap(NULL, (size_t)xoff + 1, PROT_READ, MAP_SHARED, fd, 0);
	if(*datp == MAP_FAILED){
		emsg = "cannot memory map";
		goto jerr;
	}

	if(close(fd) == -1){
		emsg = "cannot close descriptor";
		goto jerr;
	}

	fd = -1;
	emsg = NULL;
jleave:
	if(fd != -1)
		(void)close(fd);

	return (emsg == NULL);
jerr:
	fprintf(stderr, "ERROR: file \"%s\": %s: %s\n", id, emsg, strerror(errno));
	goto jleave;
}

static enum s_bsdipa_state
a_hook_write(void *cookie, uint8_t const *dat, s_bsdipa_off_t len, s_bsdipa_off_t is_last){
	FILE *fp;
	enum s_bsdipa_state rv;

	rv = s_BSDIPA_OK;
	fp = (FILE*)cookie;

	if(len > 0){
		a_RESLEN(len);
		if(fwrite(dat, len, 1, fp) != 1)
			rv = s_BSDIPA_INVAL;
	}

	if(is_last && fflush(fp) == EOF)
		rv = s_BSDIPA_INVAL;

	return rv;
}

int
main(int argc, char *argv[]){
	enum{
		a_NONE,
		a_UNLINK = 1u<<0,
		a_CLOSE = 1u<<1,
		a_FCLOSE = 1u<<2,
		a_UNMAP_AFTER = 1u<<3,
		a_UNMAP_2ND = 1u<<4,
		a_FREE_2ND = 1u<<5,
		a_FREE_CTX = 1u<<6
	};

#if a_STATS
	a_CLOCK_T ts, te, ts2, te2;
#endif
	union{
		struct s_bsdipa_memory_ctx m;
		struct s_bsdipa_patch_ctx p;
		struct s_bsdipa_diff_ctx d;
	} c;
	enum s_bsdipa_state s;
	char const *targetname, *inbef, *inaft, *inpat, *emsg;
	FILE *pfp;
	int f, rv, noheader, fd;

	emsg = inbef = targetname = NULL; /* UNINIT */
	f = a_NONE;

	if(argc != 5)
		goto jeuse;

	c.m.mc_alloc = &a_alloc;
	c.m.mc_free = &a_free;
	rv = a_EX_DATAERR;

	targetname = argv[1];
	noheader = (*targetname == '=') ? (++targetname, 1) : 0;
	fd = (*targetname == '!') ? (++targetname, O_TRUNC) : O_EXCL;

	if(!strcmp(targetname, "patch")){
		targetname = "restored";
		inbef = NULL;
		inaft = argv[2];
		inpat = argv[3];
	}else{
		inbef = argv[2];
		inaft = argv[3];
		inpat = NULL;

		if(!strcmp(targetname, "diff"))
			c.d.dc_magic_window = 0;
		else if(!strcmp(targetname, "xdiff"))
			c.d.dc_magic_window = 16;
		else if(!strncmp(targetname, "diff/", sizeof("diff/") -1)){
			char *ep;
			long l;

			targetname += sizeof("diff/") -1;
			l = strtol(targetname, &ep, 10);
			if(targetname == ep || *ep != '\0')
				goto jeuse;
			if(l <= 0 || l > 4096) /* <> help output! */
				goto jeuse;

			c.d.dc_magic_window = (int32_t)l;
		}else
			goto jeuse;

		targetname = "patch";
	}

	fd = open(argv[4], O_WRONLY | O_BINARY | O_CREAT | fd, 0666);
	if(fd == -1){
		emsg = "cannot create";
		goto Jioerr;
	}
	f |= a_UNLINK | a_CLOSE;

	if((pfp = fdopen(fd, "wb")) == NULL){
		emsg = "cannot fdopen";
		goto Jioerr;
	}
	f ^= a_CLOSE | a_FCLOSE;

	if(!a_mmap(inaft, "after", &c.d.dc_after_len, &c.d.dc_after_dat))
		goto jleave;
	f |= a_UNMAP_AFTER;

	if(inbef != NULL){
		if(!a_mmap(inbef, "before", &c.d.dc_before_len, &c.d.dc_before_dat))
			goto jleave;
		f |= a_UNMAP_2ND;

		a_CLOCK(&ts);

		f |= a_FREE_CTX;
		s = s_bsdipa_diff(&c.d);

		a_CLOCK(&te);

		if(s != s_BSDIPA_OK)
			goto jes;

		emsg = "I/O error";
		if(!noheader && fwrite(a_MAGIC, sizeof(a_MAGIC) -1, 1, pfp) != 1){
			errno = EIO;
			goto Jioerr;
		}else{
			int e;

			a_CLOCK(&ts2);

			switch(a_IO_WRITE(&c.d, &a_hook_write, pfp, 0)){
			default: e = 0; break;
			case s_BSDIPA_FBIG: e = EFBIG; break;
			case s_BSDIPA_NOMEM: e = ENOMEM; break;
			case s_BSDIPA_INVAL: e = EINVAL; break;
			}

			a_CLOCK(&te2);

			if(e != 0){
				errno = e;
				goto Jioerr;
			}
		}
	}else{
		c.p.pc_after_dat = c.d.dc_after_dat;
		c.p.pc_after_len = c.d.dc_after_len;
		c.p.pc_max_allowed_restored_len = 0;

		if(!a_mmap(inpat, "patch", &c.p.pc_patch_len, &c.p.pc_patch_dat))
			goto jleave;
		f |= a_UNMAP_2ND;

		if(!noheader && (c.p.pc_patch_len < sizeof(a_MAGIC) -1 ||
				memcmp(c.p.pc_patch_dat, a_MAGIC, sizeof(a_MAGIC) -1))){
			fprintf(stderr, "ERROR: \"patch\": incorrect file magic\n");
			goto jleave;
		}

		/* Decompress patch */
		/* C99 */{
			int e;

			if(!noheader){
				c.p.pc_patch_dat += sizeof(a_MAGIC) -1;
				c.p.pc_patch_len -= sizeof(a_MAGIC) -1;
			}

			a_CLOCK(&ts2);

			switch(a_IO_READ(&c.p)){
			default: e = 0; break;
			case s_BSDIPA_FBIG: e = EFBIG; break;
			case s_BSDIPA_NOMEM: e = ENOMEM; break;
			case s_BSDIPA_INVAL: e = EINVAL; break;
			}

			a_CLOCK(&te2);

			if(!noheader){
				c.p.pc_patch_dat -= sizeof(a_MAGIC) -1;
				c.p.pc_patch_len += sizeof(a_MAGIC) -1;
			}
			munmap((void*)c.p.pc_patch_dat, (size_t)c.p.pc_patch_len + 1);
			f ^= a_UNMAP_2ND;

			if(e != 0){
				assert(c.p.pc_restored_dat == NULL);
				emsg = "patch corrupt";
				errno = e;
				goto Jioerr;
			}

			f |= a_FREE_2ND;
			c.p.pc_patch_dat = c.p.pc_restored_dat;
			c.p.pc_patch_len = c.p.pc_restored_len;
		}

		a_CLOCK(&ts);

		f |= a_FREE_CTX;
		s = s_bsdipa_patch(&c.p);

		a_CLOCK(&te);
		a_RESLEN(c.p.pc_restored_len);

		if(s != s_BSDIPA_OK)
			goto jes;

		emsg = "I/O error";
		/* C99 */{
			uint8_t const *dat;

			for(dat = c.p.pc_restored_dat; c.p.pc_restored_len > 0;){
				size_t i;

				i = c.p.pc_restored_len;
				if(i >= INT32_MAX - 1)
					i = INT32_MAX - 1;
				c.p.pc_restored_len -= (s_bsdipa_off_t)i;

				if(fwrite(dat, i, 1, pfp) != 1){
					errno = EIO;
					goto Jioerr;
				}
				dat += i;
			}
		}
	}

	if(fflush(pfp) == EOF){
		errno = EIO;
		goto Jioerr;
	}

	if(!ferror(pfp))
		rv = a_EX_OK;

jleave:
#ifndef NDEBUG
	if(f & a_CLOSE){
		assert(!(f & a_FCLOSE));
		close(fd);
	}else
#endif
	      if(f & a_FCLOSE){
		assert(!(f & a_CLOSE));
		f ^= a_FCLOSE;
		if(!fclose(pfp)){
			if(rv == a_EX_OK)
				f ^= a_UNLINK;
		}else Jioerr:{
			int e;

			e = errno;
			fprintf(stderr, "ERROR: \"%s\": %s: %s\n", targetname, emsg, strerror(e));
			if(f & a_FCLOSE)
				fclose(pfp);
			rv = (e == EEXIST) ? a_EX_TEMPFAIL : a_EX_IOERR;
		}
	}

#ifndef NDEBUG
	if(f & a_FREE_CTX){
		if(inbef != NULL)
			s_bsdipa_diff_free(&c.d);
		else
			s_bsdipa_patch_free(&c.p);
	}

	if(f & a_UNMAP_2ND){
		void *vp;
		size_t vl;

		assert(!(f & a_FREE_2ND));
		if(inbef != NULL)
			vp = (void*)c.d.dc_before_dat, vl = (size_t)c.d.dc_before_len;
		else
			vp = (void*)c.p.pc_patch_dat, vl = (size_t)c.p.pc_patch_len;
		munmap(vp, vl + 1);
	}else if(f & a_FREE_2ND)
		a_free((void*)c.p.pc_patch_dat);

	if(f & a_UNMAP_AFTER){
		void *vp;
		size_t vl;

		if(inbef != NULL)
			vp = (void*)c.d.dc_after_dat, vl = (size_t)c.d.dc_after_len;
		else
			vp = (void*)c.p.pc_after_dat, vl = (size_t)c.p.pc_after_len;
		munmap(vp, vl + 1);
	}
#endif /* !NDEBUG */

	if((f & a_UNLINK) && unlink(argv[4]) == -1){
		assert(rv != a_EX_OK);
		fprintf(stderr, "ERROR: removing \"%s\" failed: %s\n", targetname, strerror(errno));
	}

#if a_STATS
	if(rv == a_EX_OK){
		a_CLOCK_SUB(&te, &ts);
		a_CLOCK_SUB(&te2, &ts2);
		fprintf(stderr,
			/* (xxx since we print difference long should be ok)*/
			"# %ld result bytes%s | %zu allocs: all=%zu peek=%zu"
# ifndef NDEBUG
				" curr=%zu"
# endif
			"\n# Algorithm %ld:%03ld secs, I/O %ld:%03ld secs\n",
			(long)a_reslen,
			(inbef != NULL && c.d.dc_is_equal_data ? " | equal inputs" : ""),
			a_mem_allno, a_mem_all, a_mem_peek,
# ifndef NDEBUG
			a_mem_curr,
# endif
			(long)a_CLOCK_SEC(&te), (long)a_CLOCK_SSEC_2_1000th(&te),
			(long)a_CLOCK_SEC(&te2), (long)a_CLOCK_SSEC_2_1000th(&te2));

# ifndef NDEBUG
		if(inbef != NULL){
			long x;

			x = (long)(c.d.dc_ctrl_len / sizeof(struct s_bsdipa_ctrl_triple));
			fprintf(stderr, "# data: ctrl=%ld (%ld entr%s) diff=%ld extra=%ld\n",
				c.d.dc_ctrl_len, x, (x == 1 ? "y" : "ies"), c.d.dc_diff_len, c.d.dc_extra_len);
		}
# endif
	}
#endif

	return rv;
jeuse:
	fprintf(stderr,
		a_NAME " (" s_BSDIPA_IO_NAME "; " s_BSDIPA_VERSION
			"): create or apply binary difference patch\n"
		"\n"
		"  " a_NAME " [!]patch    after  patch restored\n"
		"  " a_NAME " [!]diff     before after patch\n"
		"  " a_NAME " [!]xdiff    before after patch\n"
		"  " a_NAME " [!]diff/VAL before after patch\n"
		"\n"
		"The first uses \"patch\" to create \"restored\" from \"after\".\n"
		"The latter create \"patch\" from the difference of \"after\" and \"before\";\n"
		"they differ in the size of the \"magic window\": diff uses the built-in value,\n"
		"xdiff uses 16, diff/VAL uses VAL, a positive integer <= 4096.\n"
		"An existing target is overwritten if the subcommand is prefixed with \"!\".\n"
		"(If above subcommands are prefixed by \"=\" the otherwise expected / produced\n"
		"filetype identification header is omitted, and only raw data is processed.)\n"
#if a_STATS
		"Certain statistics are written on standard error.\n"
#endif
		"\n"
#if s_BSDIPA_IO != s_BSDIPA_IO_RAW
		". Patches use " s_BSDIPA_IO_NAME " compression.\n"
#endif
#ifdef s_BSDIPA_32
		". Reduced overhead: 32-bit file sizes and patch control data.\n"
#endif
		". Bugs/Contact via " s_BSDIPA_CONTACT ".\n");
	rv = a_EX_USAGE;
	goto jleave;

jes:
	switch(s){
	default: assert(0); break;
	case s_BSDIPA_FBIG: emsg = "a file is too large"; break;
	case s_BSDIPA_NOMEM: emsg = "insufficient memory"; break;
	case s_BSDIPA_INVAL: emsg = "invalid/corrupt data encountered"; break;
	}
	fprintf(stderr, "ERROR: %s\n", emsg);
	assert(rv != a_EX_OK);
	goto jleave;
}

/* s-itt-mode */
