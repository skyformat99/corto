/* Copyright (c) 2010-2017 the corto developers
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <corto/corto.h>

int stricmp(const char *str1, const char *str2) {
    const char *ptr1, *ptr2;
    char ch1, ch2;
    ptr1 = str1;
    ptr2 = str2;

    while((ch1 = *ptr1) && (ch2 = *ptr2)) {
        if (ch1 == ch2) {
            ptr1++; ptr2++;
            continue;
        }
        if (ch1 < 97) ch1 = tolower(ch1);
        if (ch2 < 97) ch2 = tolower(ch2);
        if (ch1 != ch2) {
            return ch1 - ch2;
        }
        ptr1++;
        ptr2++;
    }

    return tolower(*ptr1) - tolower(*ptr2);
}

int strnicmp(const char *str1, int length, const char *str2) {
    const char *ptr1, *ptr2;
    char ch1, ch2;
    ptr1 = str1;
    ptr2 = str2;

    while((ch1 = *ptr1) && (ch2 = *ptr2)) {
        if (ptr1 - str1 >= (length - 1)) break;
        if (ch1 == ch2) {
            ptr1++; ptr2++;
            continue;
        }
        if (ch1 < 97) ch1 = tolower(ch1);
        if (ch2 < 97) ch2 = tolower(ch2);
        if (ch1 != ch2) {
            return ch1 - ch2;
        }
        ptr1++;
        ptr2++;
    }

    return tolower(*ptr1) - tolower(*ptr2);
}

int tokicmp(char ** const str1, const char *str2, char sep) {
    const char *ptr2;
    char ch1, ch2, *ptr1;
    ptr1 = *str1;
    ptr2 = str2;
    int result = 0;

    while((ch1 = *ptr1) && (ch2 = *ptr2) && (ch1 != sep) && (ch2 != sep)) {
        if (ch1 == ch2) {
            ptr1++; ptr2++;
            continue;
        }
        if (ch1 < 97) ch1 = tolower(ch1);
        if (ch2 < 97) ch2 = tolower(ch2);
        if (ch1 != ch2) {
            return ch1 - ch2;
        }
        ptr1++;
        ptr2++;
    }

    result = tolower(*ptr1) - tolower(*ptr2);
    if (!*ptr2 && (*ptr1 == sep)) result = 0;
    if (!*ptr1 && (*ptr2 == sep)) result = 0;
    if ((*ptr1 == sep) && (*ptr2 == sep)) result = 0;

    if (!result) {
        *str1 = ptr1;
    }

    return result;
}

/* Case insensitive string compare, stop at / instead of \0. Returns next
 * element, NULL when no match or "\0" when reached the end */
char* corto_elemcmp(char *path, char *elem) {
    char *pathptr, *elemptr;
    char pathch, elemch;
    pathptr = path;
    elemptr = elem;

    while((pathch = *pathptr) && (elemch = *elemptr)) {
        if (pathch == '/') {
           if (!elemch) break;
           else return NULL;
        }
        if (pathch == elemch) {
            pathptr++; elemptr++;
            continue;
        }
        if (pathch < 97) pathch = tolower(pathch);
        if (elemch < 97) elemch = tolower(elemch);
        if (pathch != elemch) {
            return NULL;
        }
        pathptr++;
        elemptr++;
    }

    return pathptr;
}

char *corto_strelem(char *str) {
    char *ptr, ch;
    for (ptr = str; (ch = *ptr); ptr++) {
        if (ch == '(' || ch == '/') {
            break;
        }
    }
    return ch ? ptr : NULL;
}

