#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec4 aColor;
layout (location = 3) in int aTexID;
layout (location = 4) in int aType;

uniform mat4 projection;

out vec2 TexCoord;
out vec4 Color;
flat out int TexID;
flat out int Type;

void main() {
    gl_Position = projection * vec4(aPos.xy, 0.0, 1.0);
    TexCoord = aUV;
    Color = aColor;
    TexID = aTexID;
    Type = aType;
}