#version 130

uniform sampler2D shadowMap;
uniform float lightIntensity;
uniform bool blinnPhong;
uniform float shininess;
uniform float eta;
uniform float nearPlane;
uniform float farPlane;
uniform sampler2D colorTexture;
uniform int maxFilterSize;
uniform int lightSize;


in vec3 eyeVector;
in vec3 lightVector;
in vec3 vertNormal;
in vec4 vertColor;
in vec2 textCoords;
in vec4 lightSpace;

out vec4 fragColor;

vec4 computeIllumination(float ka, float kd, float ks, vec4 texColor, vec3 normal, float visibility)
{
    // Normalize vectors
    vec3 N = normalize(vertNormal);
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

    vec4 specular = F * ((blinnPhong) ? blinnPhongSpecular : phongSpecular);
    return (ambiant + visibility * (diffuse + specular));
}

void main( void )
{
    vec3 lightSpaceScaled = ((lightSpace.xyz / lightSpace.w) + 1.0) / 2;
    float visibility = 1.0;

    vec3 N = normalize(vertNormal);
    vec3 L = normalize(lightVector);
    float bias = 0.005*tan(acos(dot(N, L)));
    bias = clamp(bias, 0, 0.01);

    float texelSize = 1.0 / textureSize(shadowMap, 0).x;
    float SMSize = min(lightSize * lightSpaceScaled.z * (farPlane - nearPlane) / (lightSpaceScaled.z * (farPlane - nearPlane) + nearPlane), maxFilterSize);

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

            // PCF
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
	
    vec4 texColor = texture2D(colorTexture, textCoords);
    fragColor = computeIllumination(0.3,0.3,0.4,texColor,vertNormal, visibility);
}
