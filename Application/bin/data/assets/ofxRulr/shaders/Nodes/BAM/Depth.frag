#version 430

in vec4 worldVarying;
in vec4 projectedVarying;
out vec4 fragColor;

void main(){
	vec4 projected = projectedVarying / projectedVarying.w;
	fragColor.r = gl_FragCoord.z;
	fragColor.r = projected.z == gl_FragCoord.z ? 1 : 0;
	fragColor.gba = vec3(0, 0, 1);
}