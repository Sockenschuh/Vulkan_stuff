#include "Simulation_base.h"

int random(int a, int b)
{
	std::random_device rd; // obtain a random number from hardware
	std::mt19937 gen(rd()); // seed the generator
	std::uniform_int_distribution<> distr(a, b); // define the range

	return distr(gen); // generate number
}

uint32_t randomUint32(uint32_t a, uint32_t b) {
	uint32_t x = a / 2;
	uint32_t y = b / 2;
	int z = random(int(x), int(y));
	return uint32_t(z) * 2;
}


int nat_sum(int n) {
	int sum = (n * n + n) / 2;
	return sum;
}


int cloitreSum(int n) {
	int Sum = 0;
	for (int i = 0; i < 1 + int(pow(n, 0.5f)); i++)
	{
		Sum += int(pow(n - pow(i, 2), 0.5f));
	}
	return 1 + 4 * Sum;
}

int diskPixels(int n) {
	return cloitreSum(pow(n, 2) + n);
}

int EigththDiskPixels(int n) {
	return int(round(((11.948f * n) / 14.0f) + (diskPixels(n) / 8.0f)));
}


int aproxDiskRangeEigth(int ID) {
	/*int n = round((pow(ID * 8, 0.5) / 1.772458288966203));
	if (round(diskPixels(n - 1) / 8) > ID) {
		n -= 1;
	}*/
	int dist = int(2.0663 + (pow(pow(134.8694 / 112, 2) + ID * 4 * 43.9825174058 / 112, 0.5) - 134.8694 / 112) / (2 * 43.9825174058 / 112));
	dist = int(1.1 + (pow(pow(134.8694 / 112, 2) + ID * 4 * 43.9825174058 / 112, 0.5) - 134.8694 / 112) / (2 * 43.9825174058 / 112));
	if (EigththDiskPixels(dist - 1) > ID) {
		dist -= 1;
	}
	return dist;
}

int circlePixels(int range) {
	return diskPixels(range) - diskPixels(range - 1);
}
