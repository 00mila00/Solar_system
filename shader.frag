#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 color;

uniform sampler2D texture;
uniform sampler2D texture_diffuse1;
uniform int is_texture;

void main()
{   
	if(is_texture == 1)
		FragColor = texture(texture_diffuse1, TexCoord) * color;
	else
		FragColor = color;
}