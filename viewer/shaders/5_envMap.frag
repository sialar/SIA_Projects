#version 410
#define M_PI 3.14159265358979323846

uniform sampler2D envMap;
uniform mat4 lightMatrix;
uniform bool transparent;
uniform float eta;

in vec4 vertColor;
in vec3 vertNormal;

out vec4 fragColor;


void main( void )
{
    // Here begins the real work.
    fragColor = vertColor;
}
