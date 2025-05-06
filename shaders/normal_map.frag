#version 330 core

out vec4 FragColor;

uniform sampler2D normalMap;

in VS_OUT {
	vec3 FragPos;
	vec2 TexCoords;
	vec3 TangentLightPos;
	vec3 TangentViewPos;
	vec3 TangentFragPos;
} fs_in;

void main()
{	
    vec3 normal = texture(normalMap, fs_in.TexCoords).rgb;
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