char *strappend(char *src, char *fmt, ...) {
    char buff[1024];
    va_list args;
    unsigned int sourceLength = 0;

    va_start(args, fmt);
    vsprintf(buff, fmt, args);
    va_end(args);

    if (src) {
        sourceLength = strlen(src);
    }

    src = corto_realloc(src, sourceLength + strlen(buff) + 1);
    strcpy(&src[sourceLength], buff);

    return src;
}

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
char *itostr(int value, char *result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

char *utostr(unsigned int value, char *result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}


char *chresc(char *out, char in, char delimiter) {
    char *bptr = out;
    switch(in) {
    case '\a':
        *bptr++ = '\\';
        *bptr = 'a';
        break;
    case '\b':
        *bptr++ = '\\';
        *bptr = 'b';
        break;
    case '\f':
        *bptr++ = '\\';
        *bptr = 'f';
        break;
    case '\n':
        *bptr++ = '\\';
        *bptr = 'n';
        break;
    case '\r':
        *bptr++ = '\\';
        *bptr = 'r';
        break;
    case '\t':
        *bptr++ = '\\';
        *bptr = 't';
        break;
    case '\v':
        *bptr++ = '\\';
        *bptr = 'v';
        break;
    case '\\':
        *bptr++ = '\\';
        *bptr = '\\';
        break;
    default:
        if (in == delimiter) {
            *bptr++ = '\\';
            *bptr = delimiter;
        } else {
            *bptr = in;
        }
        break;
    }

    *(++bptr) = '\0';

    return bptr;
}

size_t stresc(char *out, size_t n, const char *in) {
    const char *ptr = in;
    char ch, *bptr = out, buff[3];
    size_t written = 0;
    while ((ch = *ptr++)) {
        if ((written += (chresc(buff, ch, '"') - buff)) <= n) {
            *bptr++ = buff[0];
            if ((ch = buff[1])) {
                *bptr = ch;
                bptr++;
            }
        }
    }

    if (bptr) {
        while (written < n) {
            *bptr = '\0';
            bptr++;
            written++;
        }
    }
    return written;
}

/* Count the number of characters in a string that do not match a provided
 * mask. */
size_t strmask(char *str, char *mask) {
    char maskbuff[256];
    memset(maskbuff, 0, sizeof(maskbuff));
    char ch, *ptr;
    size_t result = 0;

    /* Set mask */
    ptr = mask;
    while ((ch = *ptr)) {
        maskbuff[(int)ch] = 1;
        ptr++;
    }

    /* Loop string, count mismatches */
    ptr = str;
    while ((ch = *ptr)) {
        if (!maskbuff[(int)ch]) {
            result++;
        }
        ptr++;
    }

    return result;
}

/* strdup is not a standard C function, so provide own implementation. */
char* corto_strdup(const char* str) {
    char *result = corto_alloc(strlen(str) + 1);
    strcpy(result, str);
    return result;
}

/**
 * From:
 * `asprintf.c' - asprintf
 *
 * copyright (c) 2014 joseph werle <joseph.werle@gmail.com>
 */

char* corto_asprintf (const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);

    char *result = corto_vasprintf(fmt, args);

    va_end(args);

    return result;
}

char* corto_vasprintf (const char *fmt, va_list args) {
    int size = 0;
    char *result  = NULL;
    va_list tmpa;

    va_copy(tmpa, args);

    size = vsnprintf(NULL, size, fmt, tmpa);

    va_end(tmpa);

    if (size < 0) { return NULL; }

    // alloc with size plus 1 for `\0'
    result = (char *) malloc(size + 1);

    if (!result) { return NULL; }

    vsprintf(result, fmt, args);
    return result;
}

char* strarg(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *tmp = corto_vasprintf(fmt, args);
    va_end(args);
    char *result = corto_setThreadString(tmp);
    free(tmp);
    return result;
}

/* Convert characters in string to uppercase */
char* corto_strupper(char *str) {
    char *ptr, ch;
    ptr = str;
    while ((ch = *ptr)) {
        if (ch >= 97) *ptr = toupper(ch);
        ptr++;
    }
    return str;
}

/* Convert characters in string to lowercase */
char* corto_strlower(char *str) {
    char *ptr, ch;
    ptr = str;
    while ((ch = *ptr)) {
        if (ch < 97) *ptr = tolower(ch);
        ptr++;
    }
    return str;
}

// You must free the result if result is non-NULL.
char* corto_replace(char *orig, char *rep, char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    if (!orig)
        return NULL;
    if (!rep)
        rep = "";
    len_rep = strlen(rep);
    if (!with)
        with = "";
    len_with = strlen(with);

    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count) {
        ins = tmp + len_rep;
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}
