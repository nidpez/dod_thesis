#ifdef VERTEX

layout ( location = 0 ) in vec4 color;
layout ( location = 1 ) in vec2 minPosition;
layout ( location = 2 ) in vec2 maxPosition;

out vec4 vColor;

void main() {
  gl_Position = vec4( minPosition, maxPosition );
  vColor = color;
}

#endif

#ifdef GEOMETRY

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
layout ( points ) in;
layout ( line_strip, max_vertices = 5 ) out;

in vec4 vColor[];

out vec4 fColor;

vec2 applyProjection( vec2 pos ) {
  pos *= vec2( 2 / ( projection.right - projection.left ),
               2 / ( projection.top - projection.bottom ) );
  pos -= vec2( ( projection.left + projection.right ) / 2.0,
               ( projection.top + projection.bottom ) / 2.0 );
  return pos;
}
                  
void main() {
  fColor = vColor[ 0 ];

  vec2 minPos = gl_in[ 0 ].gl_Position.xy;
  vec2 maxPos = gl_in[ 0 ].gl_Position.zw;

  // top right
  gl_Position = vec4( applyProjection( maxPos ), 0.0, 1.0 );
  EmitVertex();
  // bottom right
  gl_Position = vec4( applyProjection( vec2( maxPos.x, minPos.y ) ), 0.0, 1.0 );
  EmitVertex();
  // bottom left
  gl_Position = vec4( applyProjection( minPos ), 0.0, 1.0 );
  EmitVertex();
  // top left
  gl_Position = vec4( applyProjection( vec2( minPos.x, maxPos.y ) ), 0.0, 1.0 );
  EmitVertex();
  // close loop
  gl_Position = vec4( applyProjection( maxPos ), 0.0, 1.0 );
  EmitVertex();

  EndPrimitive();
}

#endif

#ifdef FRAGMENT

in vec4 fColor;

out vec4 outColor;

void main() {
  outColor = fColor;
}

#endif
