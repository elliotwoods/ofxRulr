uniform sampler2DRect highFrequency;
uniform sampler2DRect lowFrequency;

void main() {
	vec2 texCoord = gl_TexCoord[0].st;

	float low = texture2DRect(highFrequency, texCoord)[0];
	float high = texture2DRect(lowFrequency, texCoord)[0];

	gl_FragColor = vec4(low, high, 1.0, 1.0);
}