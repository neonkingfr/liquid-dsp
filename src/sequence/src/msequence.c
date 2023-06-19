/*
 * Copyright (c) 2007 - 2023 Joseph Gaeddert
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// maximum-length sequence

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "liquid.internal.h"

#define LIQUID_MIN_MSEQUENCE_M  2
#define LIQUID_MAX_MSEQUENCE_M  31

// maximal-length sequence
struct msequence_s {
    unsigned int m;     // length generator polynomial, shift register
    unsigned int g;     // generator polynomial
    unsigned int a;     // initial shift register state, default: 1

    unsigned int n;     // length of sequence, n = (2^m)-1
    unsigned int v;     // shift register
    unsigned int b;     // return bit
};

// msequence structure
//  Note that 'g' is stored as the default polynomial shifted to the
//  right by one bit; this bit is implied and not actually used in
//  the shift register's feedback bit computation.
struct msequence_s msequence_default[16] = {
//   m,     g,      a,      n,          v,      b
    {0,     0,      1,      0,          1,      0}, // dummy placeholder
    {0,     0,      1,      0,          1,      0}, // dummy placeholder
    {2,     0x0003, 0x0002, (1<< 2U)-1, 0x0002, 0},
    {3,     0x0005, 0x0004, (1<< 3U)-1, 0x0004, 0},
    {4,     0x0009, 0x0008, (1<< 4U)-1, 0x0008, 0},
    {5,     0x0012, 0x0010, (1<< 5U)-1, 0x0010, 0},
    {6,     0x0021, 0x0020, (1<< 6U)-1, 0x0020, 0},
    {7,     0x0044, 0x0040, (1<< 7U)-1, 0x0040, 0},
    {8,     0x008E, 0x0080, (1<< 8U)-1, 0x0080, 0},
    {9,     0x0108, 0x0100, (1<< 9U)-1, 0x0100, 0},
    {10,    0x0204, 0x0200, (1<<10U)-1, 0x0200, 0},
    {11,    0x0402, 0x0400, (1<<11U)-1, 0x0400, 0},
    {12,    0x0829, 0x0800, (1<<12U)-1, 0x0800, 0},
    {13,    0x100d, 0x1000, (1<<13U)-1, 0x1000, 0},
    {14,    0x2015, 0x2000, (1<<14U)-1, 0x2000, 0},
    {15,    0x4001, 0x4000, (1<<15U)-1, 0x4000, 0}
};

// create a maximal-length sequence (m-sequence) object with
// an internal shift register length of _m bits.
//  _m      :   generator polynomial length, sequence length is (2^m)-1
//  _g      :   generator polynomial, starting with most-significant bit
//  _a      :   initial shift register state, default: 000...001
msequence msequence_create(unsigned int _m,
                           unsigned int _g,
                           unsigned int _a)
{
    // validate input
    if (_m > LIQUID_MAX_MSEQUENCE_M || _m < LIQUID_MIN_MSEQUENCE_M)
        return liquid_error_config("msequence_create(), m not in range");
    
    // allocate memory for msequence object
    msequence ms = (msequence) malloc(sizeof(struct msequence_s));

    // set internal values
    ms->m = _m;         // generator polynomial length
    ms->g = _g >> 1;    // generator polynomial (clip off most significant bit)

    // initialize state register, reversing order
    // 0001 -> 1000
    unsigned int i;
    ms->a = 0;
    for (i=0; i<ms->m; i++) {
        ms->a <<= 1;
        ms->a |= (_a & 0x01);
        _a >>= 1;
    }

    ms->n = (1<<_m)-1;  // sequence length, (2^m)-1
    ms->v = ms->a;      // shift register
    ms->b = 0;          // return bit

    return ms;
}


// create a maximal-length sequence (m-sequence) object from a generator polynomial
msequence msequence_create_genpoly(unsigned int _g)
{
    unsigned int t = liquid_msb_index(_g);
    
    // validate input
    if (t < 2)
        return liquid_error_config("msequence_create_genpoly(), invalid generator polynomial: 0x%x", _g);

    // compute derived values
    unsigned int m = t - 1; // m-sequence shift register length
    unsigned int a = 1;     // m-sequence initial state

    // generate object and return
    return msequence_create(m,_g,a);
}

// creates a default maximal-length sequence
msequence msequence_create_default(unsigned int _m)
{
    // validate input
    if (_m < LIQUID_MIN_MSEQUENCE_M || _m > 15)
        return liquid_error_config("msequence_create(), m not in range");
    
    // allocate memory for msequence object
    msequence ms = (msequence) malloc(sizeof(struct msequence_s));

    // copy default sequence
    memmove(ms, &msequence_default[_m], sizeof(struct msequence_s));

    // return
    return ms;
}

// destroy an msequence object, freeing all internal memory
int msequence_destroy(msequence _ms)
{
    free(_ms);
    return LIQUID_OK;
}

// prints the sequence's internal state to the screen
int msequence_print(msequence _m)
{
    unsigned int i;

    printf("msequence: m=%u (n=%u):\n", _m->m, _m->n);

    // print shift register
    printf("    shift register: ");
    for (i=0; i<_m->m; i++)
        printf("%c", ((_m->v) >> (_m->m-i-1)) & 0x01 ? '1' : '0');
    printf("\n");

    // print generator polynomial
    printf("    generator poly: ");
    for (i=0; i<_m->m; i++)
        printf("%c", ((_m->g) >> (_m->m-i-1)) & 0x01 ? '1' : '0');
    printf("\n");
    return LIQUID_OK;
}

// advance msequence on shift register, returning output bit
unsigned int msequence_advance(msequence _ms)
{
    // compute return bit as binary dot product between the
    // internal shift register and the generator polynomial
    _ms->b = liquid_bdotprod( _ms->v, _ms->g );

    _ms->v <<= 1;       // shift internal register
    _ms->v |= _ms->b;   // push bit onto register
    _ms->v &= _ms->n;   // apply mask to register

    return _ms->b;      // return result
}


// generate pseudo-random symbol from shift register
//  _ms     :   m-sequence object
//  _bps    :   bits per symbol of output
unsigned int msequence_generate_symbol(msequence    _ms,
                                       unsigned int _bps)
{
    unsigned int i;
    unsigned int s = 0;
    for (i=0; i<_bps; i++) {
        s <<= 1;
        s |= msequence_advance(_ms);
    }
    return s;
}

// reset msequence shift register to original state, typically '1'
int msequence_reset(msequence _ms)
{
    _ms->v = _ms->a;
    return LIQUID_OK;
}

// initialize a bsequence object on an msequence object
//  _bs     :   bsequence object
//  _ms     :   msequence object
int bsequence_init_msequence(bsequence _bs,
                             msequence _ms)
{
    // clear binary sequence
    bsequence_reset(_bs);

    unsigned int i;
    for (i=0; i<(_ms->n); i++)
        bsequence_push(_bs, msequence_advance(_ms));
    return LIQUID_OK;
}

// get the length of the generator polynomial, g (m)
unsigned int msequence_get_genpoly_length(msequence _ms)
{
    return _ms->m;
}

// get the length of the sequence
unsigned int msequence_get_length(msequence _ms)
{
    return _ms->n;
}

// get the generator polynomial, g
unsigned int msequence_get_genpoly(msequence _ms)
{
    return _ms->g;
}

// get the internal state of the sequence
unsigned int msequence_get_state(msequence _ms)
{
    return _ms->v;
}

// set the internal state of the sequence
int msequence_set_state(msequence    _ms,
                        unsigned int _a)
{
    // set internal state
    // NOTE: if state is set to zero, this will lock the sequence generator,
    //       but let the user set this value if they wish
    _ms->v = _a;
    return LIQUID_OK;
}

