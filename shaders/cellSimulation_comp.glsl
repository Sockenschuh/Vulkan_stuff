#version 450
#extension GL_EXT_scalar_block_layout : require

struct cellData {
	uint command;
	uint species;
	uint state;
	uint atractivity;
	uint delay;
};

struct fullCellData {
	uint living;
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
	int Xres;          // 0 4 bytes
	int Yres;          // 1 4 bytes
	int Wish;          // 2 4 bytes
	int Stay;          // 3 4 bytes
	int maxCombiSum;	// 4 4 bytes
	int anzahlCluster;	// 5 4 bytes
	int thisSpecies;	// 6 4 bytes
	int stateGroups;	// 7 4 bytes
	int testint;		// 8 4 bytes	
	float factorPart;	// 9 4 bytes	
	int pad_0;
	int pad_1;
	int BodyDNA[96];	// 10 4 bytes	
	int RulesDNA[4];   // 11 16 bytes
	int padding_0[2]; // 8 bytes padding
	int sensorGruppenAnzahl[6]; // 24 bytes
	int SensorGruppenIDs[96]; // 384 bytes
	int Wertigkeiten[6];
	int speciesRetardValues[18];
	int speciesThresh[18];
	int speciesAcc[18];
	bool square;
	bool sumTest;
};



/*
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
*/

layout(push_constant) uniform PushConstant {
    int width;
    int height;
} pushConstant;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(std430, binding = 0) buffer InputCellLiving {
    uint[2] inputLiving[];
};

layout(std430, binding = 1) buffer OutputCellLiving {
    uint[2] outputLiving[];
};

layout(std430, binding = 2) buffer InputCellState {
    cellData inputState[];
};

layout(std430, binding = 3) buffer OutputCellState {
    cellData outputState[];
};

layout(std430, binding = 4) buffer preCalcData {
    calcData pre_calc[];
};

layout(std430, set = 1, binding = 0) uniform kernel {
    ivec3[8] neighbours_coords[10000];
};


layout(std430, set=1, binding = 1) uniform InputGlobal {
    InputData global_data;
};

/*
layout(std430, set=0, binding = 5) buffer InputGlobal {
    InputData global_data;
};
*/


fullCellData previousCellData; // Data from the previous round

fullCellData newCellData; // Data to save for the new round

fullCellData currentCellData; // temporary data to process

int x;
int y;

bool keepSpeciesMemory = false;
bool keepCommandMemory = false;

int projectID(int ID, int Sensorgruppe){	
	if(global_data.square){
	return 
	( 
	(global_data.BodyDNA[Sensorgruppe*6+0]*ID+global_data.BodyDNA[Sensorgruppe*6+2])								// (DNA1*ID + Offset)
	%(global_data.BodyDNA[Sensorgruppe*6+5]-global_data.BodyDNA[Sensorgruppe*6+3])									// %(MaxID-FirstID)
	)+global_data.BodyDNA[Sensorgruppe*6+3];															// + FirstID
	}else{
	return 
	( 
	(global_data.BodyDNA[Sensorgruppe*6+0]*ID+global_data.BodyDNA[Sensorgruppe*6+2])								// (DNA1*ID + Offset)
	%(global_data.BodyDNA[Sensorgruppe*6+5]-global_data.BodyDNA[Sensorgruppe*6+3])									// %(MaxID-FirstID)
	)+global_data.BodyDNA[Sensorgruppe*6+3];															// + FirstID
	}
}

uint projectRule(uint Value, uint maxValue, uint salt){	

	int key = int(global_data.RulesDNA[0]*global_data.RulesDNA[1]-global_data.RulesDNA[2]+int(salt));
	int data = int(Value);

	int R = (data^key) & 0xFFFF;
	int L = (data>>16) ^ (((((R>>5)^(R<<2)) + ((R>>3)^(R<<4))) ^ ((R^0x79b9) + R)) & 0xFFFF);
	key = (key>>3) | (key<<29);
	R ^= ((((L>>5)^(L<<2)) + ((L>>3)^(L<<4))) ^ ((L^0xf372) + (L^key))) & 0xFFFF;
	int res = (((L ^ ((((R>>5)^(R<<2)) + ((R>>3)^(R<<4))) ^ ((R^0x6d2b) + (R^((key>>3)|(key<<29)))))) << 16));
	return uint(res)%(maxValue+uint(1));
}

int back_nat_sum(int n) {
	return int(floor((sqrt(8 * n + 1) - 1) / 2));
}

int nat_sum(int n) {
	return (n * n + n) / 2;
}

int cloitreSum(int n) {
	int Sum = 0;
		for (int i = 0; i < 1 + int(pow(n,0.5f)); i++)
		{
			Sum += int(pow(n - pow(i, 2), 0.5f));
		}			
		return 1 + 4 * Sum;
}

