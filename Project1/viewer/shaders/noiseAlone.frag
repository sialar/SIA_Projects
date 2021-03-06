/*
 * To create offsets of one texel and one half texel in the
 * texture lookup, we need to know the texture image size.
 */
#version 130
/*
 * 3D Perlin noise (simplex), in a GLSL fragment shader.
 *
 * Simplex noise is implemented by the functions:
 * float snoise(vec3 P)
 *
 * Author: Stefan Gustavson ITN-LiTH (stegu@itn.liu.se) 2004-12-05
 * Simplex indexing functions by Bill Licea-Kane, ATI
 */

/*
This code was irrevocably released into the public domain
by its original author, Stefan Gustavson, in January 2011.
Please feel free to use it for whatever you want.
Credit is appreciated where appropriate, and I also
appreciate being told where this code finds any use,
but you may do as you like. Alternatively, if you want
to have a familiar OSI-approved license, you may use
This code under the terms of the MIT license:

Copyright (C) 2004 by Stefan Gustavson. All rights reserved.
This code is licensed to you under the terms of the MIT license:

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


/*
 * "permTexture" is a 256x256 texture that is used for both the permutations
 * and the 2D and 3D gradient lookup. For details, see the main C program.
 * "gradTexture" is a 256x256 texture with 4D gradients, similar to
 * "permTexture" but with the permutation index in the alpha component
 * replaced by the w component of the 4D gradient.
 * 3D simplex noise uses only permTexture.
 */

uniform sampler2D permTexture;
uniform float radius; // object size.
uniform float lightIntensity;
uniform float roughness;
uniform float shininess;
uniform float eta;
uniform bool blinnPhong;
uniform bool cookTorrance;
uniform bool gooch;
uniform bool toon;
uniform bool noiseMarble;
uniform bool noiseJade;
uniform bool noiseWood;
uniform bool noiseNormal;
uniform float noiseRate;
uniform float noisePersistence;
/*
 * Both 2D and 3D texture coordinates are defined, for testing purposes.
 */
in vec3 vertPos;
in vec4 vertColor;
in vec3 eyeVector;
in vec3 lightVector;
in vec3 vertNormal;

out vec4 fragColor;
/*
 * To create offsets of one texel and one half texel in the
 * texture lookup, we need to know the texture image size.
 */
#define ONE 0.00390625
#define ONEHALF 0.001953125
// The numbers above are 1/256 and 0.5/256, change accordingly
// if you change the code to use another perm/grad texture size.


/*
 * The 5th degree smooth interpolation function for Perlin "improved noise".
 */
float fade(const in float t) {
  // return t*t*(3.0-2.0*t); // Old fade, yields discontinuous second derivative
  return t*t*t*(t*(t*6.0-15.0)+10.0); // Improved fade, yields C2-continuous noise
}

/*
 * Efficient simplex indexing functions by Bill Licea-Kane, ATI. Thanks!
 * (This was originally implemented as a texture lookup. Nice to avoid that.)
 */
void simplex( const in vec3 P, out vec3 offset1, out vec3 offset2 )
{
  vec3 offset0;

  vec2 isX = step( P.yz, P.xx );         // P.x >= P.y ? 1.0 : 0.0;  P.x >= P.z ? 1.0 : 0.0;
  offset0.x  = dot( isX, vec2( 1.0 ) );  // Accumulate all P.x >= other channels in offset.x
  offset0.yz = 1.0 - isX;                // Accumulate all P.x <  other channels in offset.yz

  float isY = step( P.z, P.y );          // P.y >= P.z ? 1.0 : 0.0;
  offset0.y += isY;                      // Accumulate P.y >= P.z in offset.y
  offset0.z += 1.0 - isY;                // Accumulate P.y <  P.z in offset.z

  // offset0 now contains the unique values 0,1,2 in each channel
  // 2 for the channel greater than other channels
  // 1 for the channel that is less than one but greater than another
  // 0 for the channel less than other channels
  // Equality ties are broken in favor of first x, then y
  // (z always loses ties)

  offset2 = clamp(   offset0, 0.0, 1.0 );
  // offset2 contains 1 in each channel that was 1 or 2
  offset1 = clamp( --offset0, 0.0, 1.0 );
  // offset1 contains 1 in the single channel that was 1
}

void simplex( const in vec4 P, out vec4 offset1, out vec4 offset2, out vec4 offset3 )
{
  vec4 offset0;

  vec3 isX = step( P.yzw, P.xxx );        // See comments in 3D simplex function
  offset0.x = dot( isX, vec3( 1.0 ) );
  offset0.yzw = 1.0 - isX;

  vec2 isY = step( P.zw, P.yy );
  offset0.y += dot( isY, vec2( 1.0 ) );
  offset0.zw += 1.0 - isY;

  float isZ = step( P.w, P.z );
  offset0.z += isZ;
  offset0.w += 1.0 - isZ;

  // offset0 now contains the unique values 0,1,2,3 in each channel

  offset3 = clamp(   offset0, 0.0, 1.0 );
  offset2 = clamp( --offset0, 0.0, 1.0 );
  offset1 = clamp( --offset0, 0.0, 1.0 );
}



