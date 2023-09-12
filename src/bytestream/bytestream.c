#include "bytestream.h"
#include <stdlib.h>
#include <string.h>

int binit(bstream_t * stream,
          const uint8_t * fin,
          const uint32_t in_siz,
          const bmode_t mode)
{
    if(NULL == fin || 0 == in_siz || BM_NONE == mode || BM_WRITE < mode){
        return -1;
    }
    stream->b = (uint8_t *)fin;
    stream->bsiz = in_siz;
    stream->mode = mode;
    return 0;
}

int bread(uint8_t * const fill,
          const uint32_t obj_siz,
          const uint32_t obj_cnt,
          bstream_t * const stream)
{
    if(BM_READ != stream->mode){
        return -1;
    }
    const uint32_t can_read_cnt = stream->bsiz / obj_siz;
    if(0 == can_read_cnt){
        return -2;
    }
    const uint32_t read_cnt = (can_read_cnt < obj_cnt)? can_read_cnt : obj_cnt;
    const uint32_t read_siz = (read_cnt * obj_siz);
    
    memcpy(fill, stream->b, read_siz);

    stream->b += read_siz;
    stream->bsiz -= read_siz;

    return read_cnt;
}

int bwrite(const uint8_t * const data,
           const uint32_t obj_siz,
           const uint32_t obj_cnt,
           bstream_t * const stream)
{
    if(BM_WRITE != stream->mode){
        return -1;
    }
    const uint32_t can_write_cnt = stream->bsiz / obj_siz;
    if(0 == can_write_cnt){
        return -2;
    }
    const uint32_t write_cnt = (can_write_cnt < obj_cnt)? can_write_cnt : obj_cnt;
    const uint32_t write_siz = (write_cnt * obj_siz);
    
    memcpy(stream->b, data, write_siz);

    stream->b += write_siz;
    stream->bsiz -= write_siz;

    return write_cnt;
}