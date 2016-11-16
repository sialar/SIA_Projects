#version 130

in vec3 vertNormal;
in vec3 vertColor;

out vec4 fragColor;

void main( void )
{
    fragColor = vec4(vertColor, 1.0 );
}
