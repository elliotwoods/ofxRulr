#version 430

#pragma include "ProjectorSimulation.fragh"

PROJECTOR_INPUT(0)

//OF built-in
out vec4 fragColor;

void main(){
	float factor = PROJECTOR_FACTOR(0);	
	fragColor = vec4(factor, 0, 0, 1);
}