#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <stdint.h>

struct __attribute__((__packed__)) RequestId {
  uint16_t id;
};

struct __attribute__((__packed__)) ClientRequest {
  uint32_t begin_address;
  uint32_t number_bytes;
  uint16_t name_len;
};

struct __attribute__((__packed__)) DirList {
  uint16_t id;
  uint32_t len;
};

#endif //STRUCTURES_H
