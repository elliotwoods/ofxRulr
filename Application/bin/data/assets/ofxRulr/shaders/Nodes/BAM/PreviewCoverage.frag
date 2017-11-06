#version 430

uniform float usingTexture;
uniform float usingColors;

//World parameters
uniform float normalCutoff;
uniform float featherSize;

#define PROJECTOR_INPUT(X) \
	uniform float projectorBrightness ## X ; \
	uniform mat4 projectorVP ## X ; \
	uniform sampler2DRect projectorDepthTexture ## X; \
	uniform sampler2DRectShadow projectorDepthTextureShadow ## X; \
	uniform vec2 projectorResolution ## X; \
	uniform vec3 projectorPosition ## X; \

PROJECTOR_INPUT(0)
PROJECTOR_INPUT(1)
PROJECTOR_INPUT(2)
PROJECTOR_INPUT(3)
PROJECTOR_INPUT(4)
PROJECTOR_INPUT(5)
PROJECTOR_INPUT(6)
PROJECTOR_INPUT(7)

uniform int projectorCount;

//OF built-in
uniform vec4 globalColor;

in vec2 gTexCoord;
in vec4 colorVarying;
in vec2 texCoordVarying;
in vec4 worldVarying;
in vec4 normalWorldVarying;
out vec4 fragColor;

float getProjectorFactor(float projectorBrightness
	, mat4 projectorVP
	, sampler2DRect projectorDepthTexture
	, vec2 projectorResolution
	, vec3 projectorPosition) {

	vec4 posProjector = projectorVP * worldVarying;
	posProjector.xyz /= posProjector.w;

	float factor = 1.0f;

	//brightness
	{
		factor *= projectorBrightness;
	}

	//feather
	{
		float featherPosition = max(abs(posProjector.x), abs(posProjector.y));
		float featherFactor = smoothstep(0, featherSize, 1.0f - featherPosition);
		factor *= featherFactor;
	}

	//distance
	{
		float distance = length(worldVarying.xyz - projectorPosition0);
		float distanceFactor = 1.0f / (distance * distance);
		factor *= distanceFactor;
	}

	//shadow
	{
		vec3 projectorTextureCoords = posProjector.xyz;
		projectorTextureCoords += vec3(1, 1, 1);
		projectorTextureCoords /= vec3(2, 2, 2);
		projectorTextureCoords.xy *= projectorResolution0;

		float bias = 0.00001;
		float depthLookup = texture(projectorDepthTexture, projectorTextureCoords.xy).r;
		float depthProjected = projectorTextureCoords.z;
		float shadowFactor = abs(depthProjected - depthLookup) < bias ? 1.0 : 0.0;
		factor *= shadowFactor;
	}

	//normals
	{
		vec3 lookVector = normalize(worldVarying.xyz - projectorPosition);
		float normalFactor = clamp(dot(lookVector, - normalWorldVarying.xyz), 0, 1);
		if(normalFactor < normalCutoff) {
			normalFactor = 0.0;
		}

		factor *= normalFactor;
	}

	// projector edges
	{
		float edgeFactor = abs(posProjector.x) < 1.0f
		 						&& abs(posProjector.y) < 1.0f
								&& abs(posProjector.z) < 1.0
								? 1.0f : 0.0f;
		factor *= edgeFactor;
	}

	return clamp(factor, 0, 1);
}

#define PROJECTOR_FACTOR(X) getProjectorFactor(projectorBrightness ## X \
	, projectorVP ## X \
	, projectorDepthTexture ## X \
	, projectorResolution ## X \
	, projectorPosition ## X);

float getProjectorFactor(int projectorIndex) {
	switch(projectorIndex) {
		case 0:	return PROJECTOR_FACTOR(0)
		case 1: return PROJECTOR_FACTOR(1)
		case 2: return PROJECTOR_FACTOR(2)
		case 3: return PROJECTOR_FACTOR(3)
		case 4: return PROJECTOR_FACTOR(4)
		case 5: return PROJECTOR_FACTOR(5)
		case 6: return PROJECTOR_FACTOR(6)
		case 7: return PROJECTOR_FACTOR(7)
	}
}

void main(){
	float factor = 1;
	
	for(int i=0; i<projectorCount; i++) {
		factor *= getProjectorFactor(i);
	}

	fragColor = vec4(factor, 0, 0, 1);
}