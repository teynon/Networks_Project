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
			for (k = 1; k < polyLength; k++)
				check[k] = ((check[k] == poly[k]) ? '0' : '1');
		for (k = 0; k < polyLength - 1; k++)
			check[k] = check[k + 1];
		check[k] = data[j++];
	}
	while (j <= i + polyLength - 1);
}

int main() {
	printf("\nEnter data : ");
	scanf("%s", data);
	printf("\nEnter polynomial : ");
	scanf("%s", poly);

	i = strlen(data);

	for (j = i; j < i + polyLength - 1; j++)
		data[j] = '0';

	crc();

	for (j = i; j < i + polyLength - 1; j++)
		data[j] = check[j - i];

	printf("\nMessage to Transmit : %s", data);
	printf("\nPolynomial : %s\n", poly);
	printf("\n");

	return 0;
}
