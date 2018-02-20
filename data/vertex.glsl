#version 410

layout(location = 0) in vec2 position;

uniform mat4 mvp;

out vec3 color;

void main() {

  gl_Position = vec4(mvp*vec4(position, 0.0, 1.0));
  color = vec3(1.0, 1.0, 204/255);

}
