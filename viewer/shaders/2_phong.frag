#version 410

uniform sampler2D shadowMap;
uniform float lightIntensity;
uniform bool blinnPhong;
uniform float shininess;
uniform float eta;

in vec3 eyeVector;
in vec3 lightVector;
in vec3 vertNormal;
in vec4 vertColor;

out vec4 fragColor;

vec4 computeIllumination(float ka, float kd, float ks)
{
    // Normalize vectors
    vec3 N = normalize(vertNormal);
    vec3 L = normalize(lightVector);
    vec3 V = normalize(eyeVector);
    vec3 R = normalize(2 * dot(N,L) * N - L);

    // Modèle de Phong
    vec4 ambiant = ka * vertColor * lightIntensity;
    vec4 diffuse = kd * vertColor * max( dot(N,L),0 ) * lightIntensity;
    vec4 phongSpecular = ks * vertColor * pow( max( dot(R,V),0 ), shininess ) * lightIntensity;

    // Modèle de Blinn-Phong
    vec3 h = normalize(L + V);
    vec4 blinnPhongSpecular = ks * vertColor * pow( max( dot(N,h),0 ), 4 * shininess) * lightIntensity;

    // Indice de Fresnel
    float F0 = pow(1-eta,2) / pow(1+eta,2);
    float F = F0 + (1-F0) * pow( (1- dot(h,V)), 5 );

    return (ambiant + diffuse + F * ( (blinnPhong) ? blinnPhongSpecular : phongSpecular));
}

void main( void )
{
    fragColor = computeIllumination(0.3,0.3,0.4);
    fragColor += 0.000001 * texture2D(shadowMap, vec2(0,1));

}
