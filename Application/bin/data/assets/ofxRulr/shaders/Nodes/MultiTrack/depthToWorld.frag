#version 430

//OF built-in
uniform vec4 globalColor;

//Custom
uniform sampler2DRect uColorTexture;
uniform int uUseColorTexture;
uniform float uColorScale;

in vec2 gTexCoord;
out vec4 fragColor;

void main() {
	fragColor = globalColor;

	if(uUseColorTexture == 1) {
		fragColor *= texture(uColorTexture, gTexCoord);
		fragColor.rgb *= uColorScale;
		//fragColor.rg = gTexCoord / 500.0f;
		//fragColor.b = 0;
		fragColor.a = 1.0;
	}
}