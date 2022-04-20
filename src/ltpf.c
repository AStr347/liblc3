/******************************************************************************
 *
 *  Copyright 2022 Google LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#include "ltpf.h"
#include "tables.h"


/* ----------------------------------------------------------------------------
 *  Resampling
 * -------------------------------------------------------------------------- */

/**
 * Resample from 8 / 16 / 32 KHz to 12.8 KHz Template
 * p               Resampling factor with 64 KHz (8, 4 or 2)
 * x               [-d..-1] Previous, [0..ns-1] Current samples
 * y, n            [0..n-1] Output `n` processed samples
 *
 * The number of previous samples `d` accessed on `x` is :
 *   d: { 10, 20, 40 } - 1 for resampling factors 8, 4 and 2.
 */
static inline void resample_base_64k_12k8(const int p,
    struct lc3_ltpf_hp50_state *hp50, const float *x, float *y, int n)
{
    /* --- Parameters  ---
     * bn, an: High-Pass Biquad coefficients,
     * with `bn` support of rescaling resampling factor.
     * Note that it's an High-Pass filter, so we have `b0 = b2`,
     * in the following steps we use `b0` as `b2`. */

    const int w = 40 / p;

    const float *h = lc3_ltpf_h12k8 + 119;
    const float a1 = -1.965293373f, b1 = -1.965589417f * 3*LC3_MIN(p, 4);
    const float a2 =  0.965885461f, b2 =  0.982794708f * 3*LC3_MIN(p, 4);

    /* --- Resampling & filtering --- */

    for (int i = 0; i < n; i += 8, x += w)
        for (int j = 0; j < 40; j += 5) {

            const float *hn = h - 3*(p*w + (j % p));
            const float *xn = x -   (2*w - (j / p));
            float yn, un = 0;

            for (int k = 0; k < 2*w; k += 10) {
                un += *(++xn) * *(hn += (3*p));
                un += *(++xn) * *(hn += (3*p));
                un += *(++xn) * *(hn += (3*p));
                un += *(++xn) * *(hn += (3*p));
                un += *(++xn) * *(hn += (3*p));
                un += *(++xn) * *(hn += (3*p));
                un += *(++xn) * *(hn += (3*p));
                un += *(++xn) * *(hn += (3*p));
                un += *(++xn) * *(hn += (3*p));
                un += *(++xn) * *(hn += (3*p));
            }

            yn = b2 * un + hp50->s1;
            hp50->s1 = b1 * un - a1 * yn + hp50->s2;
            hp50->s2 = b2 * un - a2 * yn;
            *(y++) = yn;
        }
}

/**
 * Resample from 24 / 48 KHz to 12.8 KHz Template
 * p               Resampling factor with 192 KHz (8 or 4)
 * x               [-d..-1] Previous, [0..ns-1] Current samples
 * y, n            [0..n-1] Output `n` processed samples
 *
 * The number of previous samples `d` accessed on `x` is :
 *   d: { 30, 60 } - 1 for resampling factors 8 and 4.
 */
static inline void resample_base_192k_12k8(const int p,
    struct lc3_ltpf_hp50_state *hp50, const float *x, float *y, int n)
{
    /* --- Parameters  ---
     * bn, an: High-Pass Biquad coefficients,
     * with `bn` support of rescaling resampling factor.
     * Note that it's an High-Pass filter, so we have `b0 = b2`,
     * in the following steps we use `b0` as `b2`. */

    const int w = 120 / p;

    const float *h = lc3_ltpf_h12k8 + 119;
    const float a1 = -1.965293373f, b1 = -1.965589417f * p;
    const float a2 =  0.965885461f, b2 =  0.982794708f * p;

    /* --- Resampling & filtering --- */

    for (int i = 0; i < n; i += 8, x += w)
        for (int j = 0; j < 120; j += 15) {

            const float *hn = h - (p*w + (j % p));
            const float *xn = x - (2*w - (j / p));
            float yn, un = 0;

            for (int k = 0; k < 2*w; k += 15) {
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
                un += *(++xn) * *(hn += p);
            }

            yn = b2 * un + hp50->s1;
            hp50->s1 = b1 * un - a1 * yn + hp50->s2;
            hp50->s2 = b2 * un - a2 * yn;
            *(y++) = yn;
        }
}

