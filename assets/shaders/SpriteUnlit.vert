#version 330 core

struct Ortho {
  float left;
  float right;
  float bottom;
  float top;
  /* unused if every object is at z = 0!
  float near;
  float far;*/
};

uniform Ortho projection;
uniform vec2 entityPosition;
uniform float entityOrientation;
uniform vec2 entityScale;

layout ( location = 0 ) in vec2 position;
layout ( location = 1 ) in vec2 texCoords;

out vec2 interpTexCoords;

void main( ) {
  //do model transform
  float _cos = cos( entityOrientation );
  float _sin = sin( entityOrientation );
  vec2 pos = vec2( position.x * _cos - position.y * _sin,
		       position.y * _cos + position.x * _sin );
  pos *= entityScale;
  pos += entityPosition;

  //TODO: do view transform
  //...
  
  //do projection transform
  //which can be given as a scaling followed by a translation
  //(taken from https://en.wikipedia.org/wiki/Orthographic_projection)
  pos *= vec2( 2 / ( projection.right - projection.left ),
		    2 / ( projection.top - projection.bottom ) );
  pos -= vec2( ( projection.left + projection.right ) / 2.0,
  ( projection.top + projection.bottom ) / 2.0 );
  
  gl_Position = vec4( pos, 0.0, 1.0 );
  interpTexCoords = texCoords;
}
