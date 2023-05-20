#version 330 core

layout(location = 0) in vec3 a_position; 
layout(location = 1) in vec2 a_texCoord; 
layout(location = 2) in uvec4 a_color;

out vec4 v_color;
out vec2 v_texCoord;

//uniform mat4 u_modelViewProj;

void main()
{
	gl_Position = /*u_modelViewProj*/vec4(a_position, 1.0f);
	v_color = a_color;//vec4(a_position[0]+a_color[0], 0.0f+a_color[0], 0.0f+a_color[0] , 1.0f);
	v_texCoord = a_texCoord;
}

/*out vec2 pixelpos;

void main(){

    pixelpos = vec2(gl_Position[0], gl_Position[1]);
}*/