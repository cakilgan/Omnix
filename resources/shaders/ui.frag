#version 330 core

#define OMNIX_UI_FONT 0
#define OMNIX_UI_BUTTON 1

in vec2 TexCoord;
in vec4 Color;
flat in int TexID;
flat in int Type;

uniform sampler2D textures[32];

out vec4 FragColor;

void main() {
    if(Type == OMNIX_UI_FONT){
      float alpha;
      switch(TexID) {
        case 0: alpha = texture(textures[0], TexCoord).r; break;
        case 1: alpha = texture(textures[1], TexCoord).r; break;
        case 2: alpha = texture(textures[2], TexCoord).r; break;
        case 3: alpha = texture(textures[3], TexCoord).r; break;
        case 4: alpha = texture(textures[4], TexCoord).r; break;
        case 5: alpha = texture(textures[5], TexCoord).r; break;
        case 6: alpha = texture(textures[6], TexCoord).r; break;
        case 7: alpha = texture(textures[7], TexCoord).r; break;
        case 8: alpha = texture(textures[8], TexCoord).r; break;
        case 9: alpha = texture(textures[9], TexCoord).r; break;
        case 10: alpha = texture(textures[10], TexCoord).r; break;
        default: FragColor = vec4(1,0,1,1); break;
      }
      FragColor = vec4(Color.x, Color.y, Color.z, alpha);
      return;
    }
    if(Type == OMNIX_UI_BUTTON){
        switch(TexID) {
          case 0: FragColor= Color*texture(textures[0], TexCoord); break;
          case 1: FragColor= Color*texture(textures[1], TexCoord); break;
          case 2: FragColor= Color*texture(textures[2], TexCoord); break;
          case 3: FragColor= Color*texture(textures[3], TexCoord); break;
          case 4: FragColor= Color*texture(textures[4], TexCoord); break;
          case 5: FragColor= Color*texture(textures[5], TexCoord); break;
          case 6: FragColor= Color*texture(textures[6], TexCoord); break;
          case 7: FragColor= Color*texture(textures[7], TexCoord); break;
          case 8: FragColor= Color*texture(textures[8], TexCoord); break;
          case 9: FragColor= Color*texture(textures[9], TexCoord); break;
          case 10:FragColor= Color*texture(textures[10], TexCoord); break;
          default: FragColor = Color; break;
        }
    }
}
