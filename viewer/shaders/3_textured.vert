#version 410

uniform mat4 matrix;
uniform mat4 perspective;
uniform mat3 normalMatrix;
uniform bool noColor;
uniform vec3 lightPosition;

in vec3 vertex;
in vec3 normal;
in vec3 color;
in vec2 texcoords;

out vec3 eyeVector;
out vec3 lightVector;
out vec3 vertNormal;
out vec2 textCoords;

void main( void )
{

    vec4 vertPosition = matrix * vec4(vertex, 1.0);
    vec4 eyePosition = vec4(0.0, 0.0, 0.0, 1.0);
    
    // For illumination:
    lightVector = normalize(vertPosition.xyz - lightPosition);
    eyeVector = normalize(eyePosition.xyz - vertPosition.xyz);

    // For texture mapping:
    textCoords = texcoords;
    
    vertNormal = normalize(normalMatrix * normal);
    gl_Position = perspective * matrix * vec4(vertex, 1.0);
}
