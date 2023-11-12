#include "util.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char **mzd_str_split(const char *str, const char delim, void **ptr) {
    char *s = strdup(str);
    *ptr = s;

    char *p = s;
    int c = 1;
    while ((p = strchr(p, delim))) {
        if (strlen(p) > 1) {
            *p++ = 0;
            c++;
        } else
            *p = 0;
    }

    const char **strings = malloc((c + 1) * sizeof(char *));
    p = s;
    for (int i = 0; i < c; i++) {
        strings[i] = p;
        printf("%s\n", p);
        p = p + strlen(p) + 1;
    }
    strings[c] = 0;

    return strings;
}

// Adapted from http://www.cse.yorku.ca/~oz/hash.html
unsigned long mzd_str_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        if (isupper(c))
            c = c + 32;
        hash = ((hash << 5) + hash) + c; // hash * 33 + c   // hash << 5 = hash * 2^5
    }

    return hash;
}



int mzd_strptr_cmp(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}



void mzd_ptrptr_free(void **ptrptr) {
    for (int i = 0; ptrptr[i]; i++)
        free(ptrptr[i]);
    free(ptrptr);
}

void mzd_strv_free(char **strv) {
    mzd_ptrptr_free((void **) strv);
}
