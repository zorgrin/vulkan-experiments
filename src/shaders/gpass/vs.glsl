#version 450 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;

layout (std140, set = 1, binding = 0) uniform Camera
{
    vec3 eye;
    vec3 target;
    mat4 projection;
    mat4 transform;
} gCamera;

layout (std430, set = 2, binding = 0) buffer InstanceData
{
    mat4 transform[];
} gInstanceData;


layout (location = 0) out vec2 fsTexCoord;
layout (location = 1) out vec4 fsPosition;
layout (location = 2) out flat int fsMeshID;
layout (location = 3) out flat int fsInstanceIndex;
layout (location = 4) out flat int fsPrimitiveID;

void main()
{
    fsTexCoord = inTexCoord;
    fsInstanceIndex = gl_InstanceIndex;
//    fsMeshID = 0;
    
    gl_Position = gCamera.projection * gCamera.transform * gInstanceData.transform[gl_InstanceIndex] * vec4(inPosition, 1.0);
    fsPosition = gInstanceData.transform[gl_InstanceIndex] * vec4(inPosition, 1.0);
}
