#version 150

uniform sampler2DRect highFrequency;
uniform sampler2DRect lowFrequency;

in vec2 texCoordVarying;
out vec4 outputColor;

void main() {
	float low = texture(lowFrequency, texCoordVarying)[0] / 2.0;
	float high = texture(highFrequency, texCoordVarying)[0] * 4.0;

	outputColor = vec4(low , low + high , low, 1.0);
}
