#version 150

//OF built-in
uniform vec4 globalColor;

//Custom
out vec4 fragColor;

void main() {
	fragColor = globalColor;
}
