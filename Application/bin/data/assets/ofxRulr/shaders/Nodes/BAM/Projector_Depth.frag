#version 430

uniform float usingTexture;
uniform float usingColors;
uniform vec4 globalColor;

in float depth;
in vec4 colorVarying;
in vec2 texCoordVarying;
out vec4 fragColor;

void main(){
	fragColor.rgb = vec3(0.0, 0.0, 0.0);
	fragColor.r = gl_FragCoord.z;
	fragColor.a = 1.0;
}