#version 130

uniform sampler2D shadowMap;
uniform float lightIntensity;
uniform sampler2D colorTexture;
uniform bool blinnPhong;
uniform float shininess;
uniform float eta;
uniform float nearPlane;
uniform float farPlane;
uniform int maxFilterSize;


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

    float depth1;
    float depth2;

    if (lightSpace.w > 0 && lightSpaceScaled.x > 0 && lightSpaceScaled.y > 0 && lightSpaceScaled.z > 0 && lightSpaceScaled.x < 1 && lightSpaceScaled.y < 1 && lightSpaceScaled.z < 1){
            float moment1 = 0.f;
            float moment2 = 0.f;
            for(int i = -maxFilterSize; i<=maxFilterSize; i++){
                    for(int j = -maxFilterSize; j<=maxFilterSize; j++){
                            depth1 = texture2D(shadowMap, vec2(lightSpaceScaled.x, 1.f - lightSpaceScaled.y) + texelSize * vec2(i, j)).z;
                            depth2 = texture2D(shadowMap, vec2(lightSpaceScaled.x, 1.f - lightSpaceScaled.y) + texelSize * vec2(i, j)).t;
                            moment1 += depth1/pow(2 * maxFilterSize + 1, 2);
                            moment2 += depth2/pow(2 * maxFilterSize + 1, 2);
                    }
            }
            float m = moment1;
            float s2 = moment2 - pow(m, 2);
            float d = lightSpaceScaled.z;
            if(lightSpaceScaled.z >= texture2D(shadowMap, vec2(lightSpaceScaled.x, 1.f - lightSpaceScaled.y)).z)
                visibility = s2/(s2 + pow(d - m, 2));
        }
	
    vec4 texColor = texture2D(colorTexture, textCoords);
    fragColor = computeIllumination(0.3,0.3,0.4,texColor,vertNormal, visibility);
}
