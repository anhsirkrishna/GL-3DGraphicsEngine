//Gbuffer rendering vertex shader

#version 430

in vec3 vertex;

void main() {
	gl_Position = vec4(vertex, 0.0f);
}