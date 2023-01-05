#ifndef __CHERI_WORKAROUND__
#define __CHERI_WORKAROUND__

#include <stdint.h>
#include <ostream>

namespace cheri 
{
  inline std::ostream &operator<<(std::ostream &out, uintptr_t val);
  inline std::ostream &operator<<(std::ostream &out, intptr_t val);
}

#endif
