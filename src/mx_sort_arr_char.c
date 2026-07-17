#include "header.h"

//int mx_strcmp(const char * s1, const char * s2);

void mx_sort_arr_char(char *arr[], int size) {
	char *bulb;

	for (int i = 0; i < size - 1; i++) { 				//why size - 1?
		for (int j = 0; j < size - i - 1; j++) { 	//why size - i - 1?
            if (mx_strcmp(arr[j], arr[j + 1]) < 0) {
                bulb = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = bulb;
            }
		}
	}
}

// int main(void) {
// 	int size = 10;
// 	int arr[] = {-34, 3, 55, -11, 1, 45, 0, 22, 5, -2};
// 	for (int i = 0; i < size; i++)
// 		printf("%d ", arr[i]);
// 	printf("\n");
// 	mx_sort_arr_int(arr, size);
// 	for (int i = 0; i < size; i++)
// 		printf("%d ", arr[i]);
// }
