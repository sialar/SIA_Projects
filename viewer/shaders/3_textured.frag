#version 410

uniform float lightIntensity;
uniform sampler2D colorTexture;
uniform bool blinnPhong;
uniform float shininess;
uniform float eta;

in vec3 eyeVector;
in vec3 lightVector;
in vec3 vertNormal;
in vec2 textCoords;

out vec4 fragColor;

void main( void )
{
    // Here begins the real work.
    vec3 N = normalize(vertNormal);
    vec3 L = normalize(lightVector);
    vec3 V = normalize(eyeVector);
    
    vec4 vertColor = texture(colorTexture, textCoords);
    
    // Ambient
    vec4 fragColorA = 0.5f * lightIntensity * vertColor;
    // Diffus
    vec4 fragColorD = max(dot(N, L), 0.f) * lightIntensity * vertColor;
    // Speculaire
    vec3 R = normalize(2.f * dot(N, L) * N - L);
    vec4 fragColorS = pow(max(dot(R, V), 0.f), shininess) * lightIntensity * vertColor;
    

    fragColor = fragColorA + fragColorD + fragColorS;
    
}
