#include "header.h"

char *mx_strnew(const int size) {
    char *string = NULL;

    if (size < 0)
        return NULL;
    string = (char *)malloc(size + 1);
    for(int i = 0; i < size; i++)
        string[i] = '\0';
    string[size] = '\0';
    return string;
}

//int main(void) {
//    int size = 5;
//    char *string = mx_strnew(size);
//    printf("%s\n", string);
//    free(string);
//    string = NULL;
//    printf("%s\n", string);
//    system("leaks a.out");
//}
