#ifndef BYTESTREAM_H
#define BYTESTREAM_H

#include <stdint.h>

typedef enum {
    BM_NONE = 0,
    BM_READ,
    BM_WRITE,
} bmode_t;

typedef struct {
    uint8_t * b;
    uint32_t bsiz;
    bmode_t mode;
} bstream_t;

extern
int binit(bstream_t * stream,
          const uint8_t * fin,
          const uint32_t in_siz,
          const bmode_t mode);

extern
int bread(uint8_t * const fill,
          const uint32_t obj_siz,
          const uint32_t obj_cnt,
          bstream_t * const stream);

extern
int bwrite(const uint8_t * const data,
           const uint32_t obj_siz,
           const uint32_t obj_cnt,
           bstream_t * const stream);

#endif//BYTESTREAM_H