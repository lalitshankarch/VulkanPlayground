#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 0) out vec3 outColor;

void main() {
    gl_Position = vec4(inPosition.x, -1.0 * inPosition.y, inPosition.z, 1.0);
    outColor = inNormal;
}