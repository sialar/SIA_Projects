#version 130

uniform mat4 matrix;
uniform mat4 perspective;
uniform mat3 normalMatrix;
uniform vec3 lightPosition;
uniform bool noColor;

in vec3 vertex;
in vec3 normal;
in vec3 color;

out vec3 eyeVector;
out vec3 lightVector;
out vec3 vertNormal;
out vec4 vertColor;

void main( void )
{
    if (noColor) vertColor = vec4(1.0, 1.0, 1.0, 1.0 );
    else vertColor = vec4(color, 1.0);
    vec4 vertPosition = matrix * vec4(vertex, 1.0);
    vec4 eyePosition = vec4(0.0, 0.0, 0.0, 1.0);

    lightVector = normalize(lightPosition.xyz - vertPosition.xyz);
    eyeVector = normalize(eyePosition.xyz - vertPosition.xyz);
    vertNormal = normalize(normalMatrix * normal);

    gl_Position = perspective * matrix * vec4(vertex,1);
}
