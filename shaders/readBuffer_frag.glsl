#version 450
layout(push_constant) uniform PushConstant {
    uint outputWidth;
    uint outputHeight;
} pushConstant;

layout(binding = 1) buffer OutputData {
    uint[2] data[];
} outputArray;

layout(location = 0) out vec4 outColor;

void main() {
    uint x = uint(gl_FragCoord.x * float(pushConstant.outputWidth)/(720));
    uint y = uint(gl_FragCoord.y * float(pushConstant.outputHeight)/(720));
    uint index = x + y*pushConstant.outputHeight;
    outColor = outputArray.data[index][0]%100 >= 10 ? vec4(1.0) : vec4(0.0);
	//outColor = outputArray.data[0][0] > 0 ? vec4(1.0) : vec4(0.0);
	
	uint count = outputArray.data[index][0]%10;

	if(count == 6){
		outColor = vec4(0.5f, 0.2f, 1.0f, 1.0f);
	}
	if(count == 5){
		outColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
	}
	if(count == 4){
		outColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
	}
	if(count == 3){
		outColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	}
	if(count == 2){
		outColor = vec4(1.0f, 0.0f, 1.0f, 1.0f);
	}
	if(count == 1){
		outColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
	}
	if(count == 0){
		outColor = vec4(0.15f, 0.15f, 0.15f, 1.0f);
	}

	if (outputArray.data[index][0]/100 == 1){
		float r = outColor.r*0.25f;
		float g = outColor.g*0.25f;
		float b = outColor.b*0.25f;
		float a = outColor.a*0.25f;
		outColor = vec4(1.0f);
	}else{
		//outColor = vec4(0.0f);
	}


	if((index % pushConstant.outputWidth) == 0){
		if (outputArray.data[index][0] == 0){
			outColor = vec4(0.5f);
		}
	}
	if(index <= pushConstant.outputHeight){
		if (outputArray.data[index][0] == 0){
			outColor = vec4(0.5f);
		}
	}
	
	float factor = 0.6f + pow(outputArray.data[index][1]/100000,2)*0.4f;

	outColor = outColor*factor;

	outColor.r = outColor.r*factor;
}
