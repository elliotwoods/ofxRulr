#version 430

//OF built-in
uniform mat4 modelViewMatrix;
uniform mat4 modelViewProjectionMatrix;

in vec4 position;
in vec4 normal;

out vec4 colorVarying;
out vec4 worldVarying;
out vec4 normalWorldVarying;

void main()
{
	//we presume no transform, since oF doesn't distinguish well between model and world transforms
	worldVarying = position;
	worldVarying /= position.w;

	normalWorldVarying = vec4(normalize(normal.xyz), 1);

	gl_Position = modelViewProjectionMatrix * position;
}
