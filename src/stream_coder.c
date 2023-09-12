#include <lc3.h>
#include <lc3_iface.h>
#include <lc3_header.h>

#include <stdio.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#include "wave.h"
#include "bytestream.h"
#include "lc3bin.h"
#include "log.h"

static
int get_params_from_hdr(bstream_t * const fp_in,

                        uint32_t * framesize,
                        int * srate_hz,
                        int * nch,
                        int * nsamples,
                        int * pcm_sbits)
{
    uint8_t data[WAVE_HEADER_SIZ];
    const int read_res = bread(data, WAVE_HEADER_SIZ, 1, fp_in);
    if(1 != read_res){
        ERROR("can't read [%ld] wave header from bstream\n", WAVE_HEADER_SIZ);
        return ILC3_BAD_ARG;
    }

    struct wave_header header;
    const int convert_res = wave_header_read(data,
                                             sizeof(data),
                                             &header);
    if(0 != convert_res){
        ERROR("read bad wave header\n");
        return ILC3_BAD_ARG;
    }

    const struct wave_format * const format = &header.format;

    *framesize = format->framesize;
    *srate_hz = format->samplerate;
    *pcm_sbits = format->bitdepth;
    *nch = format->channels;
    *nsamples = header.data.size / *framesize;

    return ILC3_OK;
}

/**
 * convert given wave stream to lc3 and put all data to out stream
 * return error codes
*/
ilc3_res_t stream_to_lc3(ilc3_coder_t * const encoder,
                         const uint8_t * fin,
                         const uint32_t in_siz,
                         uint8_t * fout,
                         const uint32_t out_siz,
                         const bool with_hdr)
{
    bstream_t f_in;
    bstream_t * const fp_in = &f_in;
    const int bin_init_res = binit(fp_in, fin, in_siz, BM_READ);
    if(0 != bin_init_res){
        ERROR("can't init in bstream\n");
        return ILC3_BAD_ARG;
    }

    uint32_t framesize = 4;
    int srate_hz = 48000;
    int nch = 2;
    int nsamples = 0;
    int pcm_sbits = 16;
    
    if(with_hdr){
        const int res = get_params_from_hdr(fp_in, &framesize, &srate_hz, &nch, &nsamples, &pcm_sbits);
        if(0 != res){
            return res;
        }
    } else {
        nch = encoder->nch;
        pcm_sbits = encoder->samplesiz;
        framesize = nch * (pcm_sbits / 8);
        srate_hz = encoder->srate_hz;
        nsamples = in_siz / framesize;
    }
    const int pcm_sbytes = framesize / nch;

    const bool bad_16 = (pcm_sbits == 16 && pcm_sbytes != 16/8);
    const bool bad_24 = (pcm_sbits == 24 && pcm_sbytes != 24/8 && pcm_sbytes != 32/8);
    if (bad_16 || bad_24){
        ERROR("not suported samples format bad16[%d] bad24[%d]\n", bad_16, bad_24);
        return ILC3_BAD_INOUT;
    }

    int enc_srate_hz = 0;
    int enc_samples = 0;
    const int encoder_srate = encoder->srate_hz;
    if(0 == encoder_srate){
        enc_srate_hz = srate_hz;
        enc_samples = nsamples;
    } else {
        enc_srate_hz = encoder_srate;
        enc_samples = ((int64_t)nsamples * enc_srate_hz) / srate_hz;
    }

    bstream_t f_out;
    bstream_t * const fp_out = &f_out;
    const int bout_init_res = binit(fp_out, fout, out_siz, BM_WRITE);
    if(0 != bout_init_res){
        ERROR("can't init out bstream\n");
        return ILC3_BAD_ARG;
    }

    const int frame_us = encoder->frame_us;
    const int bitrate = encoder->bitrate;
    const int header_write_res = lc3bin_bwrite_header(fp_out,
                                                      frame_us,
                                                      enc_srate_hz,
                                                      bitrate,
                                                      nch,
                                                      enc_samples);
    if(0 != header_write_res){
        ERROR("can't write lc3 header to bstream [%d]\n", header_write_res);
        return ILC3_BAD_INOUT;
    }
   
    /* --- Setup encoding --- */

    int8_t alignas(int32_t) pcm[2 * LC3_MAX_FRAME_SAMPLES*4];
    uint8_t out[2 * LC3_MAX_FRAME_BYTES];
    lc3_encoder_t enc[2];

    const int frame_bytes = lc3_frame_bytes(frame_us, bitrate / nch);
    const int frame_samples = lc3_frame_samples(frame_us, srate_hz);
    const int encode_samples = nsamples + lc3_delay_samples(frame_us, srate_hz);
    const enum lc3_pcm_format pcm_fmt =
        pcm_sbytes == 32/8 ? LC3_PCM_FORMAT_S24 :
        pcm_sbytes == 24/8 ? LC3_PCM_FORMAT_S24_3LE : LC3_PCM_FORMAT_S16;

    const uint32_t encoder_size = lc3_encoder_size(frame_us, srate_hz);
    for (int ich = 0; ich < nch; ich++){
        enc[ich] = lc3_setup_encoder(frame_us,
                                     enc_srate_hz,
                                     srate_hz,
                                     malloc(encoder_size));
    }

    
    /* --- Encoding loop --- */

    for (int i = 0; i * frame_samples < encode_samples; i++) {
        int nread = bread((uint8_t*)pcm, nch * pcm_sbytes, frame_samples, fp_in);
        if(0 > nread){
            ERROR("in bstream not enought data\n");
            for (int ich = 0; ich < nch; ich++){
                free(enc[ich]);
            }
            return ILC3_BAD_INOUT;
        }

        memset(pcm + nread * nch * pcm_sbytes, 0,
            nch * (frame_samples - nread) * pcm_sbytes);

        for (int ich = 0; ich < nch; ich++){
            lc3_encode(enc[ich],
                       pcm_fmt,
                       pcm + ich * pcm_sbytes, nch,
                       frame_bytes,
                       out + ich * frame_bytes);
        }

        const int wres = lc3bin_bwrite_data(fp_out, out, nch, frame_bytes);
        if(0 != wres){
            ERROR("in bstream not enought space\n");
            for (int ich = 0; ich < nch; ich++){
                free(enc[ich]);
            }
            return ILC3_BAD_INOUT;
        }
    }
    /* --- Cleanup --- */

    for (int ich = 0; ich < nch; ich++){
        free(enc[ich]);
    }

    return ILC3_OK + (out_siz - fp_out->bsiz);
}


