#version 330 core
out vec4 FragColor;
in vec4 color;

in float ovType;
in vec2 oLocal;

void main() {
    if (ovType == 1.0) {
    vec2 coord = oLocal * 2.0 - 1.0; // [-1, 1]
    float dist = dot(coord, coord);
    if (dist > 1.0) discard;
    }
    FragColor = color;
}