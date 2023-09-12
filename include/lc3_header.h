#ifndef LC3_HEADER_H
#define LC3_HEADER_H
#include <stdint.h>

/**
 * LC3 binary header
 */

#define LC3_FILE_ID (0x1C | (0xCC << 8))

struct lc3bin_header {
    uint16_t file_id;
    uint16_t header_size;
    uint16_t srate_100hz;
    uint16_t bitrate_100bps;
    uint16_t channels;
    uint16_t frame_10us;
    uint16_t rfu;
    uint16_t nsamples_low;
    uint16_t nsamples_high;
};

#define LC3_HDR_SIZ     (sizeof(struct lc3bin_header))



/**
 * @brief try read header from byte array
 * 
 * @return  0  = ok
 *          -1 = not enought dtr size
 *          -2 = note lc3 header
*/
extern
int lc3bin_header_from_bytes(const uint8_t * const dtr,
                             const uint8_t siz,
                             struct lc3bin_header * const hdr);


/**
 * @brief fill header by given values
 * 
 * @param frame_us - Frame duration, in us
 * @param srate_hz - Samplerate, in Hz
 * @param bitrate - Bitrate indication of the stream, in bps
 * @param nchannels - Number of channels
 * @param nsamples - Count of source samples by channels
 * 
 * @return  0  = ok
 *          -1 = unsuported samplerate
 *          -2 = unsuported frame duration
*/
extern
int lc3bin_header_init(const int frame_us,
                       const int srate_hz,
                       const int bitrate,
                       const int nchannels,
                       const int nsamples,
                       struct lc3bin_header * const hdr);

#endif//LC3_HEADER_H