#version 430

layout(location = 0) in vec2 position;

//uniform float scroll;
//uniform vec2 offset;

void main() {

     //vec2 newPosition;

     //newPosition.x = offset.x+position.x*scroll;
     //newPosition.y = offset.y+position.y*scroll;

     gl_Position = vec4(position, 0.0, 1.0);

}
