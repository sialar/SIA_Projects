#version 410

uniform mat4 matrix;
uniform mat4 perspective;
uniform mat3 normalMatrix;
uniform mat4 worldToLightSpace;
uniform mat4 lightPerspective;
uniform bool noColor;
uniform vec3 lightPosition;

in vec3 vertex;
in vec3 normal;
in vec3 color;

out vec3 eyeVector;
out vec3 lightVector;
out vec3 vertNormal;
out vec4 vertColor;
out vec4 lightSpace;

void main( void )
{
    if (noColor) vertColor = vec4(1.0, 1.0, 1.0, 1.0 );
    else vertColor = vec4(color, 1.0);
    vec4 vertPosition = matrix * vec4(vertex, 1.0);
    vec4 eyePosition = vec4(0.0, 0.0, 0.0, 1.0);

    lightVector = normalize(lightPosition.xyz - vertPosition.xyz);
    eyeVector = normalize(eyePosition.xyz - vertPosition.xyz);
    vertNormal = normalize(normalMatrix * normal);

    mat4 biasMatrix = mat4(
    0.5, 0.0, 0.0, 0.0,
    0.0, 0.5, 0.0, 0.0,
    0.0, 0.0, 0.5, 0.0,
    0.5, 0.5, 0.5, 1.0
    );

    vec4 worldCoords = matrix * vec4(vertex, 1.0);

    lightSpace = lightPerspective * worldToLightSpace * worldCoords;
    gl_Position = perspective * worldCoords;
}
