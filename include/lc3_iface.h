#ifndef LC3_IFACE_H
#define LC3_IFACE_H
#include "stdint.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct ilc3_coder {
    uint32_t bitrate;
    uint32_t samplesiz;
    uint16_t srate_hz;
    float frame_us;
    uint8_t nch;
} ilc3_coder_t;

typedef enum {
    ILC3_OK = 0,
    ILC3_BAD_ARG = -1,
    ILC3_BAD_INOUT = -2,
} ilc3_res_t;

#define LC3_RES_IS_OK(__RES__)      (ILC3_OK <= (__RES__))
#define LC3_RES_IS_ERR(__RES__)      (ILC3_OK > (__RES__))

/**
 * init encoder/decoder struct
*/
extern
ilc3_res_t lc3_coder_init(ilc3_coder_t * const enc,
                          const uint32_t bitrate,
                          const uint32_t samplesiz,
                          const uint16_t srate_hz,
                          const uint8_t nch,
                          const float frame_us);

/**
 * convert 'fin' wave to lc3 and put all data to 'fout'
 * 
 * @param encoder - [in]
 * @param fin - [in] input file name or NULL for use stdin
 * @param fout - [in] output file name or NULL for use stdout
 * 
 * @return error codes
*/
extern
ilc3_res_t file_wav_to_lc3(const ilc3_coder_t * const enc,
                           const char * const fin,
                           const char * const fout);

/**
 * convert 'fin' lc3 file to wav and put all data to 'fout'
 * return error codes
*/
extern
ilc3_res_t file_lc3_to_wav(ilc3_coder_t * const dec,
                           const char * const fin,
                           const char * const fout);

/**
 * convert given wave stream to lc3 and put all data to out stream
 * return error codes
*/
extern
ilc3_res_t stream_to_lc3(ilc3_coder_t * const encoder,
                         const uint8_t * const fin,
                         const uint32_t in_siz,
                         uint8_t * const fout,
                         const uint32_t out_siz,
                         const bool with_hdr);


/**
 * convert given lc3 to wave stream and put all data to out stream
 * return error codes
*/
extern
ilc3_res_t lc3_to_stream(ilc3_coder_t * const dec,
                         const uint8_t * const in,
                         const uint32_t in_siz,
                         uint8_t * const out,
                         const uint32_t out_siz);

#endif//LC3_IFACE_H