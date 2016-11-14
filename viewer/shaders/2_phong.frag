#version 410

uniform float lightIntensity;
uniform bool blinnPhong;
uniform float shininess;
uniform float eta;

in vec3 eyeVector;
in vec3 lightVector;
in vec3 vertNormal;
in vec4 vertColor;

out vec4 fragColor;

void main( void )
{
    // Here begins the real work.
    vec3 N = normalize(vertNormal);
    vec3 L = normalize(lightVector);
    vec3 V = normalize(eyeVector);

    // Ambient
    vec4 fragColorA = 0.5f * lightIntensity * vertColor;
    // Diffus
    vec4 fragColorD = max(dot(N, L), 0.f) * lightIntensity * vertColor;
    // Speculaire
    vec3 H = normalize(V + L);
    float F0 = pow(1 - eta, 2)/pow(1 + eta, 2);
    float F = F0 + (1 - F0)*(1 - dot(H, V));
    vec4 fragColorS;
    if(blinnPhong){
        fragColorS = pow(max(dot(N, H), 0.f), 4 * shininess) * lightIntensity * vertColor;
    }else{
        vec3 R = normalize(2.f * dot(N, L) * N - L);
        fragColorS = pow(max(dot(R, V), 0.f), shininess) * lightIntensity * vertColor;
    }
    

    fragColor = fragColorA + fragColorD + F * fragColorS;
}
