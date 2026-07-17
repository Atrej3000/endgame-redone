#include "header.h"

char *mx_strcpy(char *dst, const char *src) {
    int i;
    if(src == NULL)
        return NULL;

    for(i = 0; src[i] != '\0'; i++)
        dst[i] = src[i];
    dst[i] = '\0';
    return dst;
}

// int main (void) {
// 	char *src = NULL;
// 	char dst[1000];
// //	printf ("%s", *mx_strcpy(*dst, *src));
// 	mx_strcpy(dst, src);
// 	printf("%s", dst);
// }