/**
 * convert given lc3 to wave stream and put all data to out stream
 * return error codes
*/
ilc3_res_t lc3_to_stream(ilc3_coder_t * const decoder,
                         const uint8_t * const fin,
                         const uint32_t in_siz,
                         uint8_t * const fout,
                         const uint32_t out_siz)
{
    bstream_t f_in;
    bstream_t * const fp_in = &f_in;
    const int bin_init_res = binit(fp_in, fin, in_siz, BM_READ);
    if(0 != bin_init_res){
        return ILC3_BAD_ARG;
    }

    int frame_us, srate_hz, nch, nsamples;

    if (0 != lc3bin_bread_header(fp_in, &frame_us, &srate_hz, &nch, &nsamples)){
        return ILC3_BAD_ARG;
    }

    if (nch  < 1 || nch  > 2){
        return ILC3_BAD_ARG;
    }

    if (!LC3_CHECK_DT_US(frame_us)){
        return ILC3_BAD_ARG;
    }

    const int dstate_hz = decoder->srate_hz;
    if (!LC3_CHECK_SR_HZ(srate_hz) || (dstate_hz && dstate_hz < srate_hz)){
        return ILC3_BAD_ARG;
    }

    int pcm_sbits = decoder->samplesiz;
    int pcm_sbytes = pcm_sbits / 8;

    int pcm_srate_hz = !dstate_hz ? srate_hz : dstate_hz;
    int pcm_samples = !dstate_hz ? nsamples :
        ((int64_t)nsamples * pcm_srate_hz) / srate_hz;

    bstream_t f_out;
    bstream_t * const fp_out = &f_out;
    const int bout_init_res = binit(fp_out, fout, out_siz, BM_WRITE);
    if(0 != bout_init_res){
        return ILC3_BAD_ARG;
    }

    struct wave_header hdr;
    wave_header_init(pcm_sbits,
                     pcm_sbytes,
                     pcm_srate_hz,
                     nch,
                     pcm_samples,
                     &hdr);

    const int header_write_res = bwrite((uint8_t * )&hdr, WAVE_HEADER_SIZ, 1, fp_out);
    if(1 != header_write_res){
        return ILC3_BAD_ARG;
    }

    /* --- Setup decoding --- */

    uint8_t in[2 * LC3_MAX_FRAME_BYTES];
    int8_t alignas(int32_t) pcm[2 * LC3_MAX_FRAME_SAMPLES*4];
    lc3_decoder_t dec[2];

    int frame_samples = lc3_frame_samples(frame_us, pcm_srate_hz);
    int encode_samples = pcm_samples + lc3_delay_samples(frame_us, pcm_srate_hz);
    enum lc3_pcm_format pcm_fmt = (pcm_sbits == 24) ? LC3_PCM_FORMAT_S24_3LE : LC3_PCM_FORMAT_S16;

    for (int ich = 0; ich < nch; ich++){
        dec[ich] = lc3_setup_decoder(frame_us,
                                     srate_hz,
                                     dstate_hz,
                                     malloc(lc3_decoder_size(frame_us, pcm_srate_hz)));
    }

    /* --- Decoding loop --- */

    for (int i = 0; i * frame_samples < encode_samples; i++) {
        int frame_bytes = lc3bin_bread_data(fp_in, nch, in);
        if (frame_bytes <= 0){
            memset(pcm, 0, nch * frame_samples * pcm_sbytes);
        } else {
            for (int ich = 0; ich < nch; ich++){
                lc3_decode(dec[ich],
                           in + ich * frame_bytes,
                           frame_bytes,
                           pcm_fmt,
                           pcm + ich * pcm_sbytes,
                           nch);
            }
        }

        int pcm_offset = i > 0 ? 0 : encode_samples - pcm_samples;
        int pcm_nwrite = MIN((frame_samples - pcm_offset), (encode_samples - i*frame_samples));

        
        const int wres = bwrite((uint8_t *)pcm + nch * pcm_offset * pcm_sbytes, nch * pcm_sbytes, pcm_nwrite, fp_out);
        if(0 > wres){
            ERROR("in bstream not enought space\n");
            for (int ich = 0; ich < nch; ich++){
                free(dec[ich]);
            }
            return ILC3_BAD_INOUT;
        }
    }

    /* --- Cleanup --- */

    for (int ich = 0; ich < nch; ich++){
        free(dec[ich]);
    }

    return ILC3_OK + (out_siz - fp_out->bsiz);
}


