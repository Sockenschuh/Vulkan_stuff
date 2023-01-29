/*#version 450

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (std430, binding = 0) buffer InputData {
    uint inputArray[];
};

layout (std430, binding = 1) buffer OutputData {
    uint outputArray[];
};

layout (push_constant) uniform PushConstant {
    uint width;
    uint height;
} pushConstant;

shared uint sharedArray[18][18];

vec2 swizzle(vec2 inputCoord, vec2 arraySize) {
    vec2 coord = mod(inputCoord, arraySize);
    return coord;
}

void main()
{
    vec2 arraySize = vec2(pushConstant.width, pushConstant.height);
    vec2 inputCoord = vec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
    vec2 coord = swizzle(inputCoord, arraySize);
    //coord.x -= int(coord.y/16);
    uint x = uint(coord.x);
    uint y = uint(coord.y);
    uint index = y * pushConstant.width + x;
    //sharedArray[gl_LocalInvocationID.x][gl_LocalInvocationID.y] = inputArray[index];

    // Load data from input array
    sharedArray[gl_LocalInvocationID.x+1][gl_LocalInvocationID.y+1] = inputArray[index];
    if (gl_LocalInvocationID.y == 0){
         sharedArray[gl_LocalInvocationID.x+1][0] = inputArray[((y-1)%pushConstant.height) * pushConstant.width + x];
    }
    if (gl_LocalInvocationID.y == 15){
         sharedArray[gl_LocalInvocationID.x+1][17] = inputArray[((y+1)%pushConstant.height) * pushConstant.width + x];
    }
    if (gl_LocalInvocationID.x == 0){
         sharedArray[0][gl_LocalInvocationID.y+1] = inputArray[((y)%pushConstant.height) * pushConstant.width + (x-1)%pushConstant.width];
    }
    if (gl_LocalInvocationID.x == 15){
         sharedArray[17][gl_LocalInvocationID.y+1] = inputArray[((y)%pushConstant.height) * pushConstant.width + (x+1)%pushConstant.width];
    }
    if (gl_LocalInvocationID.x == 1 && gl_LocalInvocationID.y == 1){
        sharedArray[0][0] = inputArray[((y-2)%pushConstant.height) * pushConstant.width + (x-2)%pushConstant.width];
    }
    if (gl_LocalInvocationID.x == 14 && gl_LocalInvocationID.y == 1){
        sharedArray[17][0] = inputArray[((y+2)%pushConstant.height) * pushConstant.width + (x-2)%pushConstant.width];
    }
    if (gl_LocalInvocationID.x == 1 && gl_LocalInvocationID.y == 14){
        sharedArray[0][17] = inputArray[((y-2)%pushConstant.height) * pushConstant.width + (x+2)%pushConstant.width];
    }    
    if (gl_LocalInvocationID.x == 14 && gl_LocalInvocationID.y == 14){
        sharedArray[17][17] = inputArray[((y+2)%pushConstant.height) * pushConstant.width + (x+2)%pushConstant.width];
    }

    memoryBarrierShared();
    barrier();
    // Synchronize threads

    // Calculate the number of alive neighbors
    uint aliveNeighbors = 0;
    for (int i = -1; i <= 1; i++)
    {
    
        for (int j = -1; j <= 1; j++)
        {
            if (i == 0 && j == 0)
            {
                continue;
            }

            // Calculate the neighbor's coordinates
            int xNeighbor = (int(x + 1) + i + int(pushConstant.width)) % int(pushConstant.width);
            int yNeighbor = (int(y + 1) + j + int(pushConstant.height)) % int(pushConstant.height);
            uint indexNeighbor = yNeighbor * pushConstant.width + xNeighbor;

            // Check if the neighbor is alive
            if (sharedArray[xNeighbor][yNeighbor]%100 >= 10)
            {
                aliveNeighbors++;
            }
        }
    }
    // Calculate the new state of the cell
   /* uint newState = 0;
    if (sharedArray[gl_LocalInvocationID.x][gl_LocalInvocationID.y] == 1)
    {
        if (aliveNeighbors == 2 || aliveNeighbors == 3)
        {
            newState = 1;
        }
    }
    else
    {
        if (aliveNeighbors == 3)
        {
            newState = 1;
        }
    }*//*
   uint newState = 0;
    if (sharedArray[gl_LocalInvocationID.x][gl_LocalInvocationID.y]%100 >= 10){
        newState = 100;
        if (aliveNeighbors == 2 || aliveNeighbors == 3)
        {
            newState += 10;
        }
    }else{
        if (aliveNeighbors == 3)
        {
            newState += 10;
        }
    }
    newState += aliveNeighbors;
    	
    newState = sharedArray[gl_LocalInvocationID.x][gl_LocalInvocationID.y+1];
   *//* if (gl_LocalInvocationID.x == 0){
         newState = 13;
    }
    if (gl_LocalInvocationID.y == 0){
         newState = 13;
    }*//*

    // Store the new state in the output array
    outputArray[index] = newState;
}*/


#version 450

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

layout(std430, binding = 0) buffer InputData {
    uint data[];
} inputArray;

layout(std430, binding = 1) buffer OutputData {
    uint data[];
} outputArray;

layout(push_constant) uniform PushConstant {
    uint width;
    uint height;
} pushConstant;

void main() {
    uint index = gl_GlobalInvocationID.x + pushConstant.width * gl_GlobalInvocationID.y;
    uint countAlive = 0;
    for(int i = -1; i <= 1; i++) {
        for(int j = -1; j <= 1; j++) {
            if(i == 0 && j == 0) continue;
            uint y = (gl_GlobalInvocationID.y + j) % pushConstant.height;
            uint x = (gl_GlobalInvocationID.x + i) % pushConstant.width;
            uint neighbourIndex = x + pushConstant.width * y;
            countAlive += inputArray.data[neighbourIndex]%100 >= 10 ? 1 : 0;
        }
    }
    uint newState = 0;
    if (inputArray.data[index]%100 >= 10){
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

    /*if(inputArray.data[index] > 0) {
        if(countAlive < 2 || countAlive > 3) {
            outputArray.data[index] = 0;
        } else {
            outputArray.data[index] = 1;
        }
    } else {
        if(countAlive == 3) {
            outputArray.data[index] = 1;
        } else {
            outputArray.data[index] = 0;
        }
    }*/

    outputArray.data[index] = newState;
}