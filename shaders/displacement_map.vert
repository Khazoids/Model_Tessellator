#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormals;
layout (location = 3) in vec2 aTexCoords;
layout (location = 4) in vec3 aTangent;

out VS_OUT {
	vec3 Position;
	vec3 Normal;
	vec2 TexCoord;
	vec3 Tangent;
} vs_out;

void main()
{
	vs_out.Position = aPos;
	vs_out.Normal = aNormals;
	vs_out.TexCoord = 1.0 - aTexCoords;
	vs_out.Tangent = aTangent;
}