#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormals;
layout (location = 2) in vec2 aTexCoords;

uniform bool hasDisplacementMap = false;
uniform mat4 mvp;

out VS_OUT {
	vec3 Position;
	vec3 Normal;
	vec2 TexCoord;
} vs_out;

void main()
{
	if(hasDisplacementMap)
	{
		vs_out.Position = aPos;
		vs_out.Normal = aNormals;
		vs_out.TexCoord = 1.0 - aTexCoords;
	}
	else
	{
		gl_Position = mvp * vec4(aPos, 1.0);
	}

}