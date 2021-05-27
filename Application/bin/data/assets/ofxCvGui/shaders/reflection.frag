  
#version 150

// this is how we receive the texture
uniform sampler2D src_tex_unit0;
uniform sampler2DRect reflection;
uniform float resolutionDivider;
uniform float reflectionBrightness;

uniform vec2 resolution;

in vec2 texCoordVarying;

out vec4 outputColor;
 
void main()
{
    outputColor = texture(src_tex_unit0, texCoordVarying);
    outputColor.rgb += reflectionBrightness * texture(reflection, vec2(gl_FragCoord.x, resolution.y - gl_FragCoord.y) / resolutionDivider).rgb;
}