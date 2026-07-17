#include "header.h"
//#include <stdio.h>
//#include <string.h>

void mx_sort_arr_int(int *arr, int size) {
	int bubble;

	for (int i = 0; i < size - 1; i++) { 				//why size - 1?
		for (int j = 0; j < size - i - 1; j++) { 	//why size - i - 1?
			if(arr[j] < arr[j+1]) {					//дивиться чи наступне число буде більше
				bubble = arr[j];					//міняє місяцми це і наступне
				arr[j] = arr[j+1];					//-ІІ-
				arr[j+1] = bubble;					//-ІІ-
			}
		}
	}
}

 /*int main(void) {
 	int size = 9;
 	int arr[] = {-34, 3, 55, -11, 1, 45, 0, 22, 5, 0};
 	for (int i = 0; i < size; i++)
 		printf("%d ", arr[i]);
 	printf("\n");
 	mx_sort_arr_int(arr, size);
 	for (int i = 0; i < size; i++)
 		printf("%d ", arr[i]);
 }
*/
