#version 130

uniform mat4 matrix;
uniform mat4 perspective;

in vec3 vertex;
in vec3 color;
in vec3 normal;
out vec3 vertColor;
out vec3 vertNormal;

void main( void )
{
    vertColor = color;
    vertNormal = normal;
    gl_Position = perspective * matrix * vec4(vertex, 1.0);
}
