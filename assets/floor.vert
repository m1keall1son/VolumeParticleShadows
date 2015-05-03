#version 330

in vec4 ciPosition;
in vec2 ciTexCoord0;
uniform mat4 ciModelViewProjection;

out vec2 vTexCoord0;
out vec4 vPosition;

void main () {
    
    vTexCoord0 = ciTexCoord0;
    vPosition = ciPosition;
    
    gl_Position = ciModelViewProjection * ciPosition;
}