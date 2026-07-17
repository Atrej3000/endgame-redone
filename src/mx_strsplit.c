#include "header.h"

char **mx_strsplit(char const *s, char c) {
    char **arr;
    int amm_words;
    int key = 0;
    int j = 0;
    int lengt_of_word = 0;

    if(s == NULL)
        return NULL;
    amm_words = mx_count_words(s, c);
    arr = malloc(sizeof(char *) * (amm_words + 1));
    for(int i = 0; s[i]; i++) {
        if(s[i] != c && lengt_of_word == 0)//визначаємо індекс для релевантного масиву строк
            key = i;                       //відповідним номером символа у строці
        if(s[i] != c && s[i] != 0)
            lengt_of_word++;//рахуємо кільксть символів у слові від першого недиліметра до нуля
        if(lengt_of_word > 0 && (s[i+1] == c || s[i + 1] == 0)) {
            arr[j] = mx_strnew(lengt_of_word);//виділяємо память під масив відповідно до кількості символів
            arr[j] = mx_strncpy(arr[j], &s[key],lengt_of_word);//копіюємо з відповідного символу у
                                                               // строчці стількі символів скільки вийшла довжина
            lengt_of_word = 0;//обнуляємо лічильник
            j++;//рухаємося по строці
        }
    }
    arr[j] = NULL;//why?
    return arr;
}

/*
int main(void) {
    char *s = "**Good bye,**Mr.*Anderson.****";
    char **res;
    char delimetr = '*';

    res = mx_strsplit(s, delimetr);
    for(int i = 0; i < mx_count_words(s, delimetr); i++) {
        printf("%s\n", res[i]);
    }
}
*/
