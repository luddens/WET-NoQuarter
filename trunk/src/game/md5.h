#ifdef HW_BAN
/* See md5.c for explanation and copyright information.  */

#ifndef MD5_H
#define MD5_H

/* Add prototype support.  */
#ifndef PROTO
#if defined (USE_PROTOTYPES) ? USE_PROTOTYPES : defined (__STDC__)
#define PROTO(ARGS) ARGS
#else
#define PROTO(ARGS) ()
#endif
#endif

/* Unlike previous versions of this code, uint32 need not be exactly
   32 bits, merely 32 bits or more.  Choosing a data type which is 32
   bits instead of 64 is not important; speed is considerably more
   important.  ANSI guarantees that "unsigned long" will be big enough,
   and always using it seems to have few disadvantages.  */
typedef unsigned long uint32;

struct MD5Context {
	uint32 buf[4];
	uint32 bits[2];
	unsigned char in[64];
};

void MD5Init PROTO ((struct MD5Context *context));
void MD5Update PROTO ((struct MD5Context *context,
			   unsigned char const *buf, unsigned len));
void MD5Final PROTO ((unsigned char digest[16],
			  struct MD5Context *context));
void MD5Transform PROTO ((uint32 buf[4], const unsigned char in[64]));

#endif /* !MD5_H */

#endif // HW_BAN
