#include "wave.h"
#include <string.h>

/**
 * @brief read wave header from given byte array
 * 
 * @param data - [in] input buffer
 * @param siz - [in] sizeof(data)
 * @param hdr - [out] wave header for fill
 * 
 * @return 0 = ok, -1 = not enought size, -2 = bad header
*/
int wave_header_read(const uint8_t * const data,
                     const uint32_t siz,
                     struct wave_header * const hdr)
{
    if(WAVE_HEADER_SIZ > siz){
        return -1;
    }
    memcpy(hdr, data, WAVE_HEADER_SIZ);

    const struct wave_file * const file = &hdr->file;
    const struct wave_format * const format = &hdr->format;
    const struct wave_data * const hdata = &hdr->data;

    if (file->type_id != WAVE_FILE_TYPE_ID || file->fmt_id  != WAVE_FILE_FMT_ID){
        return -2;
    }

    const uint32_t calcbyterate = (format->samplerate * format->framesize);
    if ((format->id != WAVE_FORMAT_ID) || (format->fmt != WAVE_FORMAT_PCM) ||
        (format->channels <= 0) || (format->samplerate <= 0) ||
        (format->framesize  <= 0) || (format->byterate != calcbyterate) ||
        ((16 != format->bitdepth) && (24 != format->bitdepth)))
    {
        return -2;
    }


    if (hdata->id != WAVE_DATA_ID){
        return -2;
    }

    return 0;
}

/**
 * @brief init wave header with given param
 * @param header - [out]
 * 
 * @return always 0
*/
int wave_header_init(const int bitdepth,
                     const int samplesize,
                     const int samplerate,
                     const int nchannels,
                     const int nframes,
                     struct wave_header * const header)
{
    const long data_size = nchannels * nframes * samplesize;
    const long file_size = (WAVE_HEADER_SIZ + data_size);

    header->file = (struct wave_file){
        WAVE_FILE_TYPE_ID, file_size - 8,
        .fmt_id = WAVE_FILE_FMT_ID
    };

    header->format = (struct wave_format){
        .id = WAVE_FORMAT_ID,
        .size = sizeof(struct wave_format) - 8,
        .fmt = WAVE_FORMAT_PCM,
        .channels = nchannels,
        .samplerate = samplerate,
        .byterate = samplerate * nchannels * samplesize,
        .framesize = nchannels * samplesize,
        .bitdepth = bitdepth,
    };

    header->data = (struct wave_data){
        WAVE_DATA_ID,
        data_size
    };

    return 0;
}

