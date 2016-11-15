#version 410

uniform float lightIntensity;
uniform float roughness;   // 0 : smooth, 1: rough
uniform float eta;

in vec3 eyeVector;
in vec3 lightVector;
in vec3 vertNormal;
in vec4 vertColor;
in vec4 lightSpace;

out vec4 fragColor;

void main( void )
{
    // Normalize vectors
    vec3 N = normalize(vertNormal);
    vec3 L = normalize(lightVector);
    vec3 V = normalize(eyeVector);

    float F0 = pow(1-eta,2) / pow(1+eta,2); // fresnel reflectance at normal incidence
    float k = 0.2; // fraction of diffuse reflection (specular reflection = 1 - k)
    float NdotL = max(dot(N, L), 0.0);

    float specular = 0.0;
    if(NdotL > 0.0)
    {
        // calculate intermediary values
        vec3 halfVector = normalize(L + V);
        float NdotH = max(dot(N, halfVector), 0.0);
        float NdotV = max(dot(N, V), 0.0); // note: this could also be NdotL, which is the same value
        float VdotH = max(dot(V, halfVector), 0.0);
        float mSquared = roughness * roughness;
        mSquared = (mSquared==0.0) ? 0.001 : mSquared;

        // geometric attenuation
        float NH2 = 2.0 * NdotH;
        float g1 = (NH2 * NdotV) / VdotH;
        float g2 = (NH2 * NdotL) / VdotH;
        float geoAtt = min(1.0, min(g1, g2));

        // roughness (or: microfacet distribution function)
        // beckmann distribution function
        float r1_b = 1.0 / ( 4.0 * mSquared * pow(NdotH, 4.0));
        float r2_b = (NdotH * NdotH - 1.0) / (mSquared * NdotH * NdotH);
        // standard normal (gaussian) distribution function
        float variance = mSquared;
        float mean = 0;
        float r1_g = 1 / sqrt(2 * 3.141592 * variance);
        float r2_g = - pow(NdotH - mean,2) / (2 * variance);

        float roughnessValue = r1_g * exp(r2_g);

        // fresnel
        // Schlick approximation
        float fresnel = pow(1.0 - VdotH, 5.0);
        fresnel *= (1.0 - F0);
        fresnel += F0;

        specular = (fresnel * geoAtt * roughnessValue) / (NdotV * NdotL * 3.141592);
    }
       fragColor = lightIntensity * vertColor * NdotL * (k + specular * (1.0 - k));
}
