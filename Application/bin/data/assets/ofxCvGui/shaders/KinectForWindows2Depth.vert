#version 150

// these are for the programmable pipeline system and are passed in
// by default from OpenFrameworks
uniform mat4 modelViewProjectionMatrix;

in vec4 position;
in vec2 texcoord;

// this is something we're creating for this shader
out vec2 texCoordVarying;

void main()
{
    // here we move the texture coordinates
    texCoordVarying = texcoord;

    // send the vertices to the fragment shader
    gl_Position = modelViewProjectionMatrix * position;
}
