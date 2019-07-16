#version 330 core

in vec2 UV;

out vec4 color;

uniform sampler2D mainTexture;

void main()
{	
	color = texture(mainTexture, UV).rgba;
}
