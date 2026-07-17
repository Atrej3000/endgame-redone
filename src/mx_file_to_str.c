#include "header.h"

char *mx_file_to_str(const char *filename) {
    int content;
    char *dst_str;
    int count = 0;
    int i = 0;
    char buf;

    content = open(filename, O_RDONLY);
    if(content < 0)
        return NULL;
    while(read(content, &buf, 1))
        count++;
    dst_str = mx_strnew(count);
    close(content);
    content = open(filename, O_RDONLY);
    while(read(content, &buf, 1))
        dst_str[i++] = buf;
    close(content);
    return dst_str;
}

/*
    int main(int argc, char *argv[]) {
        char *s;

        argc = argc * 1;
        s = mx_file_to_str(argv[1]);
        printf("%s", s);
        //system("leaks -q a.out");
    }
*/
