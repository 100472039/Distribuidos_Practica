#include <stdio.h> 
#include <stdlib.h>
#include "list.h"

int main(void) 
{

	List my_list;
	int err = 0;
	int key = 86;
	char value1[MAX_KEY_LENGTH] = "hola";
	int N_value2 = 2;
	double *V_value2 = (double *)malloc(N_value2 * sizeof(double));
	V_value2[0] = 1.1234567890123456;
	V_value2[1] = 2.1;

	err = init(&my_list);
	if (err == -1) {
		printf("Error in init\n");
		return -1;
	}

	for (int i=0; i < 3; i++){
		key = i;
		err = set(&my_list, key, value1, N_value2, V_value2);
		if (err == -1) {
			printf("Error in set\n");
			return -1;
		}
	}
	// set(&my_list, key, value1, N_value2, V_value2);

	printList(my_list);

	char *vacio1[MAX_KEY_LENGTH];
	int N_vacio2;
	double *V_vacio2 = (double *)malloc(200 * sizeof(double));
	get(my_list, key, *vacio1, &N_vacio2, V_vacio2);

	printf("Resultado: %s, %d\n", *vacio1, N_vacio2);
	for(int i = 0; i < N_vacio2; i++){
		printf("V_vacio2[%d]: %f\n", i, V_vacio2[i]);
	}
	printf("\n\n\n\n\n\n");
	delete(&my_list, 2);
	
	printList(my_list);

	destroy(&my_list);

	printf("\n\n\n\n\n");
	printList(my_list);
	// printf("ret: %d\n", ret);

	// for (i=0; i < 10; i++){
	// 	sprintf(key, "%d", i);
	// 	err = delete(&my_list, key);
	// 	if (err == -1) {
	// 		printf("Error in delete\n");
	// 		return -1;
	// 	}
	// }

	// err = destroy(&my_list);
	// if (err == -1) {
	// 	printf("Error in destroy\n");
	// 	return -1;
	// }
	
	return 0;
}