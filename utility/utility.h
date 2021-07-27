#ifndef UTILITY_H
#define UTILITY_H

/*
 *  assumes the memory manager is libmm.a
 *	- allows malloc(0) or realloc(obj, 0)
 *	- catches out of memory (and calls MMout_of_memory())
 *	- catch free(0) and realloc(0, size) in the macros
 */
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

extern long util_cpu_time();
extern char *util_path_search(char *program);
extern char *util_file_search(char *file, char *path, char *mode);
extern int util_pipefork(char **argv, FILE **toCommand, FILE **fromCommand);
extern int util_csystem(char *command);
extern char *util_print_time(long t);
extern char *util_strsav(char *ptr);
extern char *util_tilde_expand(char *filename);

#ifndef NIL_FN
#define NIL_FN(type) ((type(*)())0)
#endif /* NIL_FN */

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* MAX */
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* MIN */
#ifndef ABS
#define ABS(a) ((a) > 0 ? (a) : -(a))
#endif /* ABS */

#endif