/*
 * 3D simplex noise. Comparable in speed to classic noise, better looking.
 */
float snoise(const in vec3 P) {

// The skewing and unskewing factors are much simpler for the 3D case
#define F3 0.333333333333
#define G3 0.166666666667

  // Skew the (x,y,z) space to determine which cell of 6 simplices we're in
        float s = (P.x + P.y + P.z) * F3; // Factor for 3D skewing
  vec3 Pi = floor(P + s);
  float t = (Pi.x + Pi.y + Pi.z) * G3;
  vec3 P0 = Pi - t; // Unskew the cell origin back to (x,y,z) space
  Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

  vec3 Pf0 = P - P0;  // The x,y distances from the cell origin

  // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
  // To find out which of the six possible tetrahedra we're in, we need to
  // determine the magnitude ordering of x, y and z components of Pf0.
  vec3 o1;
  vec3 o2;
  simplex(Pf0, o1, o2);

  // Noise contribution from simplex origin
  float perm0 = texture(permTexture, Pi.xy).a;
  vec3  grad0 = texture(permTexture, vec2(perm0, Pi.z)).rgb * 4.0 - 1.0;
  float t0 = 0.6 - dot(Pf0, Pf0);
  float n0;
  if (t0 < 0.0) n0 = 0.0;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot(grad0, Pf0);
  }

  // Noise contribution from second corner
  vec3 Pf1 = Pf0 - o1 + G3;
  float perm1 = texture(permTexture, Pi.xy + o1.xy*ONE).a;
  vec3  grad1 = texture(permTexture, vec2(perm1, Pi.z + o1.z*ONE)).rgb * 4.0 - 1.0;
  float t1 = 0.6 - dot(Pf1, Pf1);
  float n1;
  if (t1 < 0.0) n1 = 0.0;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot(grad1, Pf1);
  }

  // Noise contribution from third corner
  vec3 Pf2 = Pf0 - o2 + 2.0 * G3;
  float perm2 = texture(permTexture, Pi.xy + o2.xy*ONE).a;
  vec3  grad2 = texture(permTexture, vec2(perm2, Pi.z + o2.z*ONE)).rgb * 4.0 - 1.0;
  float t2 = 0.6 - dot(Pf2, Pf2);
  float n2;
  if (t2 < 0.0) n2 = 0.0;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot(grad2, Pf2);
  }

  // Noise contribution from last corner
  vec3 Pf3 = Pf0 - vec3(1.0-3.0*G3);
  float perm3 = texture(permTexture, Pi.xy + vec2(ONE, ONE)).a;
  vec3  grad3 = texture(permTexture, vec2(perm3, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float t3 = 0.6 - dot(Pf3, Pf3);
  float n3;
  if(t3 < 0.0) n3 = 0.0;
  else {
    t3 *= t3;
    n3 = t3 * t3 * dot(grad3, Pf3);
  }

  // Sum up and scale the result to cover the range [-1,1]
  return 32.0 * (n0 + n1 + n2 + n3);
}

vec3 perlinNoise(in vec3 v) {
    vec3 val = vec3(0.0);
    vec3 amplitude = vec3(1);
    int octaves = 5;
    float lacunarity = 2;
    //float persistence = 0.4;
    v *= 8.0;
    for (int n = 0; n < octaves ; n++) {
        val += snoise(v) * amplitude;
        v *= lacunarity;
        amplitude *= noisePersistence;
    }
    return val;
}

// Retourne l'indice de la couleur qui correspond à la valeur "value" dans une rampe infinie ayant "step" comme pas.
int computeColor(int colorsLength, float value, float step)
{
    for (int k=0; k<floor(1/step); k++)
        for (int l=0; l< colorsLength; l++)
            if (value > l*step + colorsLength*k*step && value <= (l+1)*step + colorsLength*k*step)
                return l;
}

vec4 convertColor(float r, float g, float b)
{
    return vec4(r/255,g/255,b/255,1);
}


vec4 computePhongIllumination(float ka, float kd, float ks, float NdotL, float RdotV, float F, float visibility,vec4 color)
{
    vec4 ambiant = ka * color * lightIntensity;
    vec4 diffuse = kd * color * NdotL * lightIntensity;
    vec4 specular = ks * color * pow( RdotV, shininess ) * lightIntensity;
    return ambiant + visibility * (diffuse + F * specular);
}

vec4 computeBlinnPhongIllumination(float ka, float kd, float ks, float NdotL, float NdotH, float F, float visibility,vec4 color)
{
    vec4 ambiant = ka * color * lightIntensity;
    vec4 diffuse = kd * color * NdotL * lightIntensity;
    vec4 specular = ks * color * pow( NdotH, 4 * shininess ) * lightIntensity;
    return ambiant + visibility * (diffuse + F * specular);
}

vec4 computeCookTorranceIllumination(float NdotH,float NdotV,float VdotH,float NdotL,float F,float k, float visibility,vec4 color)
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
    return 2 * lightIntensity * color * NdotL * (k + visibility * specular * (1 - k));
}

