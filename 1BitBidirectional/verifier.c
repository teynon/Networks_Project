#include<stdio.h>
#include<string.h>

#define polyLength strlen(poly)

char data[100], check[100], poly[100];
int i, j, k;

void crc() {
	for (j = 0; j < polyLength; j++)
		check[j] = data[j];
	do {
		if (check[0] == '1')
			for (k = 0; k < polyLength - 1; k++)
				check[k] = ((check[k] == poly[k]) ? '0' : '1');

		for (k = 0; k < polyLength - 1; k++)
			check[k] = check[k + 1];

		check[k] = data[j++];
	} while (j <= i + polyLength - 1);
}

int main() {
	printf("\nEnter data : ");
	scanf("%s", data);
	printf("\nEnter polynomial : ");
	scanf("%s", poly);

	i = strlen(data);

	printf("\nEnter Message : ");
	scanf("%s", data);

	crc();

	for (j = 0; (j < polyLength - 1) && (check[j] != '1'); j++);

	if (j < polyLength - 1)
		printf("\nAn Error has Occurred\n");
	else
		printf("\nNo Error has Occurred\n");

	printf("\n\n");
	return 0;
}
