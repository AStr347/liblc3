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

#define _POSIX_C_SOURCE 199309L

#include <stdalign.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>

#include <lc3.h>
#include "lc3bin.h"
#include "wave.h"
#include <lc3_iface.h>

/**
 * Error handling
 */

static void error(int status, const char *format, ...)
{
    va_list args;

    fflush(stdout);

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, status ? ": %s\n" : "\n", strerror(status));
    exit(status);
}


/**
 * Parameters
 */

struct parameters {
    const char *fname_in;
    const char *fname_out;
    int bitdepth;
    int srate_hz;
};

static struct parameters parse_args(int argc, char *argv[])
{
    static const char *usage =
        "Usage: %s [in_file] [wav_file]\n"
        "\n"
        "wav_file\t"  "Input wave file, stdin if omitted\n"
        "out_file\t"  "Output bitstream file, stdout if omitted\n"
        "\n"
        "Options:\n"
        "\t-h\t"     "Display help\n"
        "\t-b\t"     "Output bitdepth, 16 bits (default) or 24 bits\n"
        "\t-r\t"     "Output samplerate, default is LC3 stream samplerate\n"
        "\n";

    struct parameters p = { .bitdepth = 16 };

    for (int iarg = 1; iarg < argc; ) {
        const char *arg = argv[iarg++];

        if (arg[0] == '-') {
            if (arg[2] != '\0')
                error(EINVAL, "Option %s", arg);

            char opt = arg[1];
            const char *optarg = NULL;

            switch (opt) {
                case 'b': case 'r':
                    if (iarg >= argc)
                        error(EINVAL, "Argument %s", arg);
                    optarg = argv[iarg++];
            }

            switch (opt) {
                case 'h': fprintf(stderr, usage, argv[0]); exit(0);
                case 'b': p.bitdepth = atoi(optarg); break;
                case 'r': p.srate_hz = atoi(optarg); break;
                default:
                    error(EINVAL, "Option %s", arg);
            }

        } else {

            if (!p.fname_in)
                p.fname_in = arg;
            else if (!p.fname_out)
                p.fname_out = arg;
            else
                error(EINVAL, "Argument %s", arg);
        }
    }

    return p;
}

/**
 * Return time in (us) from unspecified point in the past
 */
static unsigned clock_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);

    return (unsigned)(ts.tv_sec * 1000*1000) + (unsigned)(ts.tv_nsec / 1000);
}

/**
 * Entry point
 */
int main(int argc, char *argv[])
{
    /* --- Read parameters --- */
    struct parameters p = parse_args(argc, argv);

    if (p.srate_hz && !LC3_CHECK_SR_HZ(p.srate_hz))
        error(EINVAL, "Samplerate %d Hz", p.srate_hz);

    if (p.bitdepth && p.bitdepth != 16 && p.bitdepth != 24)
        error(EINVAL, "Bitdepth %d", p.bitdepth);

    /* --- Check parameters --- */

    unsigned t0 = clock_us();

    ilc3_coder_t decoder;
    lc3_coder_init(&decoder,
                   16000,
                   p.bitdepth,
                   p.srate_hz,
                   2,
                   0);
    
    file_lc3_to_wav(&decoder, p.fname_in, p.fname_out);

    unsigned t = (clock_us() - t0) / 1000;

    fprintf(stderr, "Decoded in %d.%03d seconds \n",
            t / 1000, t % 1000);

    return 0;
}
