/* -*- C++ -*-
 * 
 *  graphics_sse2.cpp - graphics routines using X86 SSE2 cpu functionality
 *
 *  Copyright (c) 2009 Mion. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Based upon routines provided by Roto

#include <emmintrin.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "graphics_common.h"


void imageFilterMean_SSE2(unsigned char *src1, unsigned char *src2, unsigned char *dst, int length)
{
    int n = length;

    // Compute first few values so we're on a 16-byte boundary in dst
    while( (((long)dst & 0xF) > 0) && (n > 0) ) {
        int result = ((int)(*src1) + (int)(*src2)) / 2;
        (*dst) = result;
        --n; ++dst; ++src1; ++src2;
    }

    // Do bulk of processing using SSE2 (find the mean of 16 8-bit unsigned integers, with saturation)
    __m128i mask = _mm_set1_epi8(0x7F);
    while(n >= 16) {
        __m128i s1 = _mm_loadu_si128((__m128i*)src1);
        s1 = _mm_srli_epi16(s1, 1); // shift right 1
        s1 = _mm_and_si128(s1, mask); // apply byte-mask
        __m128i s2 = _mm_loadu_si128((__m128i*)src2);
        s2 = _mm_srli_epi16(s2, 1); // shift right 1
        s2 = _mm_and_si128(s2, mask); // apply byte-mask
        __m128i r = _mm_adds_epu8(s1, s2);
        _mm_store_si128((__m128i*)dst, r);

        n -= 16; src1 += 16; src2 += 16; dst += 16;
    }

    // If any bytes are left over, deal with them individually
    BASIC_MEAN()
}


void imageFilterAddTo_SSE2(unsigned char *dst, unsigned char *src, int length)
{
    int n = length;

    // Compute first few values so we're on a 16-byte boundary in dst
    while( (((long)dst & 0xF) > 0) && (n > 0) ) {
        int result = (*dst) + (*src);
        (*dst) = (result < 255) ? result : 255;
        --n; ++dst; ++src;
    }

    // Do bulk of processing using SSE2 (add 16 8-bit unsigned integers, with saturation)
    while(n >= 16) {
        __m128i s = _mm_loadu_si128((__m128i*)src);
        __m128i d = _mm_load_si128((__m128i*)dst);
        __m128i r = _mm_adds_epu8(s, d);
        _mm_store_si128((__m128i*)dst, r);

        n -= 16; src += 16; dst += 16;
    }

    // If any bytes are left over, deal with them individually
    BASIC_ADDTO()
}


void imageFilterSubFrom_SSE2(unsigned char *dst, unsigned char *src, int length)
{
    int n = length;

    // Compute first few values so we're on a 16-byte boundary in dst
    while( (((long)dst & 0xF) > 0) && (n > 0) ) {
        int result = (*dst) - (*src);
        (*dst) = (result > 0) ? result : 0;
        --n; ++dst; ++src;
    }

    // Do bulk of processing using SSE2 (sub 16 8-bit unsigned integers, with saturation)
    while(n >= 16) {
        __m128i s = _mm_loadu_si128((__m128i*)src);
        __m128i d = _mm_load_si128((__m128i*)dst);
        __m128i r = _mm_subs_epu8(d, s);
        _mm_store_si128((__m128i*)dst, r);

        n -= 16; src += 16; dst += 16;
    }

    // If any bytes are left over, deal with them individually
    BASIC_SUBFROM()
}

