#version 450 core

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput gDiffuseIn;

layout (location = 0) out vec4 gDiffuseOut;

void main()
{
//    gDiffuseOut = vec4(1.0, 0.0, 0.0, 1.0);
//    vec4 test = subpassLoad(gDiffuseIn);
//    gDiffuseOut = vec4(1.0 - subpassLoad(gDiffuseIn).xyz, 1.0);
    gDiffuseOut = vec4(subpassLoad(gDiffuseIn).xyz, 1.0);
}
