#ifndef _kpanic_h_
#define _kpanic_h_

#include "types.h"

extern bool paniced;

void kpanic(const char *info);

#endif