int gaussSum(int n){
	int m = 1;
	int posSum = 0;
	int negSum = 0;
	while (n >= m){
		posSum += n/m;			
		m += 2;
		if(n >= m){
		negSum += n/m;}
	}
	return (posSum-negSum)*4+1;
}


int diskPixels(int n) {
	int result;
	result = cloitreSum(int(pow(n, 2)) + n);
	return result;
}

int EigththDiskPixels(int n, int diskPixels_n){
	return int(round(((11.948f * n)/14.0f) + (diskPixels_n/8.0f)));
}

int precalcEigththDiskPixels(int n){
    return int (pre_calc[n].eigthDiskSolutionValue);
	//return int(texture(eigthDisk, vec2((float(n+0.5f))/200.0f, 0.5f)).x);
}

int aproxDiskRange(int ID) {
	int approxdist = int(1.1+(pow(pow(134.8694 / 112, 2) + ID * 4 * 43.9825174058 / 112, 0.5) - 134.8694 / 112) / (2 * 43.9825174058 / 112));
	//int approxdist = int(1.27323316861 * (pow(1.45007613651 + ID*1.57080419306, 0.5)) - 0.43321601348 );

	return approxdist;

}

int circlePixels(int range) {
	return diskPixels(range) - diskPixels(range - 1);
}



ivec3[8] gtXYofIDcircle(int ID, int turn){
	
	int mutatable = 0;

	int factor = 1;

	int dist;
	int outerDiskPixels;	
	int lesserDiskPixels;	
	int outerDiskPixelsEigth;	
	int lesserDiskPixelsEigth;	
	/*int appr[] = aproxDiskRange(5);
	
	int dist = appr[0];	
	int outerDiskPixels = appr[1];
	int lesserDiskPixels = appr[2];*/
	//int a = Stay;
	//int dist = aproxDiskRange(ID)-0; // wieso -1????? keine ahnung .... sollte nicht richtig sein und verursacht eventuell ungew�nschtes verhalten aber dadurch mehr Pixel abgedeckt ....

	int approxdist = aproxDiskRange(ID);

	int testDisk = diskPixels(approxdist-1);
	int testEigth = EigththDiskPixels(approxdist-1, testDisk);

    if (testEigth > ID){
        approxdist -= 1;
		int smallerDisk = diskPixels(approxdist-1);
		dist = approxdist;
		outerDiskPixels = testDisk;	
		lesserDiskPixels =  smallerDisk;	
		outerDiskPixelsEigth = testEigth;	
		lesserDiskPixelsEigth =  EigththDiskPixels(approxdist-1, smallerDisk);	
		}else{			
			int upperDisk = diskPixels(approxdist);		
			dist = approxdist;
			outerDiskPixels = upperDisk;	
			lesserDiskPixels = testDisk;	
			outerDiskPixelsEigth =  EigththDiskPixels(approxdist, upperDisk);	
			lesserDiskPixelsEigth = testEigth;	
	}

	/*int outerDiskPixels = diskPixels(dist);
	int lesserDiskPixels = diskPixels(dist-1);
	int outerDiskPixelsEigth = EigththDiskPixels(dist, outerDiskPixels);
	int lesserDiskPixelsEigth = EigththDiskPixels(dist-1, lesserDiskPixels);*/

	int position = ID-lesserDiskPixelsEigth;//EigththDiskPixels(dist-1, lesserDiskPixels);
	int radSteps = outerDiskPixels-(lesserDiskPixels-1);
	//radSteps = outerDiskPixelsEigth;
	
	
	float radPosition = float(position)/float(radSteps);
	//radPosition= 1.0f/8.0f;

	if (radPosition == 0 || radPosition == 1.0f/8.0f) factor = 0;
	//factor = int(-1*(abs(roundEven((ID/dist)*2+1)-2)-1));

	ivec3[8] result;

	int sinID = int(round(dist*float(sin((radPosition)*2.0f*3.1415926535897f))));
	int cosID = int(round(dist*float(cos((radPosition)*2.0f*3.1415926535897f))));
	

	result[0] = ivec3(sinID, -cosID, 1);
	result[1] = ivec3(-sinID, cosID, 1);
	result[2] = ivec3(cosID, sinID, 1);
	result[3] = ivec3(-cosID, -sinID, 1);

	result[4] = ivec3(cosID, -sinID, factor);
	result[5] = ivec3(-cosID, sinID, factor);
	result[6] = ivec3(sinID, cosID, factor);
	result[7] = ivec3(-sinID, -cosID, factor);

	return result;
}

ivec3 rotate(ivec3 point, float xTwoPi)
{
	float rX = point.x * cos(xTwoPi) - point.y * sin(xTwoPi);
	float rY = point.x * sin(xTwoPi) + point.y * cos(xTwoPi);

    return ivec3(rX, rY, point.z);
}

