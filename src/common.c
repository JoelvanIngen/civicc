// src/common.c

#include "common.h"

/**
 * Concatenates two strings and frees them
 * @param s1 first string
 * @param s2 second string
 * @return concatenated strings
 */
char* safe_concat_str(char* s1, char* s2) {
    char* buf = MEMmalloc(MAX_STR_LEN);
    snprintf(buf, MAX_STR_LEN, "%s%s", s1, s2);
    MEMfree(s1);
    MEMfree(s2);
    return buf;
}
