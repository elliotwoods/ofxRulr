#version 150

//OF built-in
uniform mat4 modelViewProjectionMatrix;
uniform mat4 textureMatrix;

in vec4 position;
in vec2 texcoord;

//Custom
uniform sampler2DRect uDepthTexture;
uniform sampler2DRect uWorldTable;
uniform vec2 uDimensions;

void main()
{
	const vec2 halfvec = vec2(0.5, 0.5);
    vec2 depthCoord = texcoord + halfvec;
    // short to int, mm to meters
	float depth = texture(uDepthTexture, depthCoord).x * 65535.0 / 1000.0;

	//Map depth to world.
	vec3 worldPos = vec3(texture(uWorldTable, texcoord).xw, 1.0) * depth;
    
    gl_Position = modelViewProjectionMatrix * vec4(worldPos, 1.0);
}
