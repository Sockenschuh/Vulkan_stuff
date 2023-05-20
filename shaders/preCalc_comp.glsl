#version 450 core

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

struct calcData {
    uint circleSolutionValue;
    uint eigthDiskSolutionValue;
};


layout (std430, binding = 0) buffer preCalcData
{
    calcData pre_calc[];
};

int cloitreSum(int n) {
	int Sum = 0;
		for (int i = 0; i < 1 + int(pow(n,0.5f)); i++)
		{
			Sum += int(pow(n - pow(i, 2), 0.5f));
		}			
		return 1 + 4 * Sum;
}

int diskPixels(int n) {
	int result;
	result = cloitreSum(int(pow(n, 2)) + n);
	return result;
}

int EigththDiskPixels(int n, int diskPixels_n){
	return int(round(((11.948f * n)/14.0f) + (diskPixels_n/8.0f)));
}

void main(){

    uint index = gl_GlobalInvocationID.x;    

    uint fullCircle = pre_calc[index].circleSolutionValue;
    uint achtelDisk = pre_calc[index].eigthDiskSolutionValue;

	int range = int(gl_GlobalInvocationID.x);
	int fullouterdisk = diskPixels(range);
	int fullinnerdisk = diskPixels(range-1);
	fullCircle = uint(fullouterdisk-(fullinnerdisk));
	achtelDisk = uint(EigththDiskPixels(range, fullouterdisk));

    pre_calc[index].circleSolutionValue = fullCircle;
    pre_calc[index].eigthDiskSolutionValue= achtelDisk;
}
