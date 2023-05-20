#version 450


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


layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(std430, set = 0, binding = 0) buffer preCalcData {
    calcData pre_calc[];    
};

layout(std430, set = 0, binding = 1) buffer InputGlobal {
    InputData global_data;
};

layout(std430, set = 0, binding = 2) buffer outKernel {
    ivec3[8] coords[];
};



/*
layout(std140, set = 1, binding = 0) uniform outKernel {
    ivec3[8] coords[1000];
};*/

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



void main(){
    int out_index = 0;

    for (int interpretationsCluster = 0;interpretationsCluster < global_data.anzahlCluster; interpretationsCluster++){
            for (int sensorgruppe=0; sensorgruppe < global_data.sensorGruppenAnzahl[interpretationsCluster]; sensorgruppe++)
            {	
                for(int i=0; i <global_data.BodyDNA[global_data.SensorGruppenIDs[16*interpretationsCluster+sensorgruppe]*6+4]; i++)
                {		
                    ivec3[8] neighbours;
                    if (global_data.square){
                            neighbours = gtXYofIDsquare(projectID(i,global_data.SensorGruppenIDs[16*interpretationsCluster+sensorgruppe]));		
                        }else{
                            if(false){
                            neighbours = gtXYofIDcircle(projectID(i,global_data.SensorGruppenIDs[16*interpretationsCluster+sensorgruppe]), 0);//int(texture(mainStateMemory, v_texCoord).x));		
                            }else{
                            if(global_data.sumTest){
                            neighbours = pregtXYofIDcircle(projectID(i,global_data.SensorGruppenIDs[16*interpretationsCluster+sensorgruppe]), 0,int(global_data.maxCombiSum/4));		
                            }else{
                            neighbours = pregtXYofIDcircle(projectID(i, global_data.SensorGruppenIDs[16*interpretationsCluster+sensorgruppe]), 0, int(global_data.maxCombiSum/4));		
                            }
                            }
                        }
                    coords[out_index] = neighbours;
                    out_index++;   
                    /*for (int c=0; c<8; c++){
                        if (neighbours[c].z == 0) continue;
                        coordinate neighbour = {neighbours[c].x,neighbors[c].y};
                        coords[index] = neighbour;
                        index++;
                    }*/
                }
            }
    }

    //global_data.kernelSize = index;
}