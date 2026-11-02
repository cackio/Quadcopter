#ifndef TIMEBASE_H
#define TIMEBASE_H
#include <stdint.h>
/* Valid for intervals shorter than one 32-bit microsecond wrap (~71.6 min). */
static inline uint32_t Time_ElapsedUs(uint32_t now, uint32_t before)
{
    return now - before;
}
/* Latch expiration across subsequent counter wraps. Only a new producer
 * sample may set the validity flag back to one. */
static inline void Time_ExpireUs(uint32_t now, uint32_t before, uint32_t limit, uint8_t *valid)
{
    if (Time_ElapsedUs(now, before) > limit)
        *valid = 0;
}
#endif
