#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;
layout(location = 3) in int texLoc;

uniform mat4 projection;

out vec2 TexCoord;
out vec4 Color;
flat out int TexIndex;

void main() {
    gl_Position = projection * vec4(aPos, 1.0, 1.0);
    TexCoord = aTexCoord;
    TexIndex = texLoc;
    Color = aColor;
}