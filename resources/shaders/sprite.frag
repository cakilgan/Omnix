#version 330 core
in vec2 TexCoord;

uniform sampler2D textures[8];
out vec4 FragColor;
in vec4 Color;
flat in int TexIndex;
void main() {
    switch(TexIndex) {
      case 0: FragColor = texture(textures[0], TexCoord); break;
      case 1: FragColor = texture(textures[1], TexCoord); break;
      case 2: FragColor = texture(textures[2], TexCoord); break;
      case 3: FragColor = texture(textures[3], TexCoord); break;
      case 4: FragColor = texture(textures[4], TexCoord); break;
      case 5: FragColor = texture(textures[5], TexCoord); break;
      case 6: FragColor = texture(textures[6], TexCoord); break;
      case 7: FragColor = texture(textures[7], TexCoord); break;
      default: FragColor = Color; break;
    }
}