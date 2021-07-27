#include "copyright.h"
#include "port.h"
#include "utility.h"
#include <sys/time.h>
#include <sys/resource.h>

/*
 *   util_cpu_time -- return a long which represents the elapsed processor
 *   time in milliseconds since some constant reference
 */
long util_cpu_time() {
    long t = 0;

    struct rusage rusage;
    (void)getrusage(RUSAGE_SELF, &rusage);
    t = (long)rusage.ru_utime.tv_sec * 1000 + rusage.ru_utime.tv_usec / 1000;

    return t;
}
