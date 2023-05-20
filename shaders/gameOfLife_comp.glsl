#version 450

struct cellData {
	uint command;
	uint species;
	uint state;
	uint atractivity;
	uint delay;
};

struct calcData {
	uint circleSolutionValue;
	uint eigthDiskSolutionValue;
};



struct InputData {
	int Xres;
	int Yres;
	int RulesDNA[3];
	int BodyDNA[96];
	int sensorGruppenAnzahl[6];
	int Wish;
	int Stay;
	int maxCombiSum;
	int SensorGruppenIDs[96];
	int anzahlCluster;
	int Wertigkeiten[6];
	int thisSpecies;
	bool square;
	int stateGroups;
	bool sumTest;
	int testint;
	int speciesRetardValues[18];
	int speciesThresh[18];
	int speciesAcc[18];
	float factorPart;
};

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(std430, binding = 0) buffer InputCellLiving {
    uint inputLiving[];
};

layout(std430, binding = 1) buffer OutputCellLiving {
    uint outputLiving[];
};

layout(std430, binding = 2) buffer InputCellState {
    cellData inputState[];
};

layout(std430, binding = 3) buffer OutputCellState {
    cellData outputState[];
};

layout(std430, binding = 4) buffer preCalcData {
    calcData calcDataRanges[];
};

layout(std430, binding = 5) buffer InputGlobal {
    InputData globalData;
};

layout(push_constant) uniform PushConstant {
    uint width;
    uint height;
} pushConstant;

void main() {
    uint index = gl_GlobalInvocationID.x + globalData.Xres * gl_GlobalInvocationID.y;
    uint countAlive = 0;
    for(int i = -1; i <= 1; i++) {
        for(int j = -1; j <= 1; j++) {
            if(i == 0 && j == 0) continue;
            uint y = (gl_GlobalInvocationID.y + j) % globalData.Yres;
            uint x = (gl_GlobalInvocationID.x + i) % globalData.Xres;
            uint neighbourIndex = x + globalData.Xres * y;
            countAlive += inputLiving[neighbourIndex]%100 >= 10 ? 1 : 0;
        }
    }
    uint newState = 0;
    if (inputLiving[index]%100 >= 10){
        newState = 100;
        if (countAlive == 2 || countAlive == 3)
        {
            newState += 10;
        }
    }else{
        if (countAlive == 3)
        {
            newState += 10;
        }
    }
    newState += countAlive;

    /*if(inputLiving[index] > 0) {
        if(countAlive < 2 || countAlive > 3) {
            outputLiving[index] = 0;
        } else {
            outputLiving[index] = 1;
        }
    } else {
        if(countAlive == 3) {
            outputLiving[index] = 1;
        } else {
            outputLiving[index] = 0;
        }
    }*/

    outputLiving[index] = newState;
}