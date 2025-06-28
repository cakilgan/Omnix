#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in float thickness;
layout(location = 3) in float vType;
layout(location = 4) in vec2 aLocal;

uniform mat4 projection;


out float ovType;
out vec2 oLocal;
out vec4 color;

void main() {
    
    gl_Position = projection * vec4(aPos, 1.0, 1.0);
    color = aColor;
    ovType = vType;
    oLocal = aLocal;
}