ivec3[8] pregtXYofIDcircle(int ID, int turn, int turnevery){
	
	int mutatable = 0;
	//turn = 0;
	int factor = 1;

	int dist;
	int outerDiskPixelsEigth;	
	int lesserDiskPixelsEigth;	
	/*int appr[] = aproxDiskRange(5);
	
	int dist = appr[0];	
	int outerDiskPixels = appr[1];
	int lesserDiskPixels = appr[2];*/
	//int a = Stay;
	//int dist = aproxDiskRange(ID)-0; // wieso -1????? keine ahnung .... sollte nicht richtig sein und verursacht eventuell ungew�nschtes verhalten aber dadurch mehr Pixel abgedeckt ....

	int approxdist = aproxDiskRange(ID);

	int testEigth = precalcEigththDiskPixels(approxdist-1);

    if (testEigth > ID){
        approxdist -= 1;
		dist = approxdist;	
		outerDiskPixelsEigth = testEigth;	
		lesserDiskPixelsEigth =  precalcEigththDiskPixels(approxdist-1);	
		}else{				
			dist = approxdist;	
			outerDiskPixelsEigth =  precalcEigththDiskPixels(approxdist);	
			lesserDiskPixelsEigth = testEigth;	
	}

	/*int outerDiskPixels = diskPixels(dist);
	int lesserDiskPixels = diskPixels(dist-1);
	int outerDiskPixelsEigth = EigththDiskPixels(dist, outerDiskPixels);
	int lesserDiskPixelsEigth = EigththDiskPixels(dist-1, lesserDiskPixels);*/
		

	int position = ID-lesserDiskPixelsEigth;//EigththDiskPixels(dist-1, lesserDiskPixels);
	int radSteps = int(pre_calc[dist].circleSolutionValue);//  int(texture(circle, vec2((float(dist+0.5f))/200.0f, 0.5f)).x);
	//radSteps = outerDiskPixelsEigth;

	float turna = turn/turnevery;
	int turnb = int((float(turn)/turnevery)*radSteps);
	
	float radPosition_1 = float(2.0f*3.14159265358979323846f*float(position+turnb))/float(radSteps);
	float radPosition_2 = float(2.0f*3.14159265358979323846f*float(position-turnb))/float(radSteps);
	//radPosition= 1.0f/8.0f;
	//float turnaround = (2.0f*3.14159265358979323846f*(float(turn)/))/float(maxCombiSum);
	/*
	float a = 0.706f;
	if(dist>=30){
		a = 0.706f+(float(dist)-30.0f)*0.004f;
		a = min(0.95f, a);
	}
	
	
	float x_2 = float(position+turnb)/float(radSteps);
	float x = x_2*4.0f*3.14159265358979323846f;
	radPosition_1 = (x_2-(float(
	float((1.0f - (float(60.0f*a)/120.0f))  )	*(pow( abs(cos(x)), abs(float(a)*float(sin(x))) ))	*float(abs(cos(x)))/float(cos(x)) + //h(x)
	float((0.0f + (float(59.0f*a)/120.0f))  )	*(pow( abs(cos(x)), a) )				*float(abs(cos(x)))/float(cos(x)) -				//g(x)
	float(	float(1.0f)/120.0f  )		*(pow( abs(sin(0.5f*x)), a ) - pow( abs(cos(0.5f*x)) , a) ) -									//j(x)
	cos(x)																																//f(x)
	)/8.0f)*pow(abs(1.0f),0.1f))*2.0f*3.14159265358979323846f;

	x_2 = float(position-turnb)/float(radSteps);
	x = x_2*4.0f*3.14159265358979323846f;
	radPosition_2 = (x_2-(float(
	float((1.0f - (float(60.0f*a)/120.0f))  )	*(pow( abs(cos(x)), abs(float(a)*float(sin(x))) ))	*float(abs(cos(x)))/float(cos(x)) + //h(x)
	float((0.0f + (float(59.0f*a)/120.0f))  )	*(pow( abs(cos(x)), a) )				*float(abs(cos(x)))/float(cos(x)) -				//g(x)
	float(	float(1.0f)/120.0f  )		*(pow( abs(sin(0.5f*x)), a ) - pow( abs(cos(0.5f*x)) , a) ) -									//j(x)
	cos(x)																																//f(x)
	)/8.0f)*pow(abs(1.0f),0.1f))*2.0f*3.14159265358979323846f;

	*/

	
	if (radPosition_1 == 0 || radPosition_1 == 1/8) factor = 0;
	//factor = int(-1*(abs(roundEven((ID/dist)*2+1)-2)-1));



	ivec3[8] result;
	/*
	float PlusRad = (2.0f*3.14159265358979323846f*float(position+turn))/float(radSteps);
	float MinusRad = (2.0f*3.14159265358979323846f*float(position-turn))/float(radSteps);
	*/
	int sinID_1 = int(round(float(dist)*float(sin((radPosition_1)))));
	int cosID_1 = int(round(float(dist)*float(cos((radPosition_1)))));
	int sinID_2 = int(round(float(dist)*float(sin((radPosition_2)))));
	int cosID_2 = int(round(float(dist)*float(cos((radPosition_2)))));	
	/*int plusSinID = int(round(float(dist)*float(sin((PlusRad)))));
	int plusCosID = int(round(float(dist)*float(cos((PlusRad)))));	
	int minusSinID = -int(round(float(dist)*float(sin((MinusRad)))));
	int minusCosID = -int(round(float(dist)*float(sin((MinusRad)))));*/
	
	/*result[0] = ivec3(sinID, -cosID, 1);
	result[1] = ivec3(-sinID, cosID, 1);
	result[2] = ivec3(cosID, sinID, 1);
	result[3] = ivec3(-cosID, -sinID, 1);

	result[4] = ivec3(cosID, -sinID, factor);
	result[5] = ivec3(-cosID, sinID, factor);
	result[6] = ivec3(sinID, cosID, factor);
	result[7] = ivec3(-sinID, -cosID, factor);*/
	
	result[0] = ivec3(sinID_1, -cosID_1, 1);
	result[1] = ivec3(-sinID_1, cosID_1, 1);
	result[2] = ivec3(cosID_1, sinID_1, 1);
	result[3] = ivec3(-cosID_1, -sinID_1, 1);

	result[4] = ivec3(cosID_2, -sinID_2, factor);
	result[5] = ivec3(-cosID_2, sinID_2, factor);
	result[6] = ivec3(sinID_2, cosID_2, factor);
	result[7] = ivec3(-sinID_2, -cosID_2, factor);
	/*
	result[0] = rotate(ivec3(sinID, -cosID, 1), turnaround);
	result[1] =  rotate(ivec3(-sinID, cosID, 1), turnaround);
	result[2] =  rotate(ivec3(cosID, sinID, 1), turnaround);
	result[3] =  rotate(ivec3(-cosID, -sinID, 1), turnaround);

	result[4] =  rotate(ivec3(cosID, -sinID, factor), turnaround);
	result[5] =  rotate(ivec3(-cosID, sinID, factor), turnaround);
	result[6] =  rotate(ivec3(sinID, cosID, factor), turnaround);
	result[7] =  rotate(ivec3(-sinID, -cosID, factor), turnaround);
	*/
	/*
	result[0] = ivec3(plusSinID, minusCosID, 1);
	result[1] = ivec3(minusSinID, plusCosID, 1);
	result[2] = ivec3(plusCosID, plusSinID, 1);
	result[3] = ivec3(minusCosID, minusSinID, 1);

	result[4] = ivec3(plusCosID, minusSinID, factor);
	result[5] = ivec3(minusCosID, plusSinID, factor);
	result[6] = ivec3(plusSinID, plusCosID, factor);
	result[7] = ivec3(minusSinID, minusCosID, factor);
	*/
	/*
	
	float radPos1 =  radPosition + 0.0f*2.0f*3.14159265358979323846f;
	float radPos2 =  radPosition + (1.0f/8.0f)*2.0f*3.14159265358979323846f;
	float radPos3 =  radPosition + (2.0f/8.0f)*2.0f*3.14159265358979323846f;
	float radPos4 =  radPosition + (3.0f/8.0f)*2.0f*3.14159265358979323846f;
	float radPos5 =  radPosition + (4.0f/8.0f)*2.0f*3.14159265358979323846f;
	float radPos6 =  radPosition + (5.0f/8.0f)*2.0f*3.14159265358979323846f;
	float radPos7 =  radPosition + (6.0f/8.0f)*2.0f*3.14159265358979323846f;
	float radPos8 =  radPosition + (7.0f/8.0f)*2.0f*3.14159265358979323846f;

	result[0] = ivec3(int(round(float(dist)*float(sin((radPos1))))), int(round(float(dist)*float(cos((radPos1))))), 1);
	result[1] = ivec3(int(round(float(dist)*float(sin((radPos2))))), int(round(float(dist)*float(cos((radPos2))))), 1);
	result[2] = ivec3(int(round(float(dist)*float(sin((radPos3))))), int(round(float(dist)*float(cos((radPos3))))), 1);
	result[3] = ivec3(int(round(float(dist)*float(sin((radPos4))))), int(round(float(dist)*float(cos((radPos4))))), 1);
	result[4] = ivec3(int(round(float(dist)*float(sin((radPos5))))), int(round(float(dist)*float(cos((radPos5))))), 1);
	result[5] = ivec3(int(round(float(dist)*float(sin((radPos6))))), int(round(float(dist)*float(cos((radPos6))))), 1);
	result[6] = ivec3(int(round(float(dist)*float(sin((radPos7))))), int(round(float(dist)*float(cos((radPos7))))), 1);
	result[7] = ivec3(int(round(float(dist)*float(sin((radPos8))))), int(round(float(dist)*float(cos((radPos8))))), 1);
	*/
	return result;
}


