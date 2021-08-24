#ifndef ESPRESSO_PORT_H
#define ESPRESSO_PORT_H

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#define NIL(type)        ((type *)0)
#define ALLOC(type, num) ((type *)malloc(sizeof(type) * (num)))
#define REALLOC(type, obj, num)                                    \
    (obj) ? ((type *)realloc((char *)(obj), sizeof(type) * (num))) \
          : ((type *)malloc(sizeof(type) * (num)))
#define FREE(obj)                  \
    if ((obj)) {                   \
        (void)free((char *)(obj)); \
        (obj) = 0;                 \
    }

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* MAX */
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* MIN */
#ifndef ABS
#define ABS(a) ((a) > 0 ? (a) : -(a))
#endif /* ABS */

#endif  // ESPRESSO_PORT_H
