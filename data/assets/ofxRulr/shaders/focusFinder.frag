uniform sampler2DRect highFrequency;
uniform sampler2DRect lowFrequency;

void main() {
	vec2 texCoord = gl_TexCoord[0].st;

	float high = texture2DRect(highFrequency, texCoord)[0] * 4.0;
	float low = texture2DRect(lowFrequency, texCoord)[0] / 2.0;

	gl_FragColor = vec4(low , low + high , low, 1.0);
}