ivec3[8] gtXYofIDsquare(int ID){
	
	int factor = 1;
	int dist = back_nat_sum(ID + 1);
	int position = ID - nat_sum(dist) + 1;	
	if (position == 0 || position == dist) factor = 0;
	//factor = int(-1*(abs(roundEven((ID/dist)*2+1)-2)-1));

	ivec3[8] result;

	result[0] = ivec3(-dist+position,-dist, 1);
	result[1] = ivec3(+dist-position,+dist, 1);
	result[2] = ivec3(+dist,-dist+position, 1);
	result[3] = ivec3(-dist,+dist-position, 1);

	result[4] = ivec3(-dist+position,+dist, factor);
	result[5] = ivec3(+dist-position,-dist, factor);
	result[6] = ivec3(-dist,-dist+position, factor);
	result[7] = ivec3(+dist,+dist-position, factor);

	return result;
}


vec2 swizzle(vec2 inputCoord, vec2 arraySize) {
    vec2 coord = mod(inputCoord, arraySize);
    return coord;
}




void auraToCommand(){
	ivec2 Res = ivec2(global_data.Xres, global_data.Yres);	

	uint previousLiving = previousCellData.living;	
	uint previousState = previousCellData.state;
	int previousCommand = int(previousCellData.command)-1;
	int currentCommand = previousCommand;    
	int precommand = currentCommand;
	int finalcommand = 0;//command;

	

	uint summe = uint(0);
	uint lokaleSumme = uint(0);
	uint lokaleGroesse = uint(0);
	uint lokaleInterpretationsSumme = uint(0);
	uint lokaleInterpretationsGroesse = uint(0);
	uint Rule = uint(0);

	int localCommand = 0;
	int localInterpratationsCommand = 0;
	int index_coords = 0;

	for (int interpretationsCluster = 0;interpretationsCluster < global_data.anzahlCluster; interpretationsCluster++){
		lokaleInterpretationsSumme = (0);
		lokaleInterpretationsGroesse = (0);
		for (int sensorgruppe=0; sensorgruppe < global_data.sensorGruppenAnzahl[interpretationsCluster]; sensorgruppe++)
		{	
			lokaleSumme = (0);
			lokaleGroesse = (0);
			for(int i=0; i <global_data.BodyDNA[global_data.SensorGruppenIDs[16*interpretationsCluster+sensorgruppe]*6+4]; i++)
			{		
				ivec3[8] neighbours = neighbours_coords[index_coords];
				index_coords++;
				/*if (global_data.square){
						neighbours = gtXYofIDsquare(projectID(i,global_data.SensorGruppenIDs[16*interpretationsCluster+sensorgruppe]));		
					}else{
						if(false){
						neighbours = gtXYofIDcircle(projectID(i,global_data.SensorGruppenIDs[16*interpretationsCluster+sensorgruppe]), 0);//int(texture(mainStateMemory, v_texCoord).x));		
						}else{
						if(global_data.sumTest){
						neighbours = pregtXYofIDcircle(projectID(i,global_data.SensorGruppenIDs[16*interpretationsCluster+sensorgruppe]), int(previousState),int(global_data.maxCombiSum/4));		
						}else{
						neighbours = pregtXYofIDcircle(projectID(i, global_data.SensorGruppenIDs[16*interpretationsCluster+sensorgruppe]), 0, int(global_data.maxCombiSum/4));		
						}
						}
					}*/
				for (int j=0; j<8;j++){
					
					lokaleGroesse += (1);
					lokaleInterpretationsGroesse += (1);
                    int neighbourIndex = ((y+neighbours[j].y)%pushConstant.height)*pushConstant.width + (x+neighbours[j].x)%pushConstant.width;
                    uint lastLivingHere = inputLiving[neighbourIndex][0];
					if (lastLivingHere * (neighbours[j].z) >= (1)){
						summe += (global_data.Wertigkeiten[interpretationsCluster]);
						lokaleSumme += (1);					
						lokaleInterpretationsSumme += (1);
					}
				}				
			}
			
			//Rule = (projectRule(lokaleSumme, maxCombiSum));//\*(((maxCombiSum+1)/(lokaleGroesse+1)));///int(pow((sensorgruppe+1),2));
			
				uint salt = previousState/(global_data.maxCombiSum/global_data.stateGroups);
				Rule = (projectRule((lokaleSumme), (global_data.maxCombiSum), salt));
				currentCommand = 1;
				if (Rule >= (global_data.Wish)) currentCommand = 0;
				if (Rule >= (global_data.Wish+global_data.Stay)) currentCommand = -1;

				currentCommand = min(abs(int(lokaleSumme)), 1)*currentCommand;	
				finalcommand = max(-1, min(1, finalcommand+currentCommand));				
			
		}
			localCommand = finalcommand;
			uint salt = previousState/(global_data.maxCombiSum/global_data.stateGroups);
			Rule = projectRule((lokaleInterpretationsSumme), (global_data.maxCombiSum), salt);
			//Rule = (projectRule(lokaleInterpretationsSumme, maxCombiSum));
			currentCommand = 1;
			if (Rule >= (global_data.Wish)) currentCommand = 0;
			if (Rule >= (global_data.Wish+global_data.Stay)) currentCommand = -1;

			currentCommand = min(abs(int(lokaleInterpretationsSumme)), 1)*currentCommand;
			finalcommand = max(-1, min(1, finalcommand+currentCommand));			
			finalcommand = max(-1, min(1, localCommand+finalcommand));

			//finalcommand = int((roundEven((float(finalcommand +command)/2)+1))-1);	
			//finalcommand = int((roundEven((float(finalcommand +localCommand)/2)+1))-1);		
	}
	if(true){
	localInterpratationsCommand = finalcommand;
	//finalcommand = 0;
	uint salt = previousState/uint(global_data.maxCombiSum/global_data.stateGroups);
	salt=uint(0);
	Rule = projectRule(uint(summe), uint(global_data.maxCombiSum), salt);
	currentCommand = 1;
	if (Rule >= uint(global_data.Wish)) currentCommand = 0;
	if (Rule >= uint(global_data.Wish+global_data.Stay)) currentCommand = -1;
	//if (finalcommand == 0){
	//finalcommand = int((roundEven((float(finalcommand +command)/2)+1))-1);
	//finalcommand = max(-1, min(finalcommand, 1));
	//finalcommand = int((roundEven((float(finalcommand +localInterpratationsCommand)/2)+1))-1);
	currentCommand = min(abs(int(summe)), 1)*currentCommand;
	
	//finalcommand = int((roundEven((float(finalcommand +command)/2)+1))-1);
	finalcommand = max(-1, min(1, finalcommand+currentCommand));		
	//finalcommand = int((roundEven((float(finalcommand +localInterpratationsCommand)/2)+1))-1);
	finalcommand = max(-1, min(1, localInterpratationsCommand+finalcommand));	
	}
	
	//finalcommand = command;	
	
	
	
	//float result = clamp(f_color.x + u_Rules[int(summe)][1]*1.0f, 0.0f, 1.0f);
	if (precommand == 0 || precommand != finalcommand){
        currentCellData.species = uint(1);//thisSpecies;
		currentCellData.command = uint(finalcommand+1);
		//memstate = 0;//uint(summe);
	}else{ // ToDo: eigentlich hier kopie-Werte der vorher ausgewerten spezies!
		currentCellData.species = uint(1);//texture(PreSpecies, vec2(v_texCoord.x, v_texCoord.y)).x;
		currentCellData.command = uint(precommand+1);
		//memstate = 0;//uint(summe);
	}
    currentCellData.state = uint(summe);//projectRule(uint(summe),maxCombiSum,4444);//%(max(2,maxCombiSum/4));// uint(summe);
	
	//uint result = thisSpecies*uint(ceil((min(uint(1), previousLiving) + finalcommand*1.0f)/2));
}

