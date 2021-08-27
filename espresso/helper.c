/**
 * Helpers for code reading.
 */

#include "espresso.h"

/* ps1 -- convert a set into a printable string */
#define largest_string 120
static char s1[largest_string];
char *ps1(pset a) {
    int i, num, l, len = 0, n = NELEM(a);
    char temp[20];
    bool first = TRUE;

    s1[len++] = '[';
    for (i = 0; i < n; i++)
        if (is_in_set(a, i)) {
            if (!first)
                s1[len++] = ',';
            first = FALSE;
            num = i;
            /* Generate digits (reverse order) */
            l = 0;
            do
                temp[l++] = num % 10 + '0';
            while ((num /= 10) > 0);
            /* Copy them back in correct order */
            do
                s1[len++] = temp[--l];
            while (l > 0);
            if (len > largest_string - 15) {
                s1[len++] = '.';
                s1[len++] = '.';
                s1[len++] = '.';
                break;
            }
        }

    s1[len++] = ']';
    s1[len++] = '\0';
    return s1;
}

/* sf_print -- print a set_family as a set (list the element numbers) */
static char s2[largest_string];
char *sf_print(pset_family A) {
    pset p;
    int i;
    foreachi_set(A, i, p) {
        sprintf(s2 + strlen(s2), "A[%d] = %s\n", i, ps1(p));
    }
    return s2;
}
