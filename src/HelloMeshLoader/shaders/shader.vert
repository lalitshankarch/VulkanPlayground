#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 0) out vec2 outPos;

void main() {
    outPos = inPosition.xy;
    gl_Position = vec4(inPosition.x, -1.0 * inPosition.y, inPosition.z, 1.0);
}