#version 150

uniform sampler2DRect highFrequency;
uniform sampler2DRect lowFrequency;

in vec2 texCoordVarying;
out vec4 outputColor;

uniform float minimum;
uniform float maximum;

void main() {
	float high = texture2DRect(highFrequency, texCoordVarying)[0] * 4.0;
	float low = texture2DRect(lowFrequency, texCoordVarying)[0] / 2.0;

	outputColor = vec4(low , low + high , low, 1.0);
}