/**
 * LTPF Resample to 12.8 KHz implementations for each samplerates
 */

static void resample_8k_12k8(
    struct lc3_ltpf_hp50_state *hp50, const float *x, float *y, int n)
{
    resample_base_64k_12k8(8, hp50, x, y, n);
}

static void resample_16k_12k8(
    struct lc3_ltpf_hp50_state *hp50, const float *x, float *y, int n)
{
    resample_base_64k_12k8(4, hp50, x, y, n);
}

static void resample_24k_12k8(
    struct lc3_ltpf_hp50_state *hp50, const float *x, float *y, int n)
{
    resample_base_192k_12k8(8, hp50, x, y, n);
}

static void resample_32k_12k8(
    struct lc3_ltpf_hp50_state *hp50, const float *x, float *y, int n)
{
    resample_base_64k_12k8(2, hp50, x, y, n);
}

static void resample_48k_12k8(
    struct lc3_ltpf_hp50_state *hp50, const float *x, float *y, int n)
{
    resample_base_192k_12k8(4, hp50, x, y, n);
}

static void (* const resample_12k8[])
    (struct lc3_ltpf_hp50_state *, const float *, float *, int ) =
{
    [LC3_SRATE_8K ] = resample_8k_12k8,
    [LC3_SRATE_16K] = resample_16k_12k8,
    [LC3_SRATE_24K] = resample_24k_12k8,
    [LC3_SRATE_32K] = resample_32k_12k8,
    [LC3_SRATE_48K] = resample_48k_12k8,
};

/**
 * Resample to 6.4 KHz (cf. 3.3.9.3-4)
 * x               [-3..-1] Previous, [0..n-1] Current samples
 * y, n            [0..n-1] Output `n` processed samples
 */
static void resample_6k4(const float *x, float *y, int n)
{
    static const float h[] = { 0.2819382921, 0.2353512128, 0.1236796411 };
    float xn2 = x[-3], xn1 = x[-2], x0 = x[-1], x1, x2;

    for (const float *ye = y + n; y < ye; xn2 = x0, xn1 = x1, x0 = x2) {
        x1 = *(x++); x2 = *(x++);

        *(y++) = x0 * h[0] + (xn1 + x1) * h[1] + (xn2 + x2) * h[2];
    }
}


/* ----------------------------------------------------------------------------
 *  Analysis
 * -------------------------------------------------------------------------- */

/**
 * Return dot product of 2 vectors
 * a, b, n         The 2 vectors of size `n` (multiple of 16)
 * return          sum( a[i] * b[i] ), i = [0..n-1]
 */
static inline float dot(const float *a, const float *b, int n)
{
    float v = 0;

    for (int i = 0; i < (n >> 4); i++)
        for (int j = 0; j < 16; j++)
            v += *(a++) * *(b++);

    return v;
}

/**
 * Return vector of correlations
 * a, b, n         The 2 vector of size `n` to correlate
 * y, nc           Output the correlation vector of size `nc`
 *
 * The size `n` of input vectors must be multiple of 16
 */
static void correlate(
    const float *a, const float *b, int n, float *y, int nc)
{
    for (const float *ye = y + nc; y < ye; )
        *(y++) = dot(a, b--, n);
}

/**
 * Search the maximum value and returns its argument
 * x, n            The input vector of size `n`
 * x_max           Return the maximum value
 * return          Return the argument of the maximum
 */
static int argmax(const float *x, int n, float *x_max)
{
    int arg = 0;

    *x_max = x[arg = 0];
    for (int i = 1; i < n; i++)
        if (*x_max < x[i])
            *x_max = x[arg = i];

    return arg;
}

/**
 * Search the maximum weithed value and returns its argument
 * x, n            The input vector of size `n`
 * w_incr          Increment of the weight
 * x_max, xw_max   Return the maximum not weighted value
 * return          Return the argument of the weigthed maximum
 */
