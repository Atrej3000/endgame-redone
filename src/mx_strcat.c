#include "header.h"

char *mx_strcat(char *s1, const char *s2) {
	int count = 0, i = 0;
	count = mx_strlen(s1);
	for(i = 0; i < mx_strlen(s2); i++) {
		s1[i+count] = s2[i];
	}
	s1[i+count] = '\0';
	return s1;
}

// int main(void) {
// 	char s1[] = "34rtghu7";
// 	char s2[] = "";
// 	printf("%s", mx_strcat(s1, s2));
// 	//printf("%s", s3);
// }