void postDecisionProcessing(){
    uint memComm = currentCellData.command;
	uint memSpec = currentCellData.species;
	uint currDelay = previousCellData.delay;	
	
	uint candComm = currentCellData.command;
	uint candSpec = currentCellData.species;
	
	/*curComm = candComm;	
	curSpec = candSpec;

	if (currDelay == 2){
		memoryDelay = 1;
	}

	memoryComm = candComm;
	memorySpec = candSpec;

	if (memoryDelay == 0){
		memoryDelay = 50;//speciesRetardValues[0];
	}*/

    
	uint curComm = candComm;	
	uint curSpec = candSpec;
	uint memoryComm = candComm;
	uint memorySpec = candSpec;
    uint memoryDelay = 0;

	if(currDelay > uint(0)){
		//if(candComm==memComm){
			memoryDelay = currDelay-uint(1);//memoryDelay = currDelay;
		//}else{
			curComm = uint(1);	

            keepCommandMemory = true; // but do not execute it
            keepSpeciesMemory = true; // but do not execute it

			memoryComm = memComm;
			memorySpec = memSpec;	
			/*if(currDelay>=2){
				memoryDelay = currDelay-2;}
			else{
				memoryDelay = 0;
			}
			}*/
	}else{
		
		memoryDelay = uint(global_data.speciesRetardValues[(candSpec-uint(1))*uint(3) + (candComm)]);
		
	}

	/*if (currDelay > 10){
		if(memSpec == candSpec && memComm == candComm){
			memoryComm = candComm;
			memorySpec = candSpec;
			memoryDelay = 0;//speciesRetardValues[candSpec*3 + (candComm+1)];
		}
		else{
			memoryComm = candComm;
			memorySpec = candSpec;
			memoryDelay = currDelay-1;
		}
		curComm = 1;
		curSpec = 0;
	}
	else{
		if (candSpec == 0){
			memoryComm = 1;
			memorySpec = 0;
			memoryDelay = 0;
			curComm = 0;
			curSpec = 0;
		}
		else{
			memoryComm = candComm;
			memorySpec = candSpec;
			memoryDelay = 1;//speciesRetardValues[candSpec*3 + (candComm+1)];
			curComm = candComm;
			curSpec = candSpec;
		}
	}*/
    currentCellData.command = curComm;
	currentCellData.species = curSpec;
    currentCellData.delay = memoryDelay;
}