static int argmax_weighted(
    const float *x, int n, float w_incr, float *x_max)
{
    int arg;

    float xw_max = (*x_max = x[arg = 0]);
    float w = 1 + w_incr;

    for (int i = 1; i < n; i++, w += w_incr)
        if (xw_max < x[i] * w)
            xw_max = (*x_max = x[arg = i]) * w;

    return arg;
}

/**
 * Interpolate from pitch detected value (3.3.9.8)
 * x, n            [-2..-1] Previous, [0..n] Current input
 * d               The phase of interpolation (0 to 3)
 * return          The interpolated vector
 *
 * The size `n` of vectors must be multiple of 4
 */
static void interpolate(const float *x, int n, int d, float *y)
{
    static const float h4[][8] = {
        { 2.09880463e-01, 5.83527575e-01, 2.09880463e-01                 },
        { 1.06999186e-01, 5.50075002e-01, 3.35690625e-01, 6.69885837e-03 },
        { 3.96711478e-02, 4.59220930e-01, 4.59220930e-01, 3.96711478e-02 },
        { 6.69885837e-03, 3.35690625e-01, 5.50075002e-01, 1.06999186e-01 },
    };

    const float *h = h4[d];
    float x3 = x[-2], x2 = x[-1], x1, x0;

    x1 = (*x++);
    for (const float *ye = y + n; y < ye; ) {
        *(y++) = (x0 = *(x++)) * h[0] + x1 * h[1] + x2 * h[2] + x3 * h[3];
        *(y++) = (x3 = *(x++)) * h[0] + x0 * h[1] + x1 * h[2] + x2 * h[3];
        *(y++) = (x2 = *(x++)) * h[0] + x3 * h[1] + x0 * h[2] + x1 * h[3];
        *(y++) = (x1 = *(x++)) * h[0] + x2 * h[1] + x3 * h[2] + x0 * h[3];
    }
}

/**
 * Interpolate autocorrelation (3.3.9.7)
 * x               [-4..-1] Previous, [0..4] Current input
 * d               The phase of interpolation (-3 to 3)
 * return          The interpolated value
 */
static float interpolate_4(const float *x, int d)
{
    static const float h4[][8] = {
        {  1.53572770e-02, -4.72963246e-02,  8.35788573e-02,  8.98638285e-01,
           8.35788573e-02, -4.72963246e-02,  1.53572770e-02,                 },
        {  2.74547165e-03,  4.59833449e-03, -7.54404636e-02,  8.17488686e-01,
           3.30182571e-01, -1.05835916e-01,  2.86823405e-02, -2.87456116e-03 },
        { -3.00125103e-03,  2.95038503e-02, -1.30305021e-01,  6.03297008e-01,
           6.03297008e-01, -1.30305021e-01,  2.95038503e-02, -3.00125103e-03 },
        { -2.87456116e-03,  2.86823405e-02, -1.05835916e-01,  3.30182571e-01,
           8.17488686e-01, -7.54404636e-02,  4.59833449e-03,  2.74547165e-03 },
    };

    const float *h = h4[(4+d) % 4];

    float y = d < 0 ? x[-4] * *(h++) :
              d > 0 ? x[ 4] * *(h+7) : 0;

    y += x[-3] * h[0] + x[-2] * h[1] + x[-1] * h[2] + x[0] * h[3] +
         x[ 1] * h[4] + x[ 2] * h[5] + x[ 3] * h[6];

    return y;
}

/**
 * Pitch detection algorithm (3.3.9.5-6)
 * ltpf            Context of analysis
 * x, n            [-114..-17] Previous, [0..n-1] Current 6.4KHz samples
 * tc              Return the pitch-lag estimation
 * return          True when pitch present
 */
static bool detect_pitch(
    struct lc3_ltpf_analysis *ltpf, const float *x, int n, int *tc)
{
    float rm1, rm2;
    float r[98];

    const int r0 = 17, nr = 98;
    int k0 = LC3_MAX(   0, ltpf->tc-4);
    int nk = LC3_MIN(nr-1, ltpf->tc+4) - k0 + 1;

    correlate(x, x - r0, n, r, nr);

