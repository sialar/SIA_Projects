#version 130
uniform sampler2D permTexture;
uniform sampler2D normalMap;
uniform float radius;
uniform float shininess;
uniform float noiseRate;
uniform float noisePersistence;

uniform mat4 matrix;
uniform mat4 perspective;
uniform mat3 normalMatrix;
uniform bool noColor;
uniform bool withNoise;
uniform vec3 lightPosition;

in vec3 vertex;
in vec3 normal;
in vec3 color;
in vec2 texcoords;

out vec3 eyeVector;
out vec3 lightVector;
out vec4 vertColor;
out vec3 vertNormal;
out float noise;
out vec2 textCoords;


#define ONE 0.00390625
#define ONEHALF 0.001953125

float fade(const in float t) {
  // return t*t*(3.0-2.0*t);
  return t*t*t*(t*(t*6.0-15.0)+10.0);
}
void simplex( const in vec3 P, out vec3 offset1, out vec3 offset2 )
{
  vec3 offset0;
  vec2 isX = step( P.yz, P.xx );
  offset0.x  = dot( isX, vec2( 1.0 ) );
  offset0.yz = 1.0 - isX;
  float isY = step( P.z, P.y );
  offset0.y += isY;
  offset0.z += 1.0 - isY;
  offset2 = clamp(   offset0, 0.0, 1.0 );
  offset1 = clamp( --offset0, 0.0, 1.0 );
}

void simplex( const in vec4 P, out vec4 offset1, out vec4 offset2, out vec4 offset3 )
{
  vec4 offset0;
  vec3 isX = step( P.yzw, P.xxx );
  offset0.x = dot( isX, vec3( 1.0 ) );
  offset0.yzw = 1.0 - isX;
  vec2 isY = step( P.zw, P.yy );
  offset0.y += dot( isY, vec2( 1.0 ) );
  offset0.zw += 1.0 - isY;
  float isZ = step( P.w, P.z );
  offset0.z += isZ;
  offset0.w += 1.0 - isZ;
  offset3 = clamp(   offset0, 0.0, 1.0 );
  offset2 = clamp( --offset0, 0.0, 1.0 );
  offset1 = clamp( --offset0, 0.0, 1.0 );
}

float snoise(const in vec3 P) {
#define F3 0.333333333333
#define G3 0.166666666667
  float s = (P.x + P.y + P.z) * 0.333333333333;
  vec3 Pi = floor(P + s);
  float t = (Pi.x + Pi.y + Pi.z) * 0.166666666667;
  vec3 P0 = Pi - t;
  Pi = Pi * 0.00390625 + 0.001953125;
  vec3 Pf0 = P - P0;
  vec3 o1;
  vec3 o2;
  simplex(Pf0, o1, o2);
  float perm0 = texture2D(permTexture, Pi.xy).a;
  vec3  grad0 = texture2D(permTexture, vec2(perm0, Pi.z)).rgb * 4.0 - 1.0;
  float t0 = 0.6 - dot(Pf0, Pf0);
  float n0;
  if (t0 < 0.0) n0 = 0.0;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot(grad0, Pf0);
  }
  vec3 Pf1 = Pf0 - o1 + 0.166666666667;
  float perm1 = texture2D(permTexture, Pi.xy + o1.xy*0.00390625).a;
  vec3  grad1 = texture2D(permTexture, vec2(perm1, Pi.z + o1.z*0.00390625)).rgb * 4.0 - 1.0;
  float t1 = 0.6 - dot(Pf1, Pf1);
  float n1;
  if (t1 < 0.0) n1 = 0.0;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot(grad1, Pf1);
  }
  vec3 Pf2 = Pf0 - o2 + 2.0 * 0.166666666667;
  float perm2 = texture2D(permTexture, Pi.xy + o2.xy*0.00390625).a;
  vec3  grad2 = texture2D(permTexture, vec2(perm2, Pi.z + o2.z*0.00390625)).rgb * 4.0 - 1.0;
  float t2 = 0.6 - dot(Pf2, Pf2);
  float n2;
  if (t2 < 0.0) n2 = 0.0;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot(grad2, Pf2);
  }
  vec3 Pf3 = Pf0 - vec3(1.0-3.0*0.166666666667);
  float perm3 = texture2D(permTexture, Pi.xy + vec2(0.00390625, 0.00390625)).a;
  vec3  grad3 = texture2D(permTexture, vec2(perm3, Pi.z + 0.00390625)).rgb * 4.0 - 1.0;
  float t3 = 0.6 - dot(Pf3, Pf3);
  float n3;
  if(t3 < 0.0) n3 = 0.0;
  else {
    t3 *= t3;
    n3 = t3 * t3 * dot(grad3, Pf3);
  }
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


void main( void )
{
    if (noColor) vertColor = vec4(1, 1, 1, 1.0 );
    else vertColor = vec4(color, 1.0);
    vec4 vertPosition = matrix * vec4(vertex, 1.0);
    vec4 eyePosition = vec4(0.0, 0.0, 0.0, 1.0);

    lightVector = normalize(lightPosition.xyz - vertPosition.xyz);
    eyeVector = normalize(eyePosition.xyz - vertPosition.xyz);
    vertNormal = normalize(normalMatrix * normal);

    vec3 newPosition;
    // displacement with noise
    noise = noiseRate * perlinNoise(vertex.xyz/radius).x;
    newPosition = vertex + normal * noise;
    // displacement mapping
    if (!withNoise)
    {
        vec3 newNormal = texture2D(normalMap, textCoords).xyz;
        newPosition = vertex + newNormal;
    }
    gl_Position = perspective * matrix * vec4(newPosition, 1.0);
}
