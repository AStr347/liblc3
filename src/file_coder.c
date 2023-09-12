#include <lc3.h>
#include <lc3_iface.h>
#include <lc3_header.h>

#include <stdio.h>
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#include "wave.h"
#include "lc3bin.h"
#include "log.h"


static inline
void file_close(FILE * const f){
    if (stdin != f && stdout != f){
        fclose(f);
    }
}

ilc3_res_t lc3_coder_init(ilc3_coder_t * const enc,
                   uint32_t bitrate,
                   uint32_t samplesiz,
                   uint16_t srate_hz,
                   uint8_t nch,
                   float frame_us)
{
    enc->bitrate = bitrate;
    enc->samplesiz = samplesiz;
    enc->srate_hz = srate_hz;
    enc->nch = nch;
    enc->frame_us = frame_us;

    return ILC3_OK;
}


/**
 * convert 'fin' wave to lc3 and put all data to 'fout'
 * 
 * @param encoder - [in] encoder parametrs
 * @param fin - [in] input file name or NULL for use stdin
 * @param fout - [in] output file name or NULL for use stdout
 * 
 * @return error codes
*/
ilc3_res_t file_wav_to_lc3(const ilc3_coder_t * const encoder,
                           const char * const fin,
                           const char * const fout)
{
    FILE* fp_in = (NULL == fin)? stdin : fopen(fin, "rb");
    if(NULL == fp_in){
        ERROR("can't open %s\n", fin);
        return ILC3_BAD_ARG;
    }

    uint8_t data[WAVE_HEADER_SIZ];
    const int read_res = fread(data, WAVE_HEADER_SIZ, 1, fp_in);
    if(1 != read_res){
        ERROR("can't read %s\n", fin);
        file_close(fp_in);
        return ILC3_BAD_INOUT;
    }

    struct wave_header header;
    const int convert_res = wave_header_read(data,
                                             sizeof(data),
                                             &header);
    if(0 != convert_res){
        ERROR("bad wave header\n");
        file_close(fp_in);
        return ILC3_BAD_ARG;
    }

    const struct wave_format * const format = &header.format;
    const uint32_t framesize = format->framesize;
    const int srate_hz = format->samplerate;
    const int nch = format->channels;
    const int nsamples = header.data.size / framesize;
    const int pcm_sbits = format->bitdepth;
    const int pcm_sbytes = framesize / nch;

    const bool bad_16 = (pcm_sbits == 16 && pcm_sbytes != 16/8);
    const bool bad_24 = (pcm_sbits == 24 && pcm_sbytes != 24/8 && pcm_sbytes != 32/8);
    if (bad_16 || bad_24){
        ERROR("bad wave header\n");
        file_close(fp_in);
        return ILC3_BAD_ARG;
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

    FILE * fp_out = (NULL == fout)? stdout : fopen(fout, "wb");
    if(NULL == fp_out){
        ERROR("can't open %s\n", fout);
        file_close(fp_in);
        return ILC3_BAD_ARG;
    }

    const int frame_us = encoder->frame_us;
    const int bitrate = encoder->bitrate;
    const int header_write_res = lc3bin_fwrite_header(fp_out,
                                                      frame_us,
                                                      enc_srate_hz,
                                                      bitrate,
                                                      nch,
                                                      enc_samples);
    if(0 != header_write_res){
        ERROR("bad lc3 header %d\n", header_write_res);
        file_close(fp_in);
        file_close(fp_out);
        return ILC3_BAD_ARG;
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
        int nread = fread(pcm, nch * pcm_sbytes, frame_samples, fp_in);
        if(0 > nread){
            ERROR("in file not enought data\n");
            for (int ich = 0; ich < nch; ich++){
                free(enc[ich]);
            }

            file_close(fp_in);
            file_close(fp_out);
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

        const int wres = lc3bin_fwrite_data(fp_out, out, nch, frame_bytes);
        if(0 != wres){
            ERROR("can't write lc3 frame to file\n");
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

    file_close(fp_in);
    file_close(fp_out);

    return ILC3_OK;
}

/**
 * convert 'fin' lc3 file to wav and put all data to 'fout'
 * return error codes
*/
ilc3_res_t file_lc3_to_wav(ilc3_coder_t * const decoder,
                           const char * const fin,
                           const char * const fout)
{
    FILE* fp_in = (NULL == fin)? stdin : fopen(fin, "rb");
    if(NULL == fp_in){
        ERROR("can't open %s\n", fin);
        return ILC3_BAD_ARG;
    }

    int frame_us, srate_hz, nch, nsamples;

    if (0 != lc3bin_fread_header(fp_in, &frame_us, &srate_hz, &nch, &nsamples)){
        ERROR("can't read lc3header\n");
        file_close(fp_in);
        return ILC3_BAD_INOUT;
    }

    if (nch  < 1 || nch  > 2){
        ERROR("bad nch\n");
        file_close(fp_in);
        return ILC3_BAD_ARG;
    }

    if (!LC3_CHECK_DT_US(frame_us)){
        ERROR("bad frame_us\n");
        file_close(fp_in);
        return ILC3_BAD_ARG;
    }

    const int dstate_hz = decoder->srate_hz;
    if (!LC3_CHECK_SR_HZ(srate_hz) || (dstate_hz && dstate_hz < srate_hz)){
        ERROR("bad srate_hz\n");
        file_close(fp_in);
        return ILC3_BAD_ARG;
    }

    int pcm_sbits = decoder->samplesiz;
    int pcm_sbytes = pcm_sbits / 8;

    int pcm_srate_hz = !dstate_hz ? srate_hz : dstate_hz;
    int pcm_samples = !dstate_hz ? nsamples :
        ((int64_t)nsamples * pcm_srate_hz) / srate_hz;

    FILE * fp_out = (NULL == fout)? stdout : fopen(fout, "wb");
    if(NULL == fp_out){
        ERROR("can't open %s\n", fout);
        file_close(fp_in);
        return ILC3_BAD_ARG;
    }

    struct wave_header hdr;
    wave_header_init(pcm_sbits,
                     pcm_sbytes,
                     pcm_srate_hz,
                     nch,
                     pcm_samples,
                     &hdr);

    const int header_write_res = fwrite(&hdr, WAVE_HEADER_SIZ, 1, fp_out);
    if(1 != header_write_res){
        file_close(fp_in);
        file_close(fp_out);
        ERROR("can't write header\n");
        return ILC3_BAD_INOUT;
    }

    /* --- Setup decoding --- */

    uint8_t in[2 * LC3_MAX_FRAME_BYTES];
    int8_t alignas(int32_t) pcm[2 * LC3_MAX_FRAME_SAMPLES*4];
    lc3_decoder_t dec[2];

    int frame_samples = lc3_frame_samples(frame_us, pcm_srate_hz);
    int encode_samples = pcm_samples +
        lc3_delay_samples(frame_us, pcm_srate_hz);
    enum lc3_pcm_format pcm_fmt =
        pcm_sbits == 24 ? LC3_PCM_FORMAT_S24_3LE : LC3_PCM_FORMAT_S16;

    for (int ich = 0; ich < nch; ich++)
        dec[ich] = lc3_setup_decoder(frame_us, srate_hz, dstate_hz,
            malloc(lc3_decoder_size(frame_us, pcm_srate_hz)));

    /* --- Decoding loop --- */

    for (int i = 0; i * frame_samples < encode_samples; i++) {
        int frame_bytes = lc3bin_fread_data(fp_in, nch, in);
        if (frame_bytes <= 0)
            memset(pcm, 0, nch * frame_samples * pcm_sbytes);
        else
            for (int ich = 0; ich < nch; ich++)
                lc3_decode(dec[ich],
                    in + ich * frame_bytes, frame_bytes,
                    pcm_fmt, pcm + ich * pcm_sbytes, nch);

        int pcm_offset = i > 0 ? 0 : encode_samples - pcm_samples;
        int pcm_nwrite = MIN(frame_samples - pcm_offset,
            encode_samples - i*frame_samples);

        fwrite(pcm + nch * pcm_offset * pcm_sbytes, nch * pcm_sbytes, pcm_nwrite, fp_out);
    }

    /* --- Cleanup --- */

    for (int ich = 0; ich < nch; ich++){
        free(dec[ich]);
    }

    file_close(fp_in);
    file_close(fp_out);
    return ILC3_OK;
}

