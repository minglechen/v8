#include <stdint.h>

#include <ostream>

inline std::ostream &operator<<(std::ostream &out, uintptr_t val)
{
    return out << static_cast<size_t>(val);
}

inline std::ostream &operator<<(std::ostream &out, intptr_t val)
{
    return out << static_cast<ssize_t>(val);
}
