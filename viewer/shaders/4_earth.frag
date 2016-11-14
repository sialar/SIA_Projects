#version 410
#define M_PI 3.14159265358979323846

uniform float lightIntensity;
uniform sampler2D earthDay;
uniform sampler2D earthNormals;
uniform mat3x3 normalMatrix;
uniform bool blinnPhong;
uniform float shininess;
uniform float eta;

in vec3 eyeVector;
in vec3 lightVector;
in vec3 Vert;
in vec3 vertNormal;

out vec4 fragColor;

void main( void )
{
    //calcul des coordonnees de texture
    vec3 vertex = normalize(Vert);
    
    float phi = atan(vertex.y, vertex.x);
    float theta = acos(vertex.z);
    
    vec2 textCoords = vec2((phi+ M_PI)/(2 * M_PI), theta/M_PI);
    
    // Here begins the real work.
    vec3 N = normalize(vertNormal);
    vec3 L = normalize(lightVector);
    vec3 V = normalize(eyeVector);
    
    //Normal
    vec3 t = vec3(-sin(phi), cos(phi), 0.f);
    vec3 b = - cross(N, t);
    mat3x3 tbn = mat3x3(t, b, N);
    
    N = normalMatrix * tbn * (2.f * texture(earthNormals, textCoords) - vec4(1.f)).xyz;
    
    //Color
    
    vec4 vertColor = texture(earthDay, textCoords);
    // Ambient
    vec4 fragColorA = 0.5f * lightIntensity * vertColor;
    // Diffus
    vec4 fragColorD = max(dot(N, L), 0.f) * lightIntensity * vertColor;
    // Speculaire
    vec3 R = normalize(2.f * dot(N, L) * N - L);
    vec4 fragColorS = pow(max(dot(R, V), 0.f), shininess) * lightIntensity * vertColor;
    

    fragColor = fragColorA + fragColorD + fragColorS;
    
    
    
}
