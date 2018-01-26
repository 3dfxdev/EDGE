
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D SceneTexture;
uniform vec2 Scale;
uniform vec2 Offset;

void main()
{
	vec4 color = texture(SceneTexture, Offset + TexCoord * Scale);
	FragColor = vec4(max(max(color.r, color.g), color.b), 0.0, 0.0, 1.0);
}
