#version 410 core

uniform mat4 MVPMatrix;	// full model-view-projection matrix
uniform mat4 MVMatrix;	// model and view matrix
uniform mat3 NormalMatrix;	// transformation matrix for normals to eye coordinates

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 VertexNormal;
layout(location = 2) in vec2 texIndex;

out vec3 Normal;	// vertex normal in eye coordinates
out vec4 Position;	// vertex position in eye coordinates
out vec2 textCoord;

void main()
{
	textCoord = texIndex;
    Normal = normalize(NormalMatrix * VertexNormal);
    Position = MVMatrix * vec4(VertexPosition, 1.0f);
    gl_Position = MVPMatrix * vec4(VertexPosition, 1.0f);
}