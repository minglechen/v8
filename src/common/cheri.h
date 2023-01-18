#ifndef __CHERI_WORKAROUND__
#define __CHERI_WORKAROUND__

#include <stdint.h>
#include <ostream>

namespace cheri 
{
  inline std::ostream &operator<<(std::ostream &out, uintptr_t val)
  {
    return out << static_cast<size_t>(val);
  }

  inline std::ostream &operator<<(std::ostream &out, intptr_t val)
  {
    return out << static_cast<ssize_t>(val);
  }

  template<typename T>
  struct is_intcap {
    constexpr static bool value = std::is_same<intptr_t, T>::value || std::is_same<uintptr_t, T>::value;
  };
}

#endif
