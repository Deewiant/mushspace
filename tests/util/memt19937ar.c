#include "random.h"
/*
   This is a maximally equidistributed Mersenne Twister MEMT19937 by Shin Harase (2010/1/25).

   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote
        products derived from this software without specific prior written
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

#if 0
#include <stdio.h>
#endif

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)

static uint32_t mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */
static int kk = 0;

/* mag01[x] = x * MATRIX_A  for x=0,1 */
static uint32_t mag01[2]={0x0, MATRIX_A};

/*plot type*/
static uint32_t case_1(void);
static uint32_t case_2(void);
static uint32_t case_3(void);
static uint32_t case_4(void);
static uint32_t case_5(void);
static uint32_t case_6(void);
static uint32_t case_7(void);
/* generates a random number on [0,0xffffffff]-interval */
       uint32_t (*genrand_int32)(void);

static uint32_t y;

/* initializes mt[N] with a seed */
void init_genrand(uint32_t s)
{
    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<N; mti++) {
        mt[mti] =
	    (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
    genrand_int32 = case_1;
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void init_by_array(uint32_t init_key[], int key_length)
{
    int i, j, k;
    init_genrand(19650218UL);
    i=1; j=0;
    k = (N>key_length ? N : key_length);
    for (; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=N-1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=N) { mt[0] = mt[N-1]; i=1; }
    }

    mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */
    genrand_int32 = case_1;
}

uint32_t case_1(void){
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
		    y = mt[kk]^ ((mt[kk+224] << 14) & 0x3cd68000) ^ ((mt[kk+124] << 3) & 0x576bad28) ^ ((mt[kk+24] << 18) & 0xd6740000);
			y ^= TEMPERING_SHIFT_U(y);
			y ^= TEMPERING_SHIFT_S(y);
			y ^= (mt[kk+324] & 0x09040000);
			kk++;
			if(kk==N-M)genrand_int32 = case_2;
			return y;
			}

uint32_t case_2(void){
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
			y = mt[kk]^ ((mt[kk+224] << 14) & 0x3cd68000) ^ ((mt[kk+124] << 3) & 0x576bad28) ^ ((mt[kk+24] << 18) & 0xd6740000);
			y ^= TEMPERING_SHIFT_U(y); y ^= TEMPERING_SHIFT_S(y);
			y ^= (mt[kk+324] & 0x09040000);
			kk++;
			if(kk==300)genrand_int32 = case_3;
			return y;
		}

uint32_t case_3(void){
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
			y=mt[kk]^ ((mt[kk+224] << 14) & 0x3cd68000) ^ ((mt[kk+124] << 3) & 0x576bad28) ^ ((mt[kk+24] << 18) & 0xd6740000);
			y ^= TEMPERING_SHIFT_U(y); y ^= TEMPERING_SHIFT_S(y);
			y ^= (mt[kk-300] & 0x09040000);
			kk++;
			if(kk==400)genrand_int32 = case_4;
			return y;
		}

uint32_t case_4(void){
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
			y=mt[kk]^ ((mt[kk-400] << 14) & 0x3cd68000) ^ ((mt[kk+124] << 3) & 0x576bad28) ^ ((mt[kk+24] << 18) & 0xd6740000);
			y ^= TEMPERING_SHIFT_U(y); y ^= TEMPERING_SHIFT_S(y);
			y ^= (mt[kk-300] & 0x09040000);
			kk++;
			if(kk==500)genrand_int32 = case_5;
			return y;
		}

uint32_t case_5(void){
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
			y=mt[kk]^ ((mt[kk-400] << 14) & 0x3cd68000) ^ ((mt[kk-500] << 3) & 0x576bad28) ^ ((mt[kk+24] << 18) & 0xd6740000);
			y ^= TEMPERING_SHIFT_U(y); y ^= TEMPERING_SHIFT_S(y);
			y  = y ^ (mt[kk-300] & 0x09040000);
			kk++;
			if(kk==600)genrand_int32 = case_6;
			return y;
		}

uint32_t case_6(void){
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
			y=mt[kk]^ ((mt[kk-400] << 14) & 0x3cd68000) ^ ((mt[kk-500] << 3) & 0x576bad28) ^ ((mt[kk-600] << 18) & 0xd6740000);
			y ^= TEMPERING_SHIFT_U(y); y ^= TEMPERING_SHIFT_S(y);
			y  = y ^ (mt[kk-300] & 0x09040000);
			kk++;
			if(kk==N-1)genrand_int32 = case_7;
			return y;
		}

uint32_t case_7(void){
            y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
            mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];
			y=mt[kk]^ ((mt[kk-400] << 14) & 0x3cd68000) ^ ((mt[kk-500] << 3) & 0x576bad28) ^ ((mt[kk-600] << 18) & 0xd6740000);
			y ^= TEMPERING_SHIFT_U(y); y ^= TEMPERING_SHIFT_S(y);
			y  = y ^ (mt[kk-300] & 0x09040000);
			kk=0;
			genrand_int32 = case_1;
			return y;
			}
#if 0

/* generates a random number on [0,0x7fffffff]-interval */
int32_t genrand_int31(void)
{
    return (int32_t)(genrand_int32()>>1);
}

/* generates a random number on [0,1]-real-interval */
double genrand_real1(void)
{
    return genrand_int32()*(1.0/4294967295.0);
    /* divided by 2^32-1 */
}

/* generates a random number on [0,1)-real-interval */
double genrand_real2(void)
{
    return genrand_int32()*(1.0/4294967296.0);
    /* divided by 2^32 */
}

/* generates a random number on (0,1)-real-interval */
double genrand_real3(void)
{
    return (((double)genrand_int32()) + 0.5)*(1.0/4294967296.0);
    /* divided by 2^32 */
}

/* generates a random number on [0,1) with 53-bit resolution*/
double genrand_res53(void)
{
    uint32_t a=genrand_int32()>>5, b=genrand_int32()>>6;
    return(a*67108864.0+b)*(1.0/9007199254740992.0);
}
/* These real versions are due to Isaku Wada, 2002/01/09 added */

int main(void)
{
    int i;
    uint32_t init[4]={0x123, 0x234, 0x345, 0x456}, length=4;
    init_by_array(init, length);
    printf("1000 outputs of genrand_int32()\n");
    for (i=0; i<1000; i++) {
      printf("%10lu ", genrand_int32());
      if (i%5==4) printf("\n");
    }
    printf("\n1000 outputs of genrand_real2()\n");
    for (i=0; i<1000; i++) {
      printf("%10.8f ", genrand_real2());
      if (i%5==4) printf("\n");
    }
    return 0;
}
#endif

void random_fill(void* p, size_t n) {
   uint32_t *p32 = p;
   for (; n >= sizeof *p32; n -= sizeof *p32)
      *p32++ = genrand_int32();
   if (n) {
      unsigned char *pc = (unsigned char*)p32;
      uint32_t i = genrand_int32();
      while (n--) {
         *pc++ = (unsigned char)i;
         i >>= CHAR_BIT;
      }
   }
}
