#version 430

layout(location = 0) out vec4 FragColor;

layout (location = 0) in vec3 aFragPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;

layout (binding = 0) uniform UniformBuffer {
	mat4 projection;
	mat4 view;
	mat4 model;
	mat4 model_IT;
	vec4 viewPos;
	vec4 lightPos;
	vec4 lightClr;
	vec4 ambientClr;
	float ambientStr;
};

void main() {
	vec4 color = vec4(1.0f);
	vec3 ambient = ambientStr * ambientClr.xyz;

	vec3 norm = normalize(aNormal);
	vec3 lightDir = normalize(lightPos.xyz - aFragPos);
	vec3 diffuse = lightClr.xyz * max(dot(norm, lightDir), 0.0);

	float specularStr = lightPos.w;
	vec3 viewDir = normalize(viewPos.xyz - aFragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	vec3 specular = specularStr * pow(max(dot(viewDir, reflectDir), 0.0), 32) * lightClr.xyz;

	FragColor = vec4(ambient + diffuse + specular, 1.0f) * color;
}
