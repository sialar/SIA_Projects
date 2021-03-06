#version 130

uniform sampler2D shadowMap;
uniform float lightIntensity;
uniform float shininess;
uniform float eta;
uniform float nearPlane;
uniform float farPlane;
uniform float roughness;
uniform bool blinnPhong;
uniform bool cookTorrance;
uniform bool gooch;
uniform int lightSize;
uniform int maxFilterSize;
uniform int biasCoeff;
uniform bool toon;

in vec3 eyeVector;
in vec3 lightVector;
in vec3 vertNormal;
in vec4 vertColor;
in vec4 lightSpace;

out vec4 fragColor;

vec4 computePhongIllumination(float ka, float kd, float ks, float NdotL, float RdotV, float F, float visibility)
{
    vec4 ambiant = ka * vertColor * lightIntensity;
    vec4 diffuse = kd * vertColor * NdotL * lightIntensity;
    vec4 specular = ks * vertColor * pow( RdotV, shininess ) * lightIntensity;
    return ambiant + visibility * (diffuse + F * specular);
}

vec4 computeBlinnPhongIllumination(float ka, float kd, float ks, float NdotL, float NdotH, float F, float visibility)
{
    vec4 ambiant = ka * vertColor * lightIntensity;
    vec4 diffuse = kd * vertColor * NdotL * lightIntensity;
    vec4 specular = ks * vertColor * pow( NdotH, 4 * shininess ) * lightIntensity;
    return ambiant + visibility * (diffuse + F * specular);
}

vec4 computeCookTorranceIllumination(float NdotH,float NdotV,float VdotH,float NdotL,float F,float k, float visibility)
{
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
    float roughnessValue = r1_b * exp(r2_b);;

    float specular = (F * geoAtt * roughnessValue) / (NdotV * NdotL * 3.141592);
    return visibility * lightIntensity * vertColor * NdotL * (k + specular * (0.8 - k));
}

vec4 computeGoochIllumination(float NdotL, float u_alpha, float u_beta, float visibility)
{
    vec3 u_coolColor = vec3(0,0,1);
    vec3 u_warmColor = vec3(1,0,0);
    vec3 diffuse = vec3(1) * NdotL * lightIntensity;

    // intensity of diffuse lighting [-1, 1]
    float diffuseLighting = NdotL;
    // map intensity of lighting from range [-1; 1] to [0, 1]
    float interpolationValue = (1.0 + diffuseLighting)/2;

    vec3 coolColorMod = u_coolColor + diffuse * u_alpha;
    vec3 warmColorMod = u_warmColor + diffuse * u_beta;
    // interpolation of cool and warm colors according
    // to lighting intensity. The lower the light intensity,
    // the larger part of the cool color is used
    return visibility * vec4(mix(coolColorMod, warmColorMod, interpolationValue),1);
}

vec4 computeToonIllumination(float NdotL, float visibility)
{
    if (NdotL > 0.95)
        return visibility * lightIntensity * vec4(1.0,0.5,0.5,1.0);
    else if (NdotL > 0.5)
        return visibility * lightIntensity * vec4(0.6,0.3,0.3,1.0);
    else if (NdotL > 0.25)
        return visibility * lightIntensity * vec4(0.4,0.2,0.2,1.0);
    else
        return visibility * lightIntensity * vec4(0.2,0.1,0.1,1.0);
}

void main( void )
{
	vec3 lightSpaceScaled = ((lightSpace.xyz / lightSpace.w) + 1.0) / 2;
    float visibility = 1.0;

	vec3 N = normalize(vertNormal);
    vec3 L = normalize(lightVector);
    vec3 V = normalize(eyeVector);
    vec3 R = normalize(2 * dot(N,L) * N - L);

    vec3 halfVector = normalize(L + V);
    float RdotV = max(dot(R, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, halfVector), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float VdotH = max(dot(V, halfVector), 0.0);
    
    float F0 = pow(1-eta,2) / pow(1+eta,2);
    float F = F0 + (1-F0) * pow( (1- VdotH), 5 );
	
    float texelSize = 1.0 / textureSize(shadowMap, 0).x;
    float SMSize = min(lightSize * lightSpaceScaled.z * (farPlane - nearPlane) / (lightSpaceScaled.z * (farPlane - nearPlane) + nearPlane), maxFilterSize);

    float texelSizeProjected = texelSize * (nearPlane + lightSpaceScaled.z * (farPlane - nearPlane)) / nearPlane;
    float bias = biasCoeff * texelSizeProjected * tan(acos(dot(N, L)));

    float depth = texture2D(shadowMap, vec2(lightSpaceScaled.x, 1.0 - lightSpaceScaled.y)).z;
    float blockerDepthAvg = 0.f;
    float blockerNumber = 0.f;
    float blockerDepth;


    if (lightSpace.w > 0 && lightSpaceScaled.x > 0 && lightSpaceScaled.y > 0 && lightSpaceScaled.z > 0 && lightSpaceScaled.x < 1 && lightSpaceScaled.y < 1 && lightSpaceScaled.z < 1){
            for(float i = -SMSize/2.f; i <= SMSize/2.f; i = i + 1){
                    for(float j = -SMSize/2.f; j <= SMSize/2.f; j = j + 1){
                            blockerDepth = texture2D(shadowMap, vec2(lightSpaceScaled.x, 1.0 - lightSpaceScaled.y) + texelSize * vec2(i, j)).z;
                            if(blockerDepth < depth){
                                    blockerNumber = blockerNumber + 1;
                                    blockerDepthAvg += blockerDepth;
                            }
                    }
            }
            blockerDepthAvg /= blockerNumber;

            float filterSize = min(lightSize * (lightSpaceScaled.z - blockerDepthAvg)/blockerDepthAvg, maxFilterSize);

            float potentialBlocker = 0.f;
            blockerNumber = 0.f;
            for(float i = -filterSize/2.f; i<=filterSize/2.f; i = i + 1){
                    for(float j = -filterSize/2.f; j<=filterSize/2.f; j = j + 1){
                            depth = texture2D(shadowMap, vec2(lightSpaceScaled.x, 1.f - lightSpaceScaled.y) + texelSize * vec2(i, j)).z;
                            potentialBlocker++;
                            if(depth < lightSpaceScaled.z - bias){
                                    blockerNumber++;
                            }
                    }
            }
            visibility -= blockerNumber/potentialBlocker;
    }
	
    if (blinnPhong)
        fragColor = computeBlinnPhongIllumination(0.3, 0.3, 0.4, NdotL, NdotH, F, visibility);
    else if (cookTorrance)
        fragColor = computeCookTorranceIllumination(NdotH, NdotV, VdotH, NdotL, F, 0.4, visibility);
    else if (gooch)
        fragColor = computeGoochIllumination(NdotL, 0.25, 0.5, visibility);
    else if (toon)
        fragColor = computeToonIllumination(NdotL, visibility);
    else
        fragColor = computePhongIllumination(0.3, 0.3, 0.4, NdotL, RdotV, F, visibility);
}
