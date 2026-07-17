#include <header.h>

bool mx_isdigit(int c) {
    if (c > 47 && c < 58)
        return 1;
    return 0;
}
bool mx_isspace(char c) {
    if (( c > 8 && c < 14) || c == 32)
        return 1;
    return 0;
}

int mx_atoi(const char *str) {
    unsigned long long int n = 0;
    int i = 0;
    int ch = 1;

    for ( ; mx_isspace(str[i]); i++);
    if (str[i] == '+' || str[i] == '-') {
        if (str[i] == '-')
               ch = -1;
        i++;
        }
    for( ; mx_isdigit(str[i]); i++) {
        if (ch == -1 && ((n > -(LONG_MIN / 10)) || ((n == -(LONG_MIN / 10))
            && (str[i] - 48 > -(LONG_MIN % 10)))))
            return 0;
        if ((n > (LONG_MAX / 10)) || ((n == (LONG_MAX / 10))
            && str[i] - 48 > LONG_MAX % 10 && ch != -1))
            return -1;
        n = n * 10 + str[i] - 48;
    }
    return ch * n;
}