    int t1 = argmax_weighted(r, nr, -.5f/(nr-1), &rm1);
    int t2 = k0 + argmax(r + k0, nk, &rm2);

    const float *x1 = x - (r0 + t1);
    const float *x2 = x - (r0 + t2);

    float nc1 = rm1 <= 0 ? 0 :
        rm1 / sqrtf(dot(x, x, n) * dot(x1, x1, n));

    float nc2 = rm2 <= 0 ? 0 :
        rm2 / sqrtf(dot(x, x, n) * dot(x2, x2, n));

    int t1sel = nc2 <= 0.85f * nc1;
    ltpf->tc = (t1sel ? t1 : t2);

    *tc = r0 + ltpf->tc;
    return (t1sel ? nc1 : nc2) > 0.6f;
}

/**
 * Pitch-lag parameter (3.3.9.7)
 * x, n            [-232..-28] Previous, [0..n-1] Current 12.8KHz samples
 * tc              Pitch-lag estimation
 * pitch           The pitch value, in fixed .4
 * return          The bitstream pitch index value
 */
static int refine_pitch(const float *x, int n, int tc, int *pitch)
{
    float r[17], rm;
    int e, f;

    int r0 = LC3_MAX( 32, 2*tc - 4);
    int nr = LC3_MIN(228, 2*tc + 4) - r0 + 1;

    correlate(x, x - (r0 - 4), n, r, nr + 8);

    e = r0 + argmax(r + 4, nr, &rm);
    const float *re = r + (e - (r0 - 4));

    float dm = interpolate_4(re, f = 0);
    for (int i = 1; i <= 3; i++) {
        float d;

        if (e >= 127 && ((i & 1) | (e >= 157)))
            continue;

        if ((d = interpolate_4(re, i)) > dm)
            dm = d, f = i;

        if (e > 32 && (d = interpolate_4(re, -i)) > dm)
            dm = d, f = -i;
    }

    e -=   (f < 0);
    f += 4*(f < 0);

    *pitch = 4*e + f;
    return e < 127 ? 4*e +  f       - 128 :
           e < 157 ? 2*e + (f >> 1) + 126 : e + 283;
}

/**
 * LTPF Analysis
 */
bool lc3_ltpf_analyse(enum lc3_dt dt, enum lc3_srate sr,
    struct lc3_ltpf_analysis *ltpf, const float *x, struct lc3_ltpf_data *data)
{
    /* --- Resampling to 12.8 KHz --- */

    int z_12k8 = sizeof(ltpf->x_12k8) / sizeof(float);
    int n_12k8 = dt == LC3_DT_7M5 ? 96 : 128;

    memmove(ltpf->x_12k8, ltpf->x_12k8 + n_12k8,
        (z_12k8 - n_12k8) * sizeof(float));

    float *x_12k8 = ltpf->x_12k8 + (z_12k8 - n_12k8);
    resample_12k8[sr](&ltpf->hp50, x, x_12k8, n_12k8);

    x_12k8 -= (dt == LC3_DT_7M5 ? 44 :  24);

    /* --- Resampling to 6.4 KHz --- */

    int z_6k4 = sizeof(ltpf->x_6k4) / sizeof(float);
    int n_6k4 = n_12k8 >> 1;

    memmove(ltpf->x_6k4, ltpf->x_6k4 + n_6k4,
        (z_6k4 - n_6k4) * sizeof(float));

    float *x_6k4 = ltpf->x_6k4 + (z_6k4 - n_6k4);
    resample_6k4(x_12k8, x_6k4, n_6k4);

    /* --- Pitch detection --- */

    int tc, pitch = 0;
    float nc = 0;

    bool pitch_present = detect_pitch(ltpf, x_6k4, n_6k4, &tc);

    if (pitch_present) {
        float u[n_12k8], v[n_12k8];

        data->pitch_index = refine_pitch(x_12k8, n_12k8, tc, &pitch);

        interpolate(x_12k8, n_12k8, 0, u);
        interpolate(x_12k8 - (pitch >> 2), n_12k8, pitch & 3, v);

        nc = dot(u, v, n_12k8) / sqrtf(dot(u, u, n_12k8) * dot(v, v, n_12k8));
    }

