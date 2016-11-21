#version 130
#define M_PI 3.14159265358979323846

uniform sampler2D envMap;
uniform mat4 lightMatrix;
uniform bool transparent;
uniform float eta;

in vec4 vertColor;
in vec3 eyeVector;
in vec3 vertNormal;
in vec3 lightVector;

out vec4 fragColor;

float computeFresnelCoef()
{
    vec3 V = normalize(eyeVector);
    vec3 L = normalize(lightVector);
    vec3 h = normalize(L + V);
    float F0 = pow(1-eta,2) / pow(1+eta,2);
    return F0 + (1-F0) * pow( (1- dot(h,V)), 5 );
}

vec2 computeTexCoords(vec4 vector)
{
    vec2 texCoords = vec2((atan(vector.x, vector.z) / 3.1415926 + 1.0) * 0.5,
                                      (asin(vector.y) / 3.1415926 + 0.5));
    texCoords.y = 1.f - texCoords.y;
    return texCoords;
}

void main( void )
{
    vec4 I = normalize(lightMatrix * vec4(normalize(eyeVector),0.0));
    vec4 N = normalize(lightMatrix * vec4(normalize(vertNormal),0.0));

    // Compute reflexion vector
    vec4 reflexion = normalize(I - 2 * dot(I,N) * N);
    // or
    //vec4 reflexion = reflect(I,N);
    vec4 fragColorFromReflexion = texture2D(envMap, computeTexCoords(reflexion));

    // Compute refraction vector

    float n1 = 1, n2 = 1.33;
    float cosTheta1 = dot(I,N);
    float cosTheta2 = sqrt(1 - pow(n1 / n2,2) * (1 - pow(cosTheta1,2)));
    int sign = (dot(N,I) >= 0) ? 1 : -1;
    vec4 refraction = normalize((n1/n2) * I + ((n1/n2) * cosTheta1 - sign * cosTheta2) * N);
    // or
    //vec4 refraction = refract(I,N,eta);
    vec4 fragColorFromRefraction = texture2D(envMap, computeTexCoords(refraction));

    float F = (transparent) ? computeFresnelCoef() : 1;
    fragColor = F * fragColorFromReflexion + (1 - F) * fragColorFromRefraction;
}
