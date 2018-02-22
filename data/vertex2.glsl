#version 430

layout(location = 0) in vec3 windowCoord;

//uniform float scroll;
//uniform vec2 offset;
uniform mat4 mvp;
void main() {

     //vec2 newPosition;

     //newPosition.x = offset.x+position.x*scroll;
     //newPosition.y = offset.y+position.y*scroll;

  gl_Position = vec4(mvp*vec4(windowCoord, 1.0));

}