void evalEfforts(){
    uint candComm =  currentCellData.command;
	uint candSpec =  currentCellData.species;
	uint attr = currentCellData.atractivity;

	uint outCommand = candComm; //OUTPUT
	uint outSpecies = candSpec; //OUTPUT
    uint newAttractivity; //OUTPUT

    float factorPart = global_data.factorPart;
	
	if(candSpec != uint(0)){
		
		uint a = uint(3)*(candSpec-uint(1))+(candComm);
		uint b = uint(global_data.speciesThresh[uint(3)*(candSpec-uint(1))+(candComm)])-uint(attr);
		if(candComm == uint(0)){	
			float factor = (uint(global_data.speciesThresh[uint(3)*(candSpec-uint(1))+(candComm)])-attr)/uint(100000);			
			factor = pow((uint(global_data.speciesThresh[uint(3)*(candSpec-uint(1))+(candComm)])-uint(attr))/uint(100000), 2);
			float x = 2*float(attr)/100000.0f;
			factor = (pow(pow(x,2-2*x),2));
			factor = float(attr)/100000.0f;
			factor = (1.0f-factorPart)+factorPart*(1-pow(factor,2.0f));
			//factor = (1.0f-factorPart);
			int add = int(factor*global_data.speciesAcc[uint(3)*(candSpec-uint(1))+(candComm)]);
			newAttractivity = uint(min(100000,max(int(0),int(attr)+ add)));
				if(newAttractivity < uint(global_data.speciesThresh[uint(3)*(candSpec-uint(1))+(candComm)])){
					outCommand = candComm;
					outSpecies = candSpec;
				}else{
					outCommand = uint(1);
					outSpecies = candSpec;
				}
			}
		}
		if(candComm == uint(1)){
			int some1 = int(attr) + int(global_data.speciesAcc[uint(3)*(candSpec-uint(1))+(candComm)]);
			newAttractivity = uint(min(100000,max(some1,0)));
			
			
			
			/*if (attr>50000){
				int some1 = int(attr) - speciesAcc[3*(candSpec-1)+(candComm)];
				if (some1 < 0){
					newAttractivity = 0;
					outCommand = 0;
					}
					else{
					newAttractivity = some1;
					}
				
				}
				

			else{
				int some1 = int(attr) + speciesAcc[3*(candSpec-1)+(candComm)];
				if (some1 > 100000){
					newAttractivity = 100000;
					outCommand = 2;
					}
					else{
					newAttractivity = some1;
					}				
				
				}
				/*if(newAttractivity < speciesThresh[3*(candSpec-1)+0]){
					outCommand = 0;
					outSpecies = candSpec;}
				if(newAttractivity > speciesThresh[3*(candSpec-1)+2]){
					outCommand = 2;
					outSpecies = candSpec;}*/
		}
		if(candComm == uint(2)){	
			float factor = min(1.0f,1.0f*float(max(1000, int(attr)))/(100000- global_data.speciesThresh[uint(3)*(candSpec-uint(1))+(candComm)]));
			factor = pow((global_data.speciesThresh[uint(3)*(candSpec-uint(1))+(candComm)]-int(attr))/100000, 2);
			float x = 2*float(attr)/100000.0f;
			factor = (pow(pow(x,2-2*x),2));
			factor = float(attr)/100000.0f;
			factor = (1.0f-factorPart)+factorPart*(pow(factor,2.0f));
			//factor = (1.0f-factorPart);
			int add = int(factor*global_data.speciesAcc[3*(int(candSpec)-1)+int(candComm)]);
			newAttractivity = uint(min(100000,max(int(0),int(attr) + add )));
			
			if(newAttractivity > uint(global_data.speciesThresh[3*(int(candSpec)-1)+int(candComm)])){
				outCommand = candComm;
				outSpecies = candSpec;
			}else{
				outCommand = uint(1);
				outSpecies = candSpec;
			}
		}
		



		// uint(min(max(int(0), (int(attr) + (int(candComm)-1)*int(speciesAcc[3*candSpec+(candComm)]))), 100000)); 
	
	
	/*
	if(candSpec != 0){
		if(candComm == 0){
			if (attr < speciesThresh[3*(candSpec-1)+(candComm)]){
				outCommand = 1;
				outSpecies = candSpec;
			}else{
				if(newAttractivity < speciesThresh[3*(candSpec-1)+(candComm)]){
					outCommand = candComm;
					outSpecies = candSpec;
				}else{
					outCommand = 1;
					outSpecies = candSpec;
				}
			}
		}
		if(candComm == 2){
			if (attr > speciesThresh[3*(candSpec-1)+(candComm)]){
				outCommand = 1; 
				outSpecies = candSpec;
			}else{
				if(newAttractivity > speciesThresh[3*(candSpec-1)+(candComm)]){
					outCommand = candComm;
					outSpecies = candSpec;
				}else{
					outCommand = 1;
					outSpecies = candSpec;
				}
			}
		}
	}*/

    currentCellData.command = outCommand; //OUTPUT
	currentCellData.species = outSpecies; //OUTPUT
    currentCellData.atractivity = newAttractivity; //OUTPUT
}

