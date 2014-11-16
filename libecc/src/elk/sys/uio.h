#ifndef _uio_h_
#define _uio_h_

#define __NEED_struct_iovec
#include <sys/types.h>

typedef struct uio
{
  int iovcnt;
  const struct iovec *iov;
} uio_t;

#endif
