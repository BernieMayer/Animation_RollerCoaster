#version 430 core

uniform mat4 modelviewMatrix = mat4(1.f);
uniform mat4 perspectiveMatrix;

uniform vec3 beadPosition;

void main(void)
{
    gl_Position = perspectiveMatrix * modelviewMatrix * vec4(beadPosition, 1.0);
}