void executeChanges(){
    uint command = currentCellData.command;
	uint species = currentCellData.species;	
	//int d = texture(targetCommand, v_texCoord).z;

	uint memstate = currentCellData.state;
	uint memorymemstate = previousCellData.state;
	
    uint new_memstate = memstate; //OUTPUT
    uint Exec_new_living = uint(0); //OUTPUT

	uint living = currentCellData.living;
	
	//new_living = species*uint(ceil((min(1, living)*1.0f + (command+1)*1.0f)/2));
	
		if (living >= uint(1)){	
			if(command == uint(0)){
				Exec_new_living = uint(0);
				new_memstate = uint(0);//memstate;
				//new_living = 1;			
				//new_living = 1;
			}
			if(command == uint(1)){
				Exec_new_living = living;	
				new_memstate = memorymemstate;
				//new_living = 1;
			}
			if(command == uint(2)){
				Exec_new_living = living;
				new_memstate = memorymemstate;
				//new_living = 90;
			}
		}else{
			//new_living = 20;

			if(command == uint(0)){
				Exec_new_living = uint(0);
				new_memstate = uint(0);//memorymemstate;
				//new_living = 1;
			}
			if(command == uint(1)){
				Exec_new_living = uint(0);
				new_memstate = uint(0);//memstate;
			}
			if(command == uint(2)){
				Exec_new_living = uint(1);//species;	
				new_memstate = memstate;
				//new_living = 1;
			}
		}
		
		/*if (memstate == 2){
			discard;
		}else{
			new_memstate=3;
		}*/
		/*if(memstate == uint(0)){
			new_memstate = memorymemstate+1;
		}else{
			new_memstate=memstate;}*/

            
    currentCellData.state = new_memstate; //OUTPUT
    currentCellData.living = Exec_new_living; //OUTPUT
}

