#ifdef VERTEX

layout ( location = 0 ) in vec4 color;
layout ( location = 1 ) in vec2 position;
layout ( location = 2 ) in float radius;

out float vRadius;
out vec4 vColor;

void main( ) {
  
  gl_Position = vec4( position, 0.0, 1.0 );
  vRadius = radius;
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
//uniform vec2 entityScale;

/*based on https://open.gl/geometry */

layout( points ) in;
layout( line_strip, max_vertices = 32 ) out;

in float vRadius[];
in vec4 vColor[];

out vec4 fColor;

void main() {
  fColor = vColor[ 0 ];

  float baseAng = 6.28318530718 / 31;
  for ( int i = 0; i <= 31; ++i ) {
    float ang = baseAng * i;
    vec2 offset = vec2( cos( ang ) * vRadius[ 0 ],
                        -sin( ang ) * vRadius[ 0 ] );

    //do model transform
    vec2 pos = gl_in[ 0 ].gl_Position.xy + offset;
    //pos *= entityScale;

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

    EmitVertex();
  }

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
