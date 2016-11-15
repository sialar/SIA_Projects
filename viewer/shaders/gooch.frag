#version 330

uniform float lightIntensity;

in vec3 eyeVector;
in vec3 lightVector;
in vec3 vertNormal;
in vec4 vertColor;
in vec4 lightSpace;

out vec4 fragColor;

void main(void)
{
    vec3 u_coolColor = vec3(159.0f/255, 148.0f/255, 255.0f/255);
    vec3 u_warmColor = vec3(255.0f/255, 75.0f/255, 75.0f/255);
    float u_alpha = 0.25;
    float u_beta = 0.5;

    // normlize vectors for lighting
    vec3 N = normalize(vertNormal);
    vec3 L = normalize(lightVector);
    vec3 V = normalize(eyeVector);

    vec3 diffuse = vertColor.xyz * max( dot(N,L),0 ) * lightIntensity;

    // intensity of diffuse lighting [-1, 1]
    float diffuseLighting = dot(L, N);
    // map intensity of lighting from range [-1; 1] to [0, 1]
    float interpolationValue = (1.0 + diffuseLighting)/2;

    vec3 coolColorMod = u_coolColor + diffuse * u_alpha;
    vec3 warmColorMod = u_warmColor + diffuse * u_beta;
    // interpolation of cool and warm colors according
    // to lighting intensity. The lower the light intensity,
    // the larger part of the cool color is used
    vec3 colorOut = mix(coolColorMod, warmColorMod, interpolationValue);

    // save color
    fragColor = vertColor * vec4(colorOut,1) * lightIntensity;
}
