#version 400

uniform sampler2D texID;//texture unit

in vec2 texCoord;//texture coordinate

layout (location=0) out vec4 FragColor;

void main()
{
	vec4 texColor = texture( texID, texCoord );
	texColor *= 1.0;

	float disp = 0.005;
	vec4 add_on_color;
	for(int i=-2;i<3;i++){
		for(int j=-2;j<3;j++){
			add_on_color = texture(texID, texCoord+vec2(i,j)*disp);
			if(0.2126 * add_on_color.r + 0.7152 * add_on_color.g + 0.0722 * add_on_color.b < 0.8)
				continue;
			texColor += add_on_color/25.0*0.2;
		}
	}

	FragColor = texColor;

}