#version 130

uniform sampler2D shadowMap;
uniform float lightIntensity;
uniform bool blinnPhong;
uniform float shininess;
uniform float eta;
uniform float nearPlane;
uniform float farPlane;


in vec3 eyeVector;
in vec3 lightVector;
in vec3 vertNormal;
in vec4 vertColor;
in vec4 lightSpace;

out vec4 fragColor;

vec4 computeIllumination(float ka, float kd, float ks, float visibility)
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
	
	float c = 1.f;
	
	float texelSize = 1.0 / textureSize(shadowMap, 0).x;
	
	float depth1;
	float depth2;
	
	if (lightSpace.w > 0 && lightSpaceScaled.x > 0 && lightSpaceScaled.y > 0 && lightSpaceScaled.z > 0 && lightSpaceScaled.x < 1 && lightSpaceScaled.y < 1 && lightSpaceScaled.z < 1){
		float z = texture2D(shadowMap, vec2(lightSpaceScaled.x, 1.f - lightSpaceScaled.y)).z;
		float d = lightSpaceScaled.z;
		visibility = min(1.f, exp(c * (z - d)));
	}
	
    fragColor = computeIllumination(0.3,0.3,0.4,visibility);
}
