#version 430

#pragma include "ProjectorSimulation.fragh"

PROJECTOR_INPUT(0)
PROJECTOR_INPUT(1)
PROJECTOR_INPUT(2)
PROJECTOR_INPUT(3)
PROJECTOR_INPUT(4)
PROJECTOR_INPUT(5)
PROJECTOR_INPUT(6)
PROJECTOR_INPUT(7)
#define PROJECTOR_COUNT 8

uniform int projectorCount;

//OF built-in
out vec4 fragColor;

float getProjectorFactor(int projectorIndex) {
	switch(projectorIndex) {
		case 0:	return PROJECTOR_FACTOR(0)
		case 1:	return PROJECTOR_FACTOR(1)
		case 2:	return PROJECTOR_FACTOR(2)
		case 3:	return PROJECTOR_FACTOR(3)
		case 4:	return PROJECTOR_FACTOR(4)
		case 5:	return PROJECTOR_FACTOR(5)
		case 6:	return PROJECTOR_FACTOR(6)
		case 7:	return PROJECTOR_FACTOR(7)
	}
}

void main(){
	float factor = 0;
	
	for(int i=0; i<PROJECTOR_COUNT; i++) {
		if(projectorCount > i) {
			factor += getProjectorFactor(i);
		}
	}

	fragColor = vec4(factor, 0, 0, 1);
}