    /* --- Activation --- */

     if (ltpf->active) {
        int pitch_diff =
            LC3_MAX(pitch, ltpf->pitch) - LC3_MIN(pitch, ltpf->pitch);
        float nc_diff = nc - ltpf->nc[0];

        data->active = pitch_present &&
            ((nc > 0.9f) || (nc > 0.84f && pitch_diff < 8 && nc_diff > -0.1f));

     } else {
         data->active = pitch_present &&
            ( (dt == LC3_DT_10M || ltpf->nc[1] > 0.94f) &&
              (ltpf->nc[0] > 0.94f && nc > 0.94f) );
     }

     ltpf->active = data->active;
     ltpf->pitch = pitch;
     ltpf->nc[1] = ltpf->nc[0];
     ltpf->nc[0] = nc;

     return pitch_present;
}


/* ----------------------------------------------------------------------------
 *  Synthesis
 * -------------------------------------------------------------------------- */

/**
 * Synthesis filter template
 * xr, nr          Ring buffer of filtered samples
 * lag             Lag parameter in the ring buffer
 * x0              w-1 previous input samples
 * x, n            Current samples as input, filtered as output
 * c, w            Coefficients `den` then `num`, and width of filter
 * fade            Fading mode of filter  -1: Out  1: In  0: None
 */
static inline void synthesize_template(const float *xr, int nr, int lag,
    const float *x0, float *x, int n, const float *c, const int w, int fade)
{
    float g = (float)(fade <= 0);
    float g_incr = (float)((fade > 0) - (fade < 0)) / n;
    float u[w];

    /* --- Load previous samples --- */

    lag += (w >> 1);

    const float *y = x - xr < lag ? x + (nr - lag) : x - lag;
    const float *y_end = xr + nr - 1;

    for (int j = 0; j < w-1; j++) {

        u[j] = 0;

        float yi = *y, xi = *(x0++);
        y = y < y_end ? y + 1 : xr;

        for (int k = 0; k <= j; k++)
            u[j-k] -= yi * c[k];

        for (int k = 0; k <= j; k++)
            u[j-k] += xi * c[w+k];
    }

    u[w-1] = 0;

    /* --- Process by filter length --- */

    for (int i = 0; i < n; i += w)
        for (int j = 0; j < w; j++, g += g_incr) {

            float yi = *y, xi = *x;
            y = y < y_end ? y + 1 : xr;

            for (int k = 0; k < w; k++)
                u[(j+(w-1)-k)%w] -= yi * c[k];

            for (int k = 0; k < w; k++)
                u[(j+(w-1)-k)%w] += xi * c[w+k];

            *(x++) = xi - g * u[j];
            u[j] = 0;
        }
}

/**
 * Synthesis filter for each samplerates (width of filter)
 */

static void nsynthesize_4(const float *xr, int nr, int lag,
    const float *x0, float *x, int n, const float *c, int fade)
{
    synthesize_template(xr, nr, lag, x0, x, n, c, 4, fade);
}

static void nsynthesize_6(const float *xr, int nr, int lag,
    const float *x0, float *x, int n, const float *c, int fade)
{
    synthesize_template(xr, nr, lag, x0, x, n, c, 6, fade);
}

static void nsynthesize_8(const float *xr, int nr, int lag,
    const float *x0, float *x, int n, const float *c, int fade)
{
    synthesize_template(xr, nr, lag, x0, x, n, c, 8, fade);
}

static void nsynthesize_12(const float *xr, int nr, int lag,
    const float *x0, float *x, int n, const float *c, int fade)
{
    synthesize_template(xr, nr, lag, x0, x, n, c, 12, fade);
}

static void (* const synthesize[])(const float *, int, int,
    const float *, float *, int, const float *, int) =
{
    [LC3_SRATE_8K ] = nsynthesize_4,
    [LC3_SRATE_16K] = nsynthesize_4,
    [LC3_SRATE_24K] = nsynthesize_6,
    [LC3_SRATE_32K] = nsynthesize_8,
    [LC3_SRATE_48K] = nsynthesize_12,
};


