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

#include <stdint.h>
#include "lc3bin.h"
#include "lc3_header.h"
#include "log.h"



/**
 * Read LC3 binary header
 */
int lc3bin_fread_header(FILE *fp,
                       int *frame_us,
                       int *srate_hz,
                       int *nchannels,
                       int *nsamples)
{
    uint8_t data[LC3_HDR_SIZ];
    const int read_res = fread(data, LC3_HDR_SIZ, 1, fp);
    if(1 != read_res){
        return -1;
    }

    struct lc3bin_header hdr;
    const int convert_res = lc3bin_header_from_bytes(data, LC3_HDR_SIZ, &hdr);
    if(0 != convert_res){
        return -2;
    }

    *nchannels = hdr.channels;
    *frame_us = hdr.frame_10us * 10;
    *srate_hz = hdr.srate_100hz * 100;
    *nsamples = hdr.nsamples_low | (hdr.nsamples_high << 16);

    fseek(fp, SEEK_SET, hdr.header_size);

    return 0;
}

/**
 * Read LC3 block of data
 */
int lc3bin_fread_data(FILE *fp, int nchannels, void *buffer)
{
    uint16_t nbytes;

    if (fread(&nbytes, sizeof(nbytes), 1, fp) < 1
            || nbytes > nchannels * LC3_MAX_FRAME_BYTES
            || nbytes % nchannels
            || fread(buffer, nbytes, 1, fp) < 1)
        return -1;

    return nbytes / nchannels;
}

/**
 * Write LC3 binary header
 */
int lc3bin_fwrite_header(FILE *fp,
                        int frame_us,
                        int srate_hz,
                        int bitrate,
                        int nchannels,
                        int nsamples)
{
    struct lc3bin_header hdr;
    const int init_res = lc3bin_header_init(frame_us,
                                            srate_hz,
                                            bitrate,
                                            nchannels,
                                            nsamples,
                                            &hdr);
    if(0 != init_res){
        return -1;
    }
    const int write_res = fwrite(&hdr, LC3_HDR_SIZ, 1, fp);
    if(1 != write_res){
        return -2;
    }
    return 0;
}

/**
 * Write LC3 block of data
 */
int lc3bin_fwrite_data(FILE *fp,
                       const void *data,
                       const int nchannels,
                       const int frame_bytes)
{
    const uint16_t nbytes = nchannels * frame_bytes;
    const int cnt_res = fwrite((uint8_t * )&nbytes, sizeof(nbytes), 1, fp);
    if(1 != cnt_res){
        return -1;
    }
    const int data_res = fwrite((uint8_t * )data, 1, nbytes, fp);
    if(nbytes != data_res){
        return -1;
    }
    return 0;
}

/**
 * Read LC3 binary header
 */
int lc3bin_bread_header(bstream_t *fp,
                       int *frame_us,
                       int *srate_hz,
                       int *nchannels,
                       int *nsamples)
{
    uint8_t data[LC3_HDR_SIZ];
    const int read_res = bread(data, LC3_HDR_SIZ, 1, fp);
    if(1 != read_res){
        return -1;
    }

    struct lc3bin_header hdr;
    const int convert_res = lc3bin_header_from_bytes(data, LC3_HDR_SIZ, &hdr);
    if(0 != convert_res){
        return -2;
    }

    *nchannels = hdr.channels;
    *frame_us = hdr.frame_10us * 10;
    *srate_hz = hdr.srate_100hz * 100;
    *nsamples = hdr.nsamples_low | (hdr.nsamples_high << 16);

    return 0;
}

/**
 * Read LC3 block of data
 */
int lc3bin_bread_data(bstream_t *fp, int nchannels, void *buffer)
{
    uint16_t nbytes;

    if (bread((uint8_t * )&nbytes, sizeof(nbytes), 1, fp) < 1
            || nbytes > nchannels * LC3_MAX_FRAME_BYTES
            || nbytes % nchannels
            || bread((uint8_t * )buffer, nbytes, 1, fp) < 1){
        return -1;
    }

    return nbytes / nchannels;
}

/**
 * Write LC3 binary header
 */
int lc3bin_bwrite_header(bstream_t *fp,
                        int frame_us,
                        int srate_hz,
                        int bitrate,
                        int nchannels,
                        int nsamples)
{
    struct lc3bin_header hdr;
    const int init_res = lc3bin_header_init(frame_us,
                                            srate_hz,
                                            bitrate,
                                            nchannels,
                                            nsamples,
                                            &hdr);
    if(0 != init_res){
        ERROR("init_res = %d\n", init_res);
        return -1;
    }
    const int write_res = bwrite((uint8_t * )&hdr, LC3_HDR_SIZ, 1, fp);
    if(1 != write_res){
        ERROR("write_res = %d\n", write_res);
        return -2;
    }
    return 0;
}

/**
 * Write LC3 block of data
 */
int lc3bin_bwrite_data(bstream_t *fp,
                       const void *data,
                       const int nchannels,
                       const int frame_bytes)
{
    const uint16_t nbytes = nchannels * frame_bytes;
    const int cnt_res = bwrite((uint8_t * )&nbytes, sizeof(nbytes), 1, fp);
    if(1 != cnt_res){
        return -1;
    }
    const int data_res = bwrite((uint8_t * )data, 1, nbytes, fp);
    if(nbytes != data_res){
        return -1;
    }
    return 0;
}
