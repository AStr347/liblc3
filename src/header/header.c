#include "lc3_header.h"
#include "lc3.h"
#include <string.h>
#include "log.h"


/**
 * @brief try read header from byte array
 * @param dtr - [in] data to read
 * @param siz - [in] sizeof data
 * @param hdr - [out] header for fill
 * 
 * @return  0  = ok
 *          -1 = not enought dtr size
 *          -2 = not lc3 header
*/
int lc3bin_header_from_bytes(const uint8_t * const dtr,
                             const uint8_t siz,
                             struct lc3bin_header * const hdr)
{
    if(LC3_HDR_SIZ > siz){
        return -1;
    }
    memcpy(hdr, dtr, LC3_HDR_SIZ);
    if(LC3_FILE_ID != hdr->file_id){
        memset(hdr, 0, LC3_HDR_SIZ);
        return -2;
    }
    
    return 0;
}


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
int lc3bin_header_init(const int frame_us,
                       const int srate_hz,
                       const int bitrate,
                       const int nchannels,
                       const int nsamples,
                       struct lc3bin_header * const hdr)
{
    if(false == LC3_CHECK_SR_HZ(srate_hz)){
        ERROR("bad srate_hz %d\n", srate_hz);
        return -1;
    }
    if(false == LC3_CHECK_DT_US(frame_us)){
        ERROR("bad frame_us %d\n", frame_us);
        return -2;
    }
    
    int wbitrate = bitrate;
    if(LC3_MIN_BITRATE > bitrate){
        wbitrate = LC3_MIN_BITRATE;
    }
    if(LC3_MAX_BITRATE < bitrate){
        wbitrate = LC3_MAX_BITRATE;
    }

    hdr->file_id = LC3_FILE_ID,
    hdr->header_size = LC3_HDR_SIZ;
    hdr->srate_100hz = srate_hz / 100;
    hdr->bitrate_100bps = wbitrate / 100;
    hdr->channels = nchannels;
    hdr->frame_10us = frame_us / 10;
    hdr->nsamples_low = nsamples & 0xffff;
    hdr->nsamples_high = nsamples >> 16;

    return 0;
}