vec4 computeGoochIllumination(float NdotL, float u_alpha, float u_beta, float visibility,vec4 color)
{
    vec3 u_coolColor = vec3(0,0,1);
    vec3 u_warmColor = vec3(1,0,0);
    vec3 diffuse = color.xyz * NdotL * lightIntensity;

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

vec4 computeToonIllumination(float NdotL, float visibility, vec4 color)
{
    if (NdotL > 0.95)
        return visibility * lightIntensity * color;
    else if (NdotL > 0.5)
        return visibility * lightIntensity * 0.6 * color;
    else if (NdotL > 0.25)
        return visibility * lightIntensity * 0.4 * color;
    else
        return visibility * lightIntensity * 0.2 * color;
}

vec4 computeIllumination(float ka, float kd, float ks, vec4 color, bool contrast)
{
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

    float visibility = 1;

    if (contrast)
    {
        if (length(color)/2<0.4)
            visibility = 1;
        else if (length(color)/2<0.5)
            visibility = 0.75;
        else if (length(color)/2<0.6)
            visibility = 0.5;
        else if (length(color)/2<0.8)
            visibility = 0.1;
        else
            visibility = 0.05;
    }
    if (blinnPhong)
        return computeBlinnPhongIllumination(0.3, 0.3, 0.4, NdotL, NdotH, F, visibility,color);
    else if (cookTorrance)
        return computeCookTorranceIllumination(NdotH, NdotV, VdotH, NdotL, F, 0.4, visibility,color);
    else if (gooch)
        return computeGoochIllumination(NdotL, 0.25, 0.5, visibility,color);
    else if (toon)
        return computeToonIllumination(NdotL, visibility,color);
    else
        return computePhongIllumination(0.3, 0.3, 0.4, NdotL, RdotV, F, visibility,color);
}

void main( void )
{
    // Define the colors ramp
    vec4 marbleColors[10];
    marbleColors[0] = 4*vec4(0.25, 0.25, 0.35, 1.0); /* pale blue        */
    marbleColors[1] = 4*vec4(0.25, 0.25, 0.35, 1.0); /* pale blue        */
    marbleColors[2] = 4*vec4(0.20, 0.20, 0.30, 1.0); /* medium blue      */
    marbleColors[3] = 4*vec4(0.20, 0.20, 0.30, 1.0); /* medium blue      */
    marbleColors[4] = 4*vec4(0.25, 0.25, 0.35, 1.0); /* pale blue        */
    marbleColors[5] = 4*vec4(0.25, 0.25, 0.35, 1.0); /* pale blue        */
    marbleColors[6] = 4*vec4(0.15, 0.15, 0.26, 1.0); /* medium dark blue */
    marbleColors[7] = 4*vec4(0.15, 0.15, 0.26, 1.0); /* medium dark blue */
    marbleColors[8] = 4*vec4(0.10, 0.10, 0.20, 1.0); /* dark blue        */
    marbleColors[9] = 4*vec4(0.10, 0.10, 0.20, 1.0); /* dark blue        */

    vec4 jadeColors[10];

    jadeColors[0] = 1.5*convertColor(56, 111, 72);
    jadeColors[1] = 1.5*convertColor(56, 111, 72);
    jadeColors[2] = 1.5*convertColor(56, 111, 72);
    jadeColors[3] = 1.7*convertColor(56, 111, 72);
    jadeColors[4] = 1.7*convertColor(56, 111, 72);
    jadeColors[5] = convertColor(176, 242, 182);
    jadeColors[6] = 2*convertColor(56, 111, 72);
    jadeColors[7] = 2*convertColor(56, 111, 72);
    jadeColors[8] = 2*convertColor(56, 111, 72);
    jadeColors[9] = 2*convertColor(56, 111, 72);

    vec4 woodColors[10];
    woodColors[0] = convertColor(88, 41, 0);
    woodColors[1] = convertColor(88, 41, 0);
    woodColors[2] = 0.9*convertColor(126, 51, 0);
    woodColors[3] = convertColor(126, 51, 0);
    woodColors[4] = 1.2*convertColor(126, 51, 0);
    woodColors[5] = 1.3*convertColor(126, 51, 0);
    woodColors[6] = 1.5*convertColor(126, 51, 0);
    woodColors[7] = 1.5*convertColor(126, 51, 0);
    woodColors[8] = 1.8*convertColor(126, 51, 0);
    woodColors[9] = 2*convertColor(126, 51, 0);

    float noiseValue = (1-noiseRate) + noiseRate * perlinNoise(vertPos.xyz/radius).x;

    int colorIndex = computeColor(10,noiseValue,0.1);
    if (noiseMarble)
        fragColor = computeIllumination(0.2,0.2,0.6,marbleColors[colorIndex], false);

    if (noiseJade)
    {
        colorIndex = computeColor(8,noiseValue,0.1);
        fragColor = computeIllumination(0.2,0.2,0.6,jadeColors[colorIndex], false);
    }
    if (noiseWood)
    {
        colorIndex = computeColor( 10, noiseValue * abs(vertPos.x+0.5*vertPos.y+100)/100, 0.005);
        fragColor = computeIllumination(0.2,0.2,0.6,woodColors[colorIndex], true);
    }
}
