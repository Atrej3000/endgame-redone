#include "header.h"

int mx_count_words(const char *str, char delimiter) {
		int i = 0;
		int count = 0;
        int temp = 0;

        if(!str[i]) return 0;
        for (i = 0; str[i] != '\0'; i++) {
            if (str[i] != delimiter) {
                temp = 1;
            }
            if (str[i] == delimiter && temp) {
                temp = 0;
                count++;
            }
        }
		if (str[i-1] != delimiter)
		    count++;
		return count;
	}

//int main(void) {
//	char delimiter = '*';
//
//	char str[] = "*WTF *are going   ** here**";
//	printf("%c are: %d\n", delimiter, mx_count_words(str, delimiter));
//
//	 char str1[] = " WTF *are going   * here ";
//	 printf("%c are: %d\n", delimiter, mx_count_words(str1, delimiter));
//
//	 char str2[] = " WTF *are going   * here*";
//	 printf("%c are: %d\n", delimiter, mx_count_words(str2, delimiter));
//
//	 char str3[] = " WTF *are go********   ** ***ing   * here ";
//	 printf("%c are: %d\n", delimiter, mx_count_words(str3, delimiter));
//
//
//}
