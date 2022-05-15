#include <iostream>
using namespace std;

int main() {
	int *a[2];
	int b[2][3];

	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 3; j++) {
			b[i][j] = i*3 +j;
		}
	a[0] = *b;
	a[1] = *(b+1);

	for (int i = 0; i < 3; i++) {
		cout << *a[0] << *a[1] << endl;
		a[0]++;
		a[1]++;
	}
	return 0;
}
