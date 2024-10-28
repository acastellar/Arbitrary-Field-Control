#version 450

layout(binding = 0) uniform PerspectiveUniformBufferObject {
    mat4 view;
    mat4 proj;
} perspectiveUBO;

//layout(binding = 1) uniform ModelUniformBufferObject {
//    mat4 model;
//} modelUBO;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

const float particleSize = 4.0f;
void main() {

    gl_Position = perspectiveUBO.proj * perspectiveUBO.view * inPosition;
    gl_PointSize = particleSize / gl_Position.w;
    fragColor = inColor;

}