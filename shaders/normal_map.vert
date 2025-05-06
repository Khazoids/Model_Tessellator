#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormals;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

uniform mat4 mvp;
uniform mat4 mv;

out VS_OUT {
	vec3 FragPos;
	vec2 TexCoords;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
} vs_out;


void main() 
{	
	vec3 lightPos = vec3(0.5f, 1.0f, 0.3f);
	vec3 viewPos = vec3(0.0f, 0.0f, 3.0f);

	vs_out.FragPos = vec3(mv * vec4(aPos, 1.0));
	vs_out.TexCoords = 1.0 - aTexCoords;

	mat3 normalMatrix = transpose(inverse(mat3(mv)));
	
	vec3 T = normalize(normalMatrix * aTangent);
	vec3 N = normalize(normalMatrix * aNormals);

	// re-orthogonalize T with respect to N
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);

	mat3 TBN = transpose(mat3(T, B, N));
	vs_out.TangentLightPos = TBN * lightPos;
	vs_out.TangentViewPos  = TBN * viewPos;
	vs_out.TangentFragPos  = TBN * vec3(mv * vec4(aPos,1.0));

	gl_Position = mvp * vec4(aPos, 1.0);
}
