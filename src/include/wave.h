#ifndef LC3_WAVE_H
#define LC3_WAVE_H
#include <stdint.h>

/**
 * Id formatting
 */

#define __WAVE_ID(s) \
    (uint32_t)( s[0] | (s[1] << 8) | (s[2] << 16) | (s[3] << 24) )


/**
 * File format statement
 * | type_id     WAVE_FILE_TYPE_ID
 * | size        File size - 8 bytes
 * | type_id     WAVE_FILE_FMT_ID
 */

#define WAVE_FILE_TYPE_ID  __WAVE_ID("RIFF")
#define WAVE_FILE_FMT_ID   __WAVE_ID("WAVE")

struct wave_file {
    uint32_t type_id;
    uint32_t size;
    uint32_t fmt_id;
};


/**
 * Audio format statement
 * | id          WAVE_FORMAT_ID
 * | size        Size of the block - 8 bytes (= 16 bytes)
 * | format      WAVE_FORMAT_PCM
 * | channels    Number of channels
 * | samplerate  Sampling rate
 * | byterate    Bytes per secondes = `samplerate * framesize`
 * | framesize   Bytes per sampling time = `channels * bitdepth / 8`
 * | bitdepth    Number of bits per sample
 */

#define WAVE_FORMAT_ID   __WAVE_ID("fmt ")
#define WAVE_FORMAT_PCM  1

struct wave_format {
    uint32_t id;
    uint32_t size;
    uint16_t fmt;
    uint16_t channels;
    uint32_t samplerate;
    uint32_t byterate;
    uint16_t framesize;
    uint16_t bitdepth;
};


/**
 * Audio data statement
 * | id          WAV_DATA_ID
 * | size        Size of the data following
 */

#define WAVE_DATA_ID  __WAVE_ID("data")

struct wave_data {
    uint32_t id;
    uint32_t size;
};

struct wave_header {
    struct wave_file file;
    struct wave_format format;
    struct wave_data data;
};

#define WAVE_HEADER_SIZ     (sizeof(struct wave_header))


/**
 * @brief read wave header from given byte array
 * 
 * @param data - [in] input buffer
 * @param siz - [in] sizeof(data)
 * @param hdr - [out] wave header for fill
 * 
 * @return 0 = ok, -1 = not enought size, -2 = bad header
*/
extern
int wave_header_read(const uint8_t * const data,
                     const uint32_t siz,
                     struct wave_header * const hdr);

/**
 * @brief init wave header with given param
 * @param header - [out]
 * 
 * @return always 0
*/
extern
int wave_header_init(const int bitdepth,
                     const int samplesize,
                     const int samplerate,
                     const int nchannels,
                     const int nframes,
                     struct wave_header * const header);

#endif//LC3_WAVE_H