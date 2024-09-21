#version 450

layout (location = 0) in vec2 inPosition;
layout (location = 0) out vec4 outColor;

float invSdCircle( vec2 p, float r )
{
    return r - length(p);
}

void main() {
    float color = 0.8;
    outColor = vec4(color, color, color, 1.0);
}