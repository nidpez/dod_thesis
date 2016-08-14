#version 330 core

layout ( location = 0 ) in vec2 position;
layout ( location = 1 ) in float radius;
layout ( location = 2 ) in vec4 color;

out float vRadius;
out vec4 vColor;

void main( ) {
  
  gl_Position = vec4( position, 0.0, 1.0 );
  vRadius = radius;
  vColor = color;
}
