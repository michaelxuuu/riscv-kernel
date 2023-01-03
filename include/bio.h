#ifndef _bio_h_
#define _bio_h_

#include "../include/types.h"

#define BSIZE 1024

typedef struct buf buf_t;
struct buf {
  bool valid;   // has data been read from disk?
  bool disk;    // does disk "own" buf?
  uint32_t dev;
  uint32_t blockno;
  // sleep lock
  uint32_t refct;
  buf_t *prev;
  buf_t *next;
  char data[1024];
};

typedef struct bio{
    void (*init)(void);
    buf_t* (*bread)(uint32_t dev, uint32_t blockno);
    void (*write)(buf_t*);
    void (*brelease)(buf_t*);
} bio_t;

extern bio_t bio;

#endif
