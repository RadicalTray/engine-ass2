#version 430

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;

layout (location = 0) out vec3 FragPos;
layout (location = 1) out vec4 Color;
layout (location = 2) out vec3 Normal;
layout (location = 3) out vec2 TexCoord;

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
	vec4 fragPos = model * vec4(aPos, 1.0f);
	vec4 pos = projection * view * fragPos;
	vec4 color = aColor;
	vec3 normal = aNormal;
	vec2 texCoord = aTexCoord;

	gl_Position = pos;
	FragPos = fragPos.xyz;
	Color = color;
	Normal = mat3(model_IT) * normal;
	TexCoord = texCoord;
}
