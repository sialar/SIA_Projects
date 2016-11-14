#version 410

uniform float lightIntensity;
uniform bool blinnPhong;
uniform float shininess;
uniform float eta;

in vec4 eyeVector;
in vec4 lightVector;
in vec3 vertNormal;
in vec4 vertColor;

out vec4 fragColor;

void main( void )
{
    // Here begins the real work.
    fragColor = vertColor;
}
