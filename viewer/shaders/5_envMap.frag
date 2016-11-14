#version 410
#define M_PI 3.14159265358979323846

uniform sampler2D envMap;
uniform mat4 lightMatrix;
uniform bool transparent;
uniform float eta;

in vec4 vertColor;
in vec3 vertNormal;
in vec3 eyeVector;

out vec4 fragColor;


void main( void )
{
    
    
    // Here begins the real work.
    vec3 N = normalize(vertNormal);
    vec3 V = normalize(eyeVector);
    
    vec4 R1 = vec4(2 * dot(N, V) * N - V, 1.f);
    
    R1 = lightMatrix * R1;
    
    vec3 R13 = normalize(R1.xyz);
    
     //calcul des coordonnees de texture
    float phi1 = atan(R13.y, R13.x);
    float theta1 = acos(R13.z);
    
    vec2 textCoords1 = vec2((phi1 + M_PI)/(2 * M_PI), theta1/M_PI);
    
    vec4 R2 = vec4(refract(V, N, eta), 1.f);
    
    R2 = lightMatrix * R2;
    
    vec3 R23 = normalize(R2.xyz);
    
    float phi2 = atan(R23.y, R23.x);
    float theta2 = acos(R23.z);
    
    vec2 textCoords2 = vec2((phi2 + M_PI)/(2 * M_PI), theta2/M_PI);
    
    // Fresnel Coeff
    float F0 = pow(1 - eta, 2)/pow(1 + eta, 2);
    float F = F0 + (1 - F0)*(1 - dot(N, V));
    
    //Color
    fragColor = F * texture(envMap, textCoords1) + (1 - F) * texture(envMap, textCoords2);
}
