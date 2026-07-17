#include "header.h"

int mx_strcmp(const char * s1, const char * s2) {
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
}
	return (unsigned char) *s1 - (unsigned char) *s2;
}

// int main (void) {
// 	char s1[] = "";
// 	char s2[] = "";
// 	printf("%d", mx_strcmp(s1, s2));
// }
