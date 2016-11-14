#version 410

uniform mat4 matrix;
uniform mat4 perspective;
uniform mat3 normalMatrix;
uniform bool noColor;

in vec3 vertex;
in vec3 normal;
in vec3 color;

out vec3 vertColor;
out vec3 vertNormal;

void main( void )
{
    if (noColor) vertColor = vec3(0.4, 0.2, 0.6);
    else vertColor = color;
    vertNormal = normalize(normalMatrix * normal);
    gl_Position = perspective * matrix * vec4(vertex, 1.0);
}
