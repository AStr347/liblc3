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

#ifndef __LC3BIN_H
#define __LC3BIN_H

#include <stdio.h>
#include <stdint.h>
#include <lc3.h>
#include "bytestream.h"


/**
 * Read LC3 binary header
 */
extern
int lc3bin_fread_header(FILE *fp,
                       int *frame_us,
                       int *srate_hz,
                       int *nchannels,
                       int *nsamples);

/**
 * Read LC3 block of data
 */
extern
int lc3bin_fread_data(FILE *fp, int nchannels, void *buffer);

/**
 * Write LC3 binary header
 */
extern
int lc3bin_fwrite_header(FILE *fp,
                        int frame_us,
                        int srate_hz,
                        int bitrate,
                        int nchannels,
                        int nsamples);

/**
 * Write LC3 block of data
 */
extern
int lc3bin_fwrite_data(FILE *fp,
                       const void *data,
                       const int nchannels,
                       const int frame_bytes);

/**
 * Read LC3 binary header
 */
extern
int lc3bin_bread_header(bstream_t *fp,
                       int *frame_us,
                       int *srate_hz,
                       int *nchannels,
                       int *nsamples);

/**
 * Read LC3 block of data
 */
extern
int lc3bin_bread_data(bstream_t *fp, int nchannels, void *buffer);

/**
 * Write LC3 binary header
 */
extern
int lc3bin_bwrite_header(bstream_t *fp,
                        int frame_us,
                        int srate_hz,
                        int bitrate,
                        int nchannels,
                        int nsamples);

/**
 * Write LC3 block of data
 */
extern
int lc3bin_bwrite_data(bstream_t *fp,
                       const void *data,
                       const int nchannels,
                       const int frame_bytes);


#ifndef MIN
#define MIN(a, b)  ( (a) < (b) ? (a) : (b) )
#endif

#endif /* __LC3BIN_H */
