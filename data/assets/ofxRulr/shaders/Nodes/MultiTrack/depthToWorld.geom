#version 430
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

// App uniforms and attributes
uniform float uMaxDisparity;

in vec2 vTexCoord[3];
out vec2 gTexCoord;

void main() 
{
	const float maxDisparity = uMaxDisparity;
	vec4 A = gl_in[0].gl_Position;
	vec4 B = gl_in[1].gl_Position;
	vec4 C = gl_in[2].gl_Position;

	if(length(A - B) < maxDisparity
		&& length(B - C) < maxDisparity
		&& length(C - A) < maxDisparity) {
		//add triangle
		for (int i = 0; i < 3; i++) {
	        gl_Position = gl_in[i].gl_Position;
	        gTexCoord = vTexCoord[i];
	        EmitVertex();
	    }	
	    EndPrimitive();
	}
}