/**
 * LTPF Synthesis
 */
void lc3_ltpf_synthesize(enum lc3_dt dt, enum lc3_srate sr, int nbytes,
    lc3_ltpf_synthesis_t *ltpf, const lc3_ltpf_data_t *data,
    const float *xr, float *x)
{
    int nr = LC3_NR(dt, sr);
    int dt_us = LC3_DT_US(dt);

    /* --- Filter parameters --- */

    int p_idx = data ? data->pitch_index : 0;
    int pitch =
        p_idx >= 440 ? (((p_idx     ) - 283) << 2)  :
        p_idx >= 380 ? (((p_idx >> 1) -  63) << 2) + (((p_idx & 1)) << 1) :
                       (((p_idx >> 2) +  32) << 2) + (((p_idx & 3)) << 0)  ;

    pitch = (pitch * LC3_SRATE_KHZ(sr) * 10 + 64) / 128;

    int nbits = (nbytes*8 * 10000 + (dt_us/2)) / dt_us;
    int g_idx = LC3_MAX(nbits / 80, 3 + (int)sr) - (3 + sr);
    bool active = data && data->active && g_idx < 4;

    int w = LC3_MAX(4, LC3_SRATE_KHZ(sr) / 4);
    float c[2*w];

    for (int i = 0; i < w; i++) {
        float g = active ? 0.4f - 0.05f * g_idx : 0;
        c[  i] = g * lc3_ltpf_cden[sr][pitch & 3][(w-1)-i];
        c[w+i] = 0.85f * g * lc3_ltpf_cnum[sr][g_idx][(w-1)-i];
    }

    /* --- Transition handling --- */

    int ns = LC3_NS(dt, sr);
    int nt = ns / (3 + dt);
    float x0[w];

    if (active)
        memcpy(x0, x + nt-(w-1), (w-1) * sizeof(float));

    if (!ltpf->active && active)
        synthesize[sr](xr, nr, pitch/4, ltpf->x, x, nt, c, 1);
    else if (ltpf->active && !active)
        synthesize[sr](xr, nr, ltpf->pitch/4, ltpf->x, x, nt, ltpf->c, -1);
    else if (ltpf->active && active && ltpf->pitch == pitch)
        synthesize[sr](xr, nr, pitch/4, ltpf->x, x, nt, c, 0);
    else if (ltpf->active && active) {
        synthesize[sr](xr, nr, ltpf->pitch/4, ltpf->x, x, nt, ltpf->c, -1);
        synthesize[sr](xr, nr, pitch/4,
            (x <= xr ? x + nr : x) - (w-1), x, nt, c, 1);
    }

    /* --- Remainder --- */

    memcpy(ltpf->x, x + ns - (w-1), (w-1) * sizeof(float));

    if (active)
        synthesize[sr](xr, nr, pitch/4, x0, x + nt, ns-nt, c, 0);

    /* --- Update state --- */

    ltpf->active = active;
    ltpf->pitch = pitch;
    memcpy(ltpf->c, c, 2*w * sizeof(*ltpf->c));
}


/* ----------------------------------------------------------------------------
 *  Bitstream data
 * -------------------------------------------------------------------------- */

/**
 * LTPF disable
 */
void lc3_ltpf_disable(struct lc3_ltpf_data *data)
{
    data->active = false;
}

/**
 * Return number of bits coding the bitstream data
 */
int lc3_ltpf_get_nbits(bool pitch)
{
    return 1 + 10 * pitch;
}

/**
 * Put bitstream data
 */
void lc3_ltpf_put_data(lc3_bits_t *bits,
    const struct lc3_ltpf_data *data)
{
    lc3_put_bit(bits, data->active);
    lc3_put_bits(bits, data->pitch_index, 9);
}

/**
 * Get bitstream data
 */
void lc3_ltpf_get_data(lc3_bits_t *bits, struct lc3_ltpf_data *data)
{
    data->active = lc3_get_bit(bits);
    data->pitch_index = lc3_get_bits(bits, 9);
}