void main(){
    vec2 arraySize = vec2(global_data.Xres, global_data.Yres);
    vec2 inputCoord = vec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
    vec2 coord = swizzle(inputCoord, arraySize);
    x = int(coord.x);
    y = int(coord.y);
    int index = y * global_data.Xres + x;

	/*
struct cellData {
	uint command;
	uint species;
	uint state;
	uint atractivity;
	uint delay;
};

struct fullCellData {
	uint living;
	uint command;
	uint species;
	uint state;
	uint atractivity;
	uint */

	cellData c = inputState[index];

    fullCellData pCellData = {inputLiving[index][0], c.command, c.species, c.state, c.atractivity, c.delay}; // Data from the previous round

	previousCellData = pCellData;

    newCellData = previousCellData; // Data to save for the new round

    currentCellData = previousCellData; // temporary data to process

    auraToCommand(); //sets current species, command and state

    postDecisionProcessing(); //evaluates current command and species. sets current delay. sets keepMemory Bools for timers 

    evalEfforts(); //evaluates tug of war with a change in state or win/lose for a species. can neutralize species/command and sets atractivity(TugOfWar)

    executeChanges(); // finally sets LIVING and can neutralize state

    //after excution to enable the delay timers:
    if(keepCommandMemory == true) currentCellData.command = previousCellData.command;
    if(keepSpeciesMemory == true) currentCellData.species = previousCellData.species;

	fullCellData fc = currentCellData;
	cellData out_c = {fc.command, fc.species, fc.state, fc.atractivity, fc.delay};

    outputState[index] = out_c;
    outputLiving[index][0] = fc.living;
    outputLiving[index][1] = fc.atractivity;
	/*
	for(int i = 0; i <18; i++){
		if (global_data.speciesAcc[i] != 0){
			outputLiving[index][0] = 15;
			outputLiving[index][1] = 100000;
		}
		if (global_data.Yres == 32 * 32*1){
			outputLiving[index][0] = 15;
			outputLiving[index][1] = 100000;
		}
	}
	if(index >= global_data.Yres*global_data.Xres/2){

		for(int i = 0; i < 3; i++){
			if(global_data.BodyDNA[i] != 0){
			outputLiving[index][0] = 10 + global_data.BodyDNA[i];
			outputLiving[index][1] = 100000;
			}
			}
	}
	if(index >= 3*global_data.Yres*global_data.Xres/4){
			if(global_data.Wish == 0){				
			outputLiving[index][0] = 12;
			outputLiving[index][1] = 100000;
			}
	}*/
	
}

