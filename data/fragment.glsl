#version 410

in vec3 color;
out vec4 FragmentColour;

void main() {

          FragmentColour = vec4(color, 1);

}
