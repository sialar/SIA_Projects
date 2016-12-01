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
in vec4 vertColor;
in vec3 vertNormal;
in vec3 vert;

out vec4 fragColor;


vec4 computeIllumination(float ka, float kd, float ks, vec4 texColor, vec3 normal)
{
    // Normalize vectors
    vec3 N = normalize(normal);
    vec3 L = normalize(lightVector);
    vec3 V = normalize(eyeVector);
    vec3 R = normalize(2 * dot(N,L) * N - L);

    // Modèle de Phong
    vec4 ambiant = ka * texColor * lightIntensity;
    vec4 diffuse = kd * texColor * max( dot(N,L),0 ) * lightIntensity;
    vec4 phongSpecular = ks * texColor * pow( max( dot(R,V),0 ), shininess ) * lightIntensity;

    // Modèle de Blinn-Phong
    vec3 h = normalize(L + V);
    vec4 blinnPhongSpecular = ks * texColor * pow( max( dot(N,h),0 ), 4 * shininess) * lightIntensity;

    // Indice de Fresnel
    float F0 = pow(1-eta,2) / pow(1+eta,2);
    float F = F0 + (1-F0) * pow( (1- dot(h,V)), 5 );

    return (ambiant + diffuse + F * ( (blinnPhong) ? blinnPhongSpecular : phongSpecular));
}

vec2 computeTexCoords(vec3 vertex)
{
    float phi = atan(vertex.y, vertex.x);
    float theta = acos(vertex.z);
    vec2 longitudeLatitude = vec2((phi+ 3.1415926)/(2 * 3.1415926), theta/3.1415926);
    /*  // ou alors
    vec2 longitudeLatitude = vec2((atan(vertex.x, vertex.z) / 3.1415926 + 1.0) * 0.5,
                                      (asin(vertex.y) / 3.1415926 + 0.5));
    longitudeLatitude.y = 1.f - longitudeLatitude.y;*/
    return longitudeLatitude;
}

mat3 computeTBN(vec3 vertex)
{
    //float phi = atan(vertex.y, vertex.x);
    //float theta = acos(vertex.z);
    vec3 N = normalize(vertNormal);
    vec3 T = normalize(vec3(N.z, 0.f, -N.x));
    vec3 B = normalize(cross(N,T));
    return transpose(mat3(T,B,N));
}

void main( void )
{
    // Compute texture coordinates
    vec3 vertex = normalize(vert);
    vec2 UV = computeTexCoords(vertex);
    vec4 texColor = texture2D(earthDay, UV);

    // Compute TBN matrix
    mat3 TBN = computeTBN(vertex);

    // Compute normal from texture matrix
    vec3 ns = texture2D(earthNormals, UV).xyz * 2.f - vec3(1.f);
    vec3 ns_modalspace = TBN * ns;
    vec3 ns_globalspace = normalMatrix * ns_modalspace;

    // For texture Mapping
    fragColor = computeIllumination(0.3,0.3,0.4,texColor,ns_globalspace);

    // For normal mapping
    fragColor = computeIllumination(0.3,0.3,0.5,vec4(1),ns) + 0.01 * texColor;
}
