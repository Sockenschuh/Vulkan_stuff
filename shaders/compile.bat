glslc.exe -fshader-stage=vert triangle_vert.glsl -o triangle_vert.spv
glslc.exe -fshader-stage=frag triangle_frag.glsl -o triangle_frag.spv
glslc.exe -fshader-stage=vert color_vert.glsl -o color_vert.spv
glslc.exe -fshader-stage=frag color_frag.glsl -o color_frag.spv

glslc.exe -fshader-stage=comp gameOfLife_comp.glsl -o gameOfLife_comp.spv

glslc.exe -fshader-stage=vert readBuffer_vert.glsl -o readBuffer_vert.spv
glslc.exe -fshader-stage=frag readBuffer_frag.glsl -o readBuffer_frag.spv
