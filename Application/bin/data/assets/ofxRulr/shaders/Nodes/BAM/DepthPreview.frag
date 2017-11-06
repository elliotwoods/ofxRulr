#version 430

uniform float usingTexture;
uniform float usingColors;

//OF built-in
uniform sampler2DRectShadow depthTextureShadow;
uniform sampler2DRect depthTexture;
uniform vec4 globalColor;

in vec2 gTexCoord;
in vec4 colorVarying;
in vec2 texCoordVarying;
out vec4 fragColor;

void main(){
	fragColor = vec4(0, 0, 0, 1);
	float shadow = texture(depthTextureShadow, vec3(texCoordVarying, 1e-5));
	float depth = texture(depthTexture, texCoordVarying).r;
	fragColor.r = (cos(depth * 100) + 1.0) / 2.0;
	fragColor.g = (cos(depth * 1000) + 1.0) / 2.0;
	fragColor.b = (cos(depth * 10000) + 1.0) / 2.0;
	fragColor.a = depth == 1 ? 0 : 1;
	//fragColor.r = fragColor.r == 1 ? 1 : 0;//gl_FragCoord.x / 1024;//gl_FragCoord.z;
}