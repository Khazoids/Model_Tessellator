#version 410 core

uniform sampler2D normalMap;

out vec4 FragColor;

in TES_OUT {
	vec3 Normal;
	vec2 TexCoord;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
} fs_in;

void main()
{
	vec3 normal = texture(normalMap, fs_in.TexCoord).rgb;
	normal = normalize(normal * 2.0 - 1.0);

	vec3 color = vec3(0.3f, 0.3f, 0.3f);
	vec3 ambient = 0.2 * color;

	vec3 lightPos = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
	
	float geometryTerm = max(0.0, dot(normal, lightPos));
    vec3 diffuse = geometryTerm * color;

	 // alpha
    float shininess = 32.0;

    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);

    vec3 halfAngle = normalize(lightPos + viewDir);
    float specularTerm = max(0.0, dot(normal, halfAngle));
    vec3 specular = vec3(0.2) * pow(specularTerm, shininess);
    
    FragColor = vec4(ambient + diffuse + specular, 1.0);
}