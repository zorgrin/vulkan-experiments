#version 450 core

layout (set = 2, binding = 1) uniform sampler2D gDiffuse;

layout (location = 0) out vec4 gDiffuseOut;
layout (location = 1) out vec4 gNormalOut;
layout (location = 2) out vec4 gFragCoordOut;
layout (location = 3) out ivec4 gMeshInstancePrimitiveOut;

layout (location = 0) in vec2 fsTexCoord;
layout (location = 1) in vec4 fsPosition;
layout (location = 2) in flat int fsMeshID;
layout (location = 3) in flat int fsInstanceIndex;
layout (location = 4) in flat int fsPrimitiveID;

void main()
{
//    vec3 palette[4];
//    palette[0] = vec3(0.0, 1.0, 0.0);
//    palette[1] = vec3(0.7, 0.3, 1.0);
//    palette[2] = vec3(1.0, 0.6, 0.2);
//    palette[3] = vec3(0.0, 1.0, 1.0);
//
//    float factor = 0.7;

//    gDiffuseOut = vec4((factor * texture(gDiffuse, fsTexCoord).xyz) + (1.0 - factor) * palette[fsInstanceIndex - 4 * (fsInstanceIndex / 4)], 1.0);
    gDiffuseOut = vec4(texture(gDiffuse, fsTexCoord).xyz, 1.0);

    gNormalOut = vec4(0); //TODO dummy
    gMeshInstancePrimitiveOut = ivec4(fsMeshID, fsInstanceIndex, fsPrimitiveID, 1);
    gFragCoordOut = fsPosition;
}
