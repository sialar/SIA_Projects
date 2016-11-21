#version 130

in vec3 vertColor;
in vec3 vertNormal;
out vec4 fragColor;

void main( void )
{
    fragColor = vec4(gl_FragCoord.z, gl_FragCoord.z, gl_FragCoord.z , 1.0 );
}
