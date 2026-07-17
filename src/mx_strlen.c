#include <stdio.h>

int mx_strlen(const char *s) {
    int lengt = 0;
    while(s[lengt]) {
        lengt++;
    }
    return lengt;
}

/*int main () {
    //mx_strlen("qwerty");
    printf("%d", mx_strlen("qwertysdfghjkjhgcxzxcvbnmkvcvbn000000000000000000000000000000000000000000000:wmkjh1111111111111111111111111gfcvbnm,nbvcvbnm,"));
}*/

