#version 430

uniform sampler2DRect AvailabilitySelf;
uniform sampler2DRect AvailabilityAll;

//World parameters
uniform float FeatherSize;
uniform float TargetBrightness;

uniform vec2 TextureResolution;

in vec2 gTexCoord;
in vec2 texCoordVarying;
out float fragColor;

void main(){
	float availabilityAll = texture(AvailabilityAll, texCoordVarying).r;
	float availabilitySelf = texture(AvailabilitySelf, texCoordVarying).r;

	float factor = 1.0f;

	vec2 posProjector = texCoordVarying / TextureResolution;
	{
		posProjector -= 0.5;
		posProjector *= 2.0;
	}

	//Apply the feather
	{
		float featherPosition = max(abs(posProjector.x), abs(posProjector.y));
		float featherFactor = smoothstep(0, FeatherSize, 1.0f - featherPosition);
		factor *= featherFactor;
	}

	//Apply inverse of total availability
	{
		factor *= TargetBrightness / availabilityAll;
	}

	//Mask where we can't project for definite
	{
		factor *= availabilitySelf < 1e-7 ? 0 : 1;
	}

	fragColor = factor;
}