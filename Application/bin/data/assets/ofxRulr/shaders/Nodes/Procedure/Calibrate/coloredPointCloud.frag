#version 430

uniform float usingTexture;
uniform float usingColors;
uniform vec4 globalColor;
uniform float brightnessBoost;

in float depth;
in vec4 colorVarying;
in vec2 texCoordVarying;
out vec4 fragColor;

void main(){
	fragColor = colorVarying * globalColor * vec4(1.0 - brightnessBoost, 1.0 - brightnessBoost, 1.0 - brightnessBoost, 1.0);
	fragColor.rgb += vec3(brightnessBoost, brightnessBoost, brightnessBoost);
}