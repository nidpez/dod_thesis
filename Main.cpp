#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL2.h>

#include <cstdio>
#include <ctime>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <sstream>
#include <cerrno>
#include <cmath>
#include <deque>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <cstdarg>

const float PIXELS_PER_UNIT = 4.0f;

//math
struct Vec2 {
  union {
    float x, u;
  };
  union {
    float y, v;
  };
  Vec2( float x, float y )
    : x( x ), y( y )
  {}
  Vec2() : x( 0.0f ), y( 0.0f )
  {}
};

Vec2 operator+( Vec2 a, Vec2 b );

Vec2 operator-( Vec2 a, Vec2 b );

Vec2 operator*( Vec2 a, Vec2 b );

float dot( Vec2 a, Vec2 b );

Vec2 rotate( Vec2 vec, float orientation );

//resources
struct TextureRsrc {
  const char* name;
  const uint32_t width, height;
  const uint32_t glId;
};
  
TextureRsrc loadTexture( const char* name );
void deleteTexture( TextureRsrc texture );

//entity and component managers

/*Entity Handle system based on http://bitsquid.blogspot.com.co/2014/08/building-data-oriented-entity-system.html and http://gamesfromwithin.com/managing-data-relationships*/
const int HANDLE_INDEX_BITS = 21;
const int HANDLE_GENERATION_BITS = 32 - HANDLE_INDEX_BITS;
//With 21 index bits 2 million entities are possible at a time.
const int MAX_ENTITIES = 1 << HANDLE_INDEX_BITS;

struct EntityHandle {
  uint32_t index : HANDLE_INDEX_BITS;
  uint32_t generation : HANDLE_GENERATION_BITS;
  EntityHandle( uint32_t index, uint32_t generation )
    : index( index ), generation ( generation )
  {}
  EntityHandle() : index( 0 ), generation( 0 )
  {}
  uint32_t toInt() const;
};

class EntityManager {
  const uint32_t MIN_FREE_INDICES = 1024;
  struct Generation { //can't just use uint32_t since they overflow at different values
    uint32_t generation : HANDLE_GENERATION_BITS;
    Generation(uint32_t g)
      : generation( g )
    {}
  };
  std::vector< Generation > generations;
  std::deque< uint32_t > freeIndices;
public:
  EntityHandle create();
  void destroy( EntityHandle entity );
  bool isAlive( EntityHandle entity ) const;
};

typedef uint32_t ComponentIndex;

struct LookupResult {
  ComponentIndex index;
  bool found;
};

struct TransformComp {
  EntityHandle entity;
  Vec2 position;
  Vec2 scale;
  float orientation;
  TransformComp( EntityHandle entity, Vec2 position, Vec2 scale, float orientation )
    : entity( entity ), position( position ), scale( scale ), orientation( orientation )
  {}
};

class TransformManager {
  std::vector< TransformComp > transformComps;
  std::unordered_map< uint32_t, ComponentIndex > map;
  /*Can't trust the index retrieved to belong to the same entity
   even within the same frame since they can change
   when some entity's component is deleted*/
  ComponentIndex getTempComponentIndex( EntityHandle entity ) const;
public:
  void set( EntityHandle entity, Vec2 position, Vec2 scale, float orientation );
  void remove( ComponentIndex compInd );
  bool has( EntityHandle entity ) const;
};

class CircleColliderManager {
  struct CircleColliderComp {
    EntityHandle entity;
    Vec2 center;
    float radius;
    CircleColliderComp( EntityHandle entity, Vec2 center, float radius )
      : entity( entity ), center( center ), radius( radius )
    {}
  };
  std::vector< CircleColliderComp > circleColliderComps;
  std::unordered_multimap< uint32_t, ComponentIndex > map;
  //TODO allow multiple colliders per entity (with linked list?)
  /*Can't trust the indices retrieved to belong to the same entity
   even within the same frame since they can change
   when some entity's component is deleted*/
  std::vector< ComponentIndex > getTempComponentIndices( EntityHandle entity ) const;
public:
  void add( EntityHandle entity, Vec2 center, float radius );
  void remove( ComponentIndex compInd );
  uint32_t count( EntityHandle entity ) const;
};

//Axis aligned bounding box 
struct Rect {
  Vec2 min;
  Vec2 max;
  Rect( Vec2 min, Vec2 max )
    : min( min ), max( max )
  {}
};

//assumes a single possible shader per system
struct RenderInfo {
  //shader's uniforms' locations
  //TODO use 2x2 matrix for projection transform
  //for now, left, right, bottom, top
  int32_t projUnifLoc[ 4 ];
  uint32_t vboIds[ 2 ]; //expect to mostly use one
  uint32_t vaoId;
  uint32_t shaderProgramId;
};

class SpriteManager {
  struct SpriteComp {
    EntityHandle entity;
    TextureRsrc baseTexture;
    Rect texCoords;
    Vec2 size;
    //transform cache
    Vec2 position;
    Vec2 scale;
    float orientation;
  };
  std::vector< SpriteComp > spriteComps;
  std::unordered_map< uint32_t, ComponentIndex > map;
  //rendering data
  struct Pos {
    Vec2 pos;
  };
  struct UV {
    Vec2 uv;
  };
  RenderInfo renderInfo;
  //TODO merge into single vertex attrib pointer
  Pos* posBufferData;
  UV* texCoordsBufferData;
  /*Can't trust the index retrieved to belong to the same entity
   even within the same frame since they can change
   when some entity's component is deleted*/
  std::vector< LookupResult > lookup( std::vector< EntityHandle > entities ) const;
public:
  void initialize();
  void shutdown();
  void set( EntityHandle entity, TextureRsrc texture, Rect texCoords );
  void remove( EntityHandle entity );
  bool has( EntityHandle entity ) const;
  void updateAndRender( const TransformManager& transformManager );
  void setOrthoProjection( float aspectRatio, float height );
};

class CircleColliderDebugRenderer {
  //buffer data (posx, posy, radius)
  GLfloat* bufferData;
  GLuint vaoId;
  GLuint vboId;
  //shader handle
  GLuint shaderProgramId;
  //shader's constants' locations
  int32_t projLeftUnifLoc;
  int32_t projRightUnifLoc;
  int32_t projBottomUnifLoc;
  int32_t projTopUnifLoc;
public:
  CircleColliderDebugRenderer();
  ~CircleColliderDebugRenderer();
  void setOrthoProjection( float aspectRatio, float height );
};

//shader and material stuff
int loadShaderSourceFile( const char* name, char** source );

int compileShader( const char* name, const GLenum type, GLuint* shaderId );

int createShaderProgram( GLuint* shaderProgramId, const char* vertShaderId, const char* fragShaderId, const char* geomShaderId = 0 );


//error handling and logging
void printGlfwError( int error, const char* description );

void APIENTRY
printOpenglError( GLenum source, GLenum type, GLuint id, GLenum severity,
		  GLsizei length, const GLchar* message, const void* userParam );

class Logger {
  static constexpr const char* LOG_FILE_NAME = "log.txt";
  static FILE* log;
public:
  static void initialize();
  static void shutdown();
  static void write( const char* const format, ... );
};

//misc
GLFWwindow* createWindowAndGlContext( const char* const windowTitle );

int main() {
  //initialize managers
  EntityManager entityManager;
  TransformManager transformManager;
  CircleColliderManager circleColliderManager;
  SpriteManager spriteManager;
  Logger::initialize();
  spriteManager.initialize();

  GLFWwindow* window = createWindowAndGlContext( "Space Adventure (working title)" );

  CircleColliderDebugRenderer circleColliderDebugRenderer;

  //configure viewport and orthographic projection
  //TODO put projection info in a Camera component
  int windowWidth, windowHeight;
  glfwGetWindowSize( window, &windowWidth, &windowHeight );
  glViewport( 0, 0, windowWidth, windowHeight );
  float aspect = windowWidth / ( float )windowHeight;

  spriteManager.setOrthoProjection( aspect, 100 );
  circleColliderDebugRenderer.setOrthoProjection( aspect, 100 );
  
  //load textures
  TextureRsrc astronautTex = loadTexture( "astronaut.png" );
  TextureRsrc planetTex = loadTexture( "planetSurface.png" );
  
  //astronaut transform
  // Vec2 pos1 = { -30.0f, 30.0f };
  // float angle = 0.0f;
  // float frameW1 = w1 / 5.0f;
  // const Vec2 BASE_SCALE = { frameW1 / PIXELS_PER_UNIT, h1 / PIXELS_PER_UNIT };
  // Vec2 scale = { 1.0f, 1.0f };
  // float speed = 20.0f;
  // //bounding geometry
  // float r1 = BASE_SCALE.x / 2.0f;
  // //uv coords
  // //use inverted Y coords to work with SOIL
  // GLfloat texCoords1DefaultX[] = {
  //   0.0f,		1.0f,
  //   1.0f / 5.0f,	1.0f,
  //   1.0f / 5.0f,	0.0f,
  //   0.0f,		0.0f };
  // GLfloat texCoords1FlippedX[] = {
  //   1.0f / 5.0f,	1.0f,
  //   0.0f,		1.0f,
  //   0.0f,		0.0f,
  //   1.0f / 5.0f,	0.0f };
  // GLfloat texCoords1[ 8 ];
  // for ( int i = 0; i < 8; ++i ) {
  //   texCoords1[ i ] = texCoords1DefaultX[ i ];
  // }
  EntityHandle astronautId = entityManager.create();
  transformManager.set( astronautId, { -30.0f, 30.0f }, { 1.0f, 1.0f }, 0.0f );
  circleColliderManager.add( astronautId, { 0.0f, 0.0f }, 1.0f );
  Rect runFrames[] = {
    { { 0.0f, 0.0f },		{ 1.0f / 5.0f, 1.0f } },
    { { 1.0f / 5.0f, 0.0f },	{ 2.0f / 5.0f, 1.0f } },
    { { 2.0f / 5.0f, 0.0f },	{ 3.0f / 5.0f, 1.0f } },
    { { 3.0f / 5.0f, 0.0f },	{ 4.0f / 5.0f, 1.0f } } };
  Rect jumpFrames[] = { { { 4.0f / 5.0f, 0.0f }, { 1.0f, 1.0f } } };
  spriteManager.set( astronautId, astronautTex, runFrames[ 0 ] );
  animationManager.add( astronautId, ( char* )"run", runFrames, 24/*fps*/, true/*looping*/, false/*autoplay*/ );
  animationManager.add( astronautId, ( char* )"jump", jumpFrames, 24, false, false );
  // //planet transform
  // float pos2[] = { 5.0f, 0.0f };
  // //bounding geometry
  // float r2 = ( w2 / 2.0f ) / PIXELS_PER_UNIT;
  // //uv coords
  // //use inverted Y coords to work with SOIL
  // GLfloat texCoords2[] = {
  //   0.0f,	1.0f,
  //   1.0f,	1.0f,
  //   1.0f,	0.0f,
  //   0.0f,	0.0f };
  EntityHandle planetId = entityManager.create();
  transformManager.set( planetId, { 5.0f, 0.0f }, { 1.0f, 1.0f }, 0.0f );
  circleColliderManager.add( planetId, { 0.0f, 0.0f }, 1.0f );
  Rect planetTexCoords( { 0.0f, 0.0f }, {1.0f, 1.0f } );
  spriteManager.set( planetId, astronautTex, planetTexCoords );
  
  //main loop      
  // float tempPos[ 2 ];
  // tempPos[ 0 ] = pos1.x;
  // tempPos[ 1 ] = pos1.y;
  // float angle2 = 0.0f;
  double t1 = glfwGetTime();
  double t2;
  double deltaT = 0.0;
  while ( !glfwWindowShouldClose( window ) ) {
    //process input
    glfwPollEvents();
    if ( glfwGetKey( window, GLFW_KEY_ESCAPE ) == GLFW_PRESS ) {
      glfwSetWindowShouldClose( window, true );
    }
    //move with WASD
    //angle = 0.0f;
    float dir[] = { 0.0f, 0.0f };
    if ( glfwGetKey( window, GLFW_KEY_A ) == GLFW_PRESS ||
	 glfwGetKey( window, GLFW_KEY_LEFT ) == GLFW_PRESS ) {
      dir[ 0 ] -= 1.0f;
    }
    if ( glfwGetKey( window, GLFW_KEY_D ) == GLFW_PRESS ||
	 glfwGetKey( window, GLFW_KEY_RIGHT ) == GLFW_PRESS ) {
      dir[ 0 ] += 1.0f;
    }
    if ( glfwGetKey( window, GLFW_KEY_W ) == GLFW_PRESS ||
	 glfwGetKey( window, GLFW_KEY_UP ) == GLFW_PRESS ) {
      dir[ 1 ] += 1.0f;
    }
    if ( glfwGetKey( window, GLFW_KEY_S ) == GLFW_PRESS ||
	 glfwGetKey( window, GLFW_KEY_DOWN ) == GLFW_PRESS ) {
      dir[ 1 ] -= 1.0f;
    }
    float mag = sqrt( dir[ 0 ] * dir[ 0 ] + dir[ 1 ] * dir[ 1 ] );
    if ( mag > 0.0f ) { 
      //normalize direction vector
      dir[ 0 ] /= mag;
      dir[ 1 ] /= mag;
      // if ( dir[ 0 ] < 0 ) {	
      // 	for ( int i = 0; i < 8; ++i ) {
      // 	  texCoords1[ i ] = texCoords1FlippedX[ i ];
      // 	}
      // } else {
      // 	for ( int i = 0; i < 8; ++i ) {
      // 	  texCoords1[ i ] = texCoords1DefaultX[ i ];
      // 	}
      // }
      //update position
      //TODO calculate new deltaT or keep using last frame's?
      // tempPos[ 0 ] += dir[ 0 ] * speed * deltaT;
      // tempPos[ 1 ] += dir[ 1 ] * speed * deltaT;
      // //update orientation
      // if ( dir[ 0 ] < 0 ) {
      // 	angle = atan2( -dir[ 1 ], -dir[ 0 ] );
      // } else {
      // 	angle = atan2( dir[ 1 ], dir[ 0 ] );
      // }
    }

    //detect collisions
    // float dx = tempPos[ 0 ] - pos2[ 0 ];
    // float dy = tempPos[ 1 ] - pos2[ 1 ];
    // float dist = sqrt( dx * dx + dy * dy );
    // if ( dist >= r1 + r2 ) {
    //   //no collision detected
    //   pos1.x = tempPos[ 0 ];
    //   pos1.y = tempPos[ 1 ];
    // } else {
    //   tempPos[ 0 ] = pos1.x;
    //   tempPos[ 1 ] = pos1.y;
    // }
    
    //render scene
    glClear( GL_COLOR_BUFFER_BIT );
    spriteManager.updateAndRender( transformManager );
  
    // //render debug shapes
    // glUseProgram( circleColliderDebugRenderer.shaderProgramId );
    // glBindVertexArray( circleColliderDebugRenderer.vaoId );
    // circlePositions[ 0 ] = pos1.x;
    // circlePositions[ 1 ] = pos1.y;
    // circlePositions[ 7 ] = pos2[ 0 ];
    // circlePositions[ 8 ] = pos2[ 1 ];
    // glBindBuffer( GL_ARRAY_BUFFER, circleColliderDebugRenderer.vboId );
    // glBufferData( GL_ARRAY_BUFFER, sizeof( circlePositions ), circlePositions, GL_STATIC_DRAW );
    // glDrawArrays( GL_POINTS, 0, 2 );
    
    glfwSwapBuffers( window );

    //pseudo v-sync at 60fps
    t2 = glfwGetTime();
    deltaT = t2 - t1;
    if ( deltaT < ( 1 / 60.0 ) ) {
      double remaining = ( 1 / 60.0 ) - deltaT;
      //printf( "d = %f, 1/60 = %f, rem = %f, nanos = %f\n", deltaT, 1 / 60.0, remaining, remaining * 1.0e+9 );
      timespec amount = { 0, ( long )( remaining * 1.0e+9 ) };
      nanosleep( &amount, &amount );
      
      t2 = glfwGetTime();
      deltaT = t2 - t1;
    }
    t1 = t2;
  }
  Logger::write( "Main loop exited.\n" );
  
  //free OpenGL resources
  glUseProgram( 0 );
  deleteTexture( astronautTex );
  deleteTexture( planetTex );
  Logger::write( "Resources freed.\n" );

  glfwDestroyWindow( window );
  glfwTerminate();

  //shut down managers
  spriteManager.shutdown();
  Logger::shutdown();
  
  return 0;
}

GLFWwindow* createWindowAndGlContext( const char* const windowTitle ) {  
  glfwSetErrorCallback( printGlfwError );
  
  if ( !glfwInit() ) {
    //TODO assert
    printf( "GLFW failed to initialize!\n" );
  }
  glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
  glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
  glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
#ifndef NDEBUG
  glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE );
#endif
  /*NOTE: there's a bug that affects video mode setting in X11 with GLFW
    where setting a higher resolution crashes the program with printed message:
    "X Error of failed request:  BadValue (integer parameter out of range for operation)
    Major opcode of failed request:  140 (RANDR)
    Minor opcode of failed request:  21 (RRSetCrtcConfig)
    Value in failed request:  0x0",
    and setting a lower resolution fails to restore original mode
    after the program closes.
    So, as a workaround, we use the current mode and avoid both problems.
    (Code for creating a "windowed fullscreen" window taken from the official GLFW docs: http://www.glfw.org/docs/latest/window.html#window_full_screen.)
  */
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode( monitor ); 
  glfwWindowHint( GLFW_RED_BITS, mode->redBits ); 
  glfwWindowHint( GLFW_GREEN_BITS, mode->greenBits ); 
  glfwWindowHint( GLFW_BLUE_BITS, mode->blueBits ); 
  glfwWindowHint( GLFW_REFRESH_RATE, mode->refreshRate );
  GLFWwindow* window = glfwCreateWindow( mode->width, mode->height , windowTitle, monitor, nullptr );
  if ( !window ) {
    //TODO assert
    printf( "Error creating the window!\n" );
  }
  glfwMakeContextCurrent( window );
  /*NOTE: glfwSwapInterval( 1 ) causes an error on the dev machine where terminating
    the program leaves the last rendered frame there covering everything.
    TODO: investigate glfwSwapInterval bug
  */
  glfwSwapInterval( 0 );
  //configure keyboard input
  glfwSetInputMode( window, GLFW_STICKY_KEYS, 1 );
  Logger::write( "Window created.\nOpenGL version %d.%d used.\n", glfwGetWindowAttrib( window, GLFW_CONTEXT_VERSION_MAJOR ), glfwGetWindowAttrib( window, GLFW_CONTEXT_VERSION_MINOR ) );  
  //initialize advanced opengl functionality
  glewExperimental = GL_TRUE;
  GLenum glewError = glewInit();
  if ( glewError != GLEW_OK ) {
    //TODO assert
    printf( "GLEW failed to initialize! %s\n", glewGetErrorString( glewError ) );
  }
  if ( !GLEW_VERSION_3_3 ) {
    //TODO assert
    //is this even possible when glfw already gave us a 3.3 context?
    printf( "Required OpenGL version 3.3 unavailable, says GLEW!\n" );
  }
  Logger::write( "OpenGL functionality successfully loaded.\n" );

  //setup opengl debugging
#ifndef NDEBUG
  if ( GLEW_KHR_debug ) {
    Logger::write( "Core KHR Debug extension found.\n" );
    glDebugMessageCallback( printOpenglError, nullptr );
    glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
  } else {
    Logger::write( "Core KHR Debug extension unavailable!\n" );
  }
#endif
  //basic gl configurations
  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

  return window;
}


int loadShaderSourceFile( const char* name, char** source ) {
  int error = 0;
  std::ifstream file( name );
  if ( file.is_open() && file.good() ) {
    std::string line;
    std::stringstream strStream; 
    while ( getline( file, line ) ) {
      strStream << line << std::endl;
    }
    file.close();
    std::string sourceStr = strStream.str();
    char* sourceContent = new char[ sourceStr.length() + 1 ];
    std::strcpy( sourceContent, sourceStr.c_str() );
    *source = sourceContent;
  }
  return error;
}

int compileShader( const char* name, const GLenum type, GLuint* shaderId ) {
  GLchar* source;
  int fileError = loadShaderSourceFile( name, &source );
  if ( fileError ) {
    printf( "Unable to open vertex shader source file '%s': %s\n", name,
	    strerror( fileError ) );
    return 1;
  } 
  *shaderId = glCreateShader( type );
  glShaderSource( *shaderId, 1, ( const char** )&source, nullptr );
  glCompileShader( *shaderId );
  int32_t compiled; 
  glGetShaderiv( *shaderId, GL_COMPILE_STATUS, &compiled );
  if ( compiled == GL_FALSE ) {
    int32_t maxLength;
    glGetShaderiv( *shaderId, GL_INFO_LOG_LENGTH, &maxLength );
    GLchar* errorLog = ( GLchar* )malloc( sizeof( GLchar ) * maxLength );
    glGetShaderInfoLog( *shaderId, maxLength, &maxLength, errorLog );
    Logger::write( "Shader error:\n\t%s\n", errorLog );
    free( errorLog );
    Logger::write( "Shader source:\n\"%s\"\n", source );
    glDeleteShader( *shaderId );
    return 1;
    //TODO on shader error replace with default ugly shader instead of quitting
  }
  delete[] source;
  return 0;
}

int createShaderProgram( GLuint* shaderProgramId, const char* vertShaderFile, const char* fragShaderFile, const char* geomShaderFile ) {
  GLuint vertShaderId;
  int error = compileShader( vertShaderFile, GL_VERTEX_SHADER, &vertShaderId );
  if ( error ) {
    return error;
  } 
  GLuint geomShaderId;
  if ( geomShaderFile != 0 ) {
    error = compileShader( geomShaderFile, GL_GEOMETRY_SHADER,
			   &geomShaderId );
  }
  GLuint fragShaderId;  
  error = compileShader( fragShaderFile, GL_FRAGMENT_SHADER, &fragShaderId );
  if ( error ) {
    return error;
  }
  *shaderProgramId  = glCreateProgram();
  glAttachShader( *shaderProgramId, vertShaderId );
  if ( geomShaderFile != 0 ) {
    glAttachShader( *shaderProgramId, geomShaderId );
  }
  glAttachShader( *shaderProgramId, fragShaderId );
  glLinkProgram( *shaderProgramId );
  int32_t linked = 0;
  glGetProgramiv( *shaderProgramId, GL_LINK_STATUS, ( int * )&linked );
  if( linked == GL_FALSE ) {
    int32_t maxLength;
    glGetProgramiv( *shaderProgramId, GL_INFO_LOG_LENGTH, &maxLength );
    GLchar *errorLog = ( GLchar * )malloc( sizeof( GLchar )*maxLength );
    glGetProgramInfoLog( *shaderProgramId, maxLength, &maxLength, errorLog );
    printf( "Shader Program error:\n\t%s\n", errorLog );
    free( errorLog );
    glDeleteProgram( *shaderProgramId );
    glDeleteShader( vertShaderId );
    if ( geomShaderFile != 0 ) {
      glDeleteShader( geomShaderId );
    }
    glDeleteShader( fragShaderId );
    return 1;
  }  
  glUseProgram( *shaderProgramId );
  glDetachShader( *shaderProgramId, vertShaderId );
  if ( geomShaderFile != 0 ) {
    glDetachShader( *shaderProgramId, geomShaderId );
  }
  glDetachShader( *shaderProgramId, fragShaderId );
  glDeleteShader( vertShaderId );
  glDeleteShader( geomShaderId );
  glDeleteShader( fragShaderId );
  return 0;
}

void printGlfwError( int error, const char* description ) {
  char* errorName;
  switch ( error ) {
  case 0x00010001:
    errorName = ( char* )"GLFW_NOT_INITIALIZED";
    break;
  case 0x00010002:
    errorName = ( char* )"GLFW_NO_CURRENT_CONTEXT";
    break;
  case 0x00010003:
    errorName = ( char* )"GLFW_INVALID_ENUM";
    break;
  case 0x00010004:
    errorName = ( char* )"GLFW_INVALID_VALUE";
    break;
  case 0x00010005:
    errorName = ( char* )"GLFW_OUT_OF_MEMORY";
    break;
  case 0x00010006:
    errorName = ( char* )"GLFW_API_UNAVAILABLE";
    break;
  case 0x00010007:
    errorName = ( char* )"GLFW_VERSION_UNAVAILABLE";
    break;
  case 0x00010008:
    errorName = ( char* )"GLFW_PLATFORM_ERROR";
    break;
  case 0x00010009:
    errorName = ( char* )"GLFW_FORMAT_UNAVAILABLE";
    break;
  }
  Logger::write( "GLFW error %s ocurred:\n\t%s\n", errorName, description );
}

void APIENTRY
printOpenglError( GLenum source, GLenum type, GLuint id, GLenum severity,
		 GLsizei length, const GLchar* message, const void* userParam ) {
  char* sourceStr;
  char* typeStr;
  char* severityStr;
  char* userParamStr;
  switch( source ) {
  case GL_DEBUG_SOURCE_API :
    sourceStr = ( char* )"API";
    break;
  case GL_DEBUG_SOURCE_APPLICATION :
    sourceStr = ( char* )"Application";
    break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM :
    sourceStr = ( char* )"Window System";
    break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER :
    sourceStr = ( char* )"Shader Compiler";
    break;
  case GL_DEBUG_SOURCE_THIRD_PARTY :
    sourceStr = ( char* )"Third Party";
    break;
  case GL_DEBUG_SOURCE_OTHER :
    sourceStr = ( char* )"Other";
    break;
  default :
    sourceStr = ( char* )"Undefined";
  }
  switch( type ) {
  case GL_DEBUG_TYPE_ERROR :
    typeStr = ( char* )"Error";
    break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR :
    typeStr = ( char* )"Deprecated Behavior";
    break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR :
    typeStr = ( char* )"Undefined Behavior";
    break;
  case GL_DEBUG_TYPE_PERFORMANCE :
    typeStr = ( char* )"Performance";
    break;
  case GL_DEBUG_TYPE_OTHER :
    typeStr = ( char* )"Other";
    break;
  default :
    typeStr = ( char* )"Undefined";
  }
  switch( severity ) {
  case GL_DEBUG_SEVERITY_HIGH :
    severityStr = ( char* )"High";
    break;    
  case GL_DEBUG_SEVERITY_MEDIUM :
    severityStr = ( char* )"Medium";
    break;
  case GL_DEBUG_SEVERITY_LOW :
    severityStr = ( char* )"Low";
    break;
  case GL_DEBUG_SEVERITY_NOTIFICATION :
    severityStr = ( char* )"Notification";
    break;
  default :
    severityStr = ( char* )"Undefined";
  }
  if ( length <= 0 ) { //just to use the length param
    message = "Error reporting the error!";
  }
  if ( userParam != nullptr ) { //just to use userParam 
    userParamStr = ( char* )userParam;
  } else {
    userParamStr = ( char* )"";
  }
  Logger::write( "OpenGL debug message "
	  "(Src: %s, Type: %s, Severity: %s, ID: %d, Extra: %s):"
	  "\n\t%s\n",
	  sourceStr, typeStr, severityStr, id, userParamStr, message );
}

FILE* Logger::log;

void Logger::initialize() {
  log = fopen( LOG_FILE_NAME, "a" );
  write( "Logging system initialized.\n" );
  //TODO assert failure??? (where to write message?)
}

void Logger::shutdown() {
  fclose( log );
  //TODO assert failure
}

void Logger::write( const char* const format, ... ) {
  //TODO measure performance
  va_list args1, args2;
  va_start( args1, format );
  va_copy( args2, args1 );
  vprintf( format, args1 );
  va_end( args1 );
  vfprintf( log, format, args2 );
  va_end( args2 );
  //TODO assert failure
}

Vec2 operator+( Vec2 a, Vec2 b ) {
  return { a.x + b.x, a.y + b.y };
}

Vec2 operator-( Vec2 a, Vec2 b ) {
  return { a.x - b.x, a.y - b.y };
}

Vec2 operator*( Vec2 a, Vec2 b ){
  return { a.x * b.x, a.y * b.y };
}

float dot( Vec2 a, Vec2 b ) {
  return a.x * b.x + a.y * b.y;
}

Vec2 rotate( Vec2 vec, float orientation ){
  float _cos = cos( orientation );
  float _sin = sin( orientation );
  return Vec2( vec.x * _cos - vec.y * _sin, vec.y * _cos + vec.x * _sin );
}

CircleColliderDebugRenderer::CircleColliderDebugRenderer() {
  //debug circle geometry
  GLfloat circlePositions[] = {
    //position		//radius	//color
    30.0f, 30.0f,	50.0f,		0.0f, 1.0f, 0.0f, 1.0f,
    45.0f, 30.0f,	20.0f,		0.0f, 1.0f, 0.0f, 0.7f };
  
  //configure buffers
  glGenVertexArrays( 1, &vaoId );
  glBindVertexArray( vaoId );
  glGenBuffers( 1, &vboId );
  glBindBuffer( GL_ARRAY_BUFFER, vboId );
  glBufferData( GL_ARRAY_BUFFER, sizeof( circlePositions ), circlePositions, GL_STATIC_DRAW );
  glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )0 );
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 1, 1, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )( 2 * sizeof( GLfloat ) ) );
  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )( 3 * sizeof( GLfloat ) ) );
  glEnableVertexAttribArray( 2 );
  glBindVertexArray( 0 );

  //create shader program
  int error = createShaderProgram( &shaderProgramId,
			       "shaders/DebugShape.vert", "shaders/DebugShape.frag",
			       "shaders/DebugShape.geom" );
  if ( error ) {
    //TODO implement error logging and maybe hardcode a default shader here
  }

  //get shader's constants' locations
  projLeftUnifLoc = glGetUniformLocation( shaderProgramId, "projection.left" );
  projRightUnifLoc = glGetUniformLocation( shaderProgramId, "projection.right" );
  projBottomUnifLoc = glGetUniformLocation( shaderProgramId, "projection.bottom" );
  projTopUnifLoc = glGetUniformLocation( shaderProgramId, "projection.top" );
  /*const int32_t
    cirlcesEntityScaleUnifLoc = glGetUniformLocation( circlesShaderProgramId, "entityScale" );*/
}

CircleColliderDebugRenderer::~CircleColliderDebugRenderer() {
  glDeleteProgram( shaderProgramId );
  glDeleteVertexArrays( 1, &vaoId );
  glDeleteBuffers( 1, &vboId );
}

void CircleColliderDebugRenderer::setOrthoProjection( float aspectRatio, float height ) {
  float halfHeight = height / 2.0f;
  glUniform1f( projLeftUnifLoc, -halfHeight * aspectRatio );
  glUniform1f( projRightUnifLoc, halfHeight * aspectRatio );
  glUniform1f( projBottomUnifLoc, -halfHeight );
  glUniform1f( projTopUnifLoc, halfHeight );
}

uint32_t EntityHandle::toInt() const {
  return generation << HANDLE_INDEX_BITS | index;
}

EntityHandle EntityManager::create() {
  uint32_t index;
  if ( freeIndices.size() < MIN_FREE_INDICES ) {
    generations.emplace_back( 0 );
    index = generations.size() - 1;
    //check if tried to create more entities than is possible
    //TODO use assert with message
    assert( index < MAX_ENTITIES );
  } else {
    index = freeIndices.front();
    freeIndices.pop_front();
  }
  return EntityHandle( index, generations[ index ].generation );
}

void EntityManager::destroy( EntityHandle entity ) {
  ++generations[ entity.index ].generation;
  freeIndices.emplace_back( ( uint32_t )entity.index );
}

bool EntityManager::isAlive( EntityHandle entity ) const {
  return generations[ entity.index ].generation == entity.generation;
}

void TransformManager::
set( EntityHandle entity, Vec2 position, Vec2 scale, float orientation ) {
  //TODO assert isalive( entity )
  transformComps.emplace_back( entity, position, scale, orientation );
  uint32_t compInd = transformComps.size() - 1;
  std::pair< uint32_t, ComponentIndex > mapArg( entity.toInt(), compInd );
  auto result = map.insert( mapArg );
  if ( result.second ) {
    return;
  } else {//if entity already has transform component
    //then the transform we just inserted is invalid
    transformComps.pop_back();
    //and we need to update the previous transfrom
    compInd = result.first->second;
    TransformComp transform = transformComps[ compInd ];
    transform.position = position;
    transform.scale = scale;
    transform.orientation = orientation;
  }
}

void TransformManager::remove( ComponentIndex compInd ) {
  //TODO assert compInd in range
  EntityHandle entity = transformComps[ compInd ].entity;
  //TODO measure which method is faster
  //transformComps.erase( transformComps.begin() + compInd );
  uint32_t lastCompInd = transformComps.size() - 1;
  EntityHandle lastEntity = transformComps[ lastCompInd ].entity;
  transformComps[ compInd ] = transformComps[ lastCompInd ];
  transformComps.pop_back();
  auto iterator = map.find( lastEntity.toInt() );
  iterator->second = compInd;
  map.erase( entity.toInt() );
}

bool TransformManager::has( EntityHandle entity ) const {
  //TODO assert isalive( entity )
  auto iterator = map.find( entity.toInt() );
  if ( iterator != map.end() ) {
    return true;
  }
  return false;
}

ComponentIndex TransformManager::getTempComponentIndex( EntityHandle entity ) const {
  //TODO assert isalive( entity )
  auto iterator = map.find( entity.toInt() );
  if ( iterator == map.end() ) {
    //TODO make this into an assert
  }
  return iterator->second;
}

void CircleColliderManager::add( EntityHandle entity, Vec2 center, float radius ) {
  circleColliderComps.emplace_back( entity, center, radius );
  uint32_t compInd = circleColliderComps.size() - 1;
  map.emplace( entity.toInt(), compInd );
}

void CircleColliderManager::remove( ComponentIndex compInd ) {
  //TODO assert compInd in range
  //swap element to remove with last element and remove last element
  EntityHandle entity = circleColliderComps[ compInd ].entity;
  uint32_t lastCompInd = circleColliderComps.size() - 1;
  EntityHandle lastEntity = circleColliderComps[ lastCompInd ].entity;
  circleColliderComps[ compInd ] = circleColliderComps[ lastCompInd ];
  //update index of swapped element in map
  auto range = map.equal_range( lastEntity.toInt() );
  for ( auto iterator = range.first; iterator != range.second; ++iterator ) {
    if ( iterator->second == lastCompInd ) {
      iterator->second = compInd;
      break;
    }
  }
  //remove element in map
  range = map.equal_range( entity.toInt() );
  for ( auto iterator = range.first; iterator != range.second; ++iterator ) {
    if ( iterator->second == compInd ) {
      map.erase( iterator );
      break;
    }
  }
}

uint32_t CircleColliderManager::count( EntityHandle entity ) const {
  return map.count( entity.toInt() );
}

std::vector< ComponentIndex >
CircleColliderManager::getTempComponentIndices( EntityHandle entity ) const {
  //TODO assert count > 0
  std::vector< ComponentIndex > indices;
  auto range = map.equal_range( entity.toInt() );
  for ( auto iterator = range.first; iterator != range.second; ++iterator ) {
    indices.push_back( iterator->second );
  }
  return indices;
}

TextureRsrc loadTexture( const char* name ) {
  uint32_t glId;
  glGenTextures( 1, &glId );
  glBindTexture( GL_TEXTURE_2D, glId );
  uint32_t width, height;
  unsigned char* texData = SOIL_load_image( name, ( int* )&width, ( int* )&height, nullptr, SOIL_LOAD_RGBA );
  if ( texData == 0 ) {
    //TODO make into assertion
    Logger::write( "Error loading texture %s: %s\n", name, SOIL_last_result() );
  }
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData );
  SOIL_free_image_data( texData );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  return { name, width, height, glId };
}

void deleteTexture( TextureRsrc texture ) {
  glDeleteTextures( 1, &texture.glId );
}

void SpriteManager::initialize() {
  //configure buffers
  glGenVertexArrays( 1, &renderInfo.vaoId );
  glBindVertexArray( renderInfo.vaoId );
  //glBufferData is different for every sprite
  glGenBuffers( 2, renderInfo.vboIds );
  //positions buffer
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, ( void* )0 );
  glEnableVertexAttribArray( 0 );
  //texture coordinates buffer
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 1 ] );
  glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, ( void* )0 );
  glEnableVertexAttribArray( 1 );
  glBindVertexArray( 0 );
  //create shader program
  int error = createShaderProgram( &renderInfo.shaderProgramId,
				   "shaders/SpriteUnlit.vert", "shaders/SpriteUnlit.frag" );
  if ( error ) {
    //TODO implement error logging and maybe hardcode a default shader here
  }  
  //get shader's constants' locations
  renderInfo.projUnifLoc[ 0 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.left" );
  renderInfo.projUnifLoc[ 1 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.right" );
  renderInfo.projUnifLoc[ 2 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.bottom" );
  renderInfo.projUnifLoc[ 3 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.top" );
}

void SpriteManager::shutdown() {
  glDeleteProgram( renderInfo.shaderProgramId );
  glDeleteVertexArrays( 1, &renderInfo.vaoId );
  glDeleteBuffers( 2, renderInfo.vboIds );
}

void SpriteManager::setOrthoProjection( float aspectRatio, float height ) {
  float halfHeight = height / 2.0f;
  glUseProgram( renderInfo.shaderProgramId );
  glUniform1f( renderInfo.projUnifLoc[ 0 ], -halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 1 ], halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 2 ], -halfHeight );
  glUniform1f( renderInfo.projUnifLoc[ 3 ], halfHeight );
}

void SpriteManager::updateAndRender( const TransformManager& transformManager ) {
  std::vector< TransformComp > updatedTransforms = transformManager.getLastUpdated();
  //update local transform cache
  std::vector< EntityHandle > updatedEntities( updatedTransforms.size() );
  for ( uint32_t trInd = 0; trInd < updatedTransforms.size(); ++trInd ) {
    updatedEntities.push_back( updatedTransforms[ trInd ].entity );
  }
  std::vector< LookupResult > updatedSprites = lookup( updatedEntities );
  for ( uint32_t trInd = 0; trInd < updatedTransforms.size(); ++trInd ) {
    if ( updatedSprites[ trInd ].found ) {
      TransformComp transformComp = updatedTransforms[ trInd ];
      ComponentIndex spriteCompInd = updatedSprites[ trInd ].index;
      spriteComps[ spriteCompInd ].position = transformComp.position;
      spriteComps[ spriteCompInd ].scale = transformComp.scale;
      spriteComps[ spriteCompInd ].orientation = transformComp.orientation;
    }
  }
  //build vertex buffer and render for sprites with same texture    
  glUseProgram( renderInfo.shaderProgramId );
  glBindVertexArray( renderInfo.vaoId );
  //we render every sprite every time
  uint32_t spritesToRenderCount = spriteComps.size();
  posBufferData = new Pos[ spritesToRenderCount * 4 ]; //4 verts per sprite
  texCoordsBufferData = new UV[ spritesToRenderCount * 4 ];
  uint32_t currentTexGlId = spriteComps[ 0 ].baseTexture.glId;
  glBindTexture( GL_TEXTURE_2D, currentTexGlId );
  //mark where a sub-buffer with sprites sharing a texture ends and a new one begins
  uint32_t currentSubBufferStart = 0;
  Vec2 baseGeometry[ 4 ] = {
    { -0.5f,	-0.5f },
    { 0.5f,	-0.5f },
    { 0.5f,	 0.5f },
    { -0.5f,	 0.5f } };
  for ( uint32_t spriteInd = 0; spriteInd < spriteComps.size(); ++spriteInd ) {
    //add to the vertex buffer
    SpriteComp spriteComp = spriteComps[ spriteInd ];
    Vec2 texCoords[ 4 ] = {
      spriteComp.texCoords.min,
      { spriteComp.texCoords.max.u, spriteComp.texCoords.min.v },
      spriteComp.texCoords.max,
      { spriteComp.texCoords.min.u, spriteComp.texCoords.max.v } };
    for ( int vertInd = 0; vertInd < 4; ++vertInd ) {
      Vec2 vert = rotate( baseGeometry[ vertInd ], spriteComp.orientation );
      vert *= spriteComp.size * spriteComp.scale;
      vert += spriteComp.position;
      posBufferData[ spriteInd * 4 + vertInd ].pos = vert;
      texCoordsBufferData[ spriteInd * 4 + vertInd ].uv = texCoords[ vertInd ];
    }
    if ( spriteComps[ spriteInd ].baseTexture.glId != currentTexGlId ) {
      //send current vertex sub-buffer and render it
      uint32_t subBufferSize = sizeof( Vec2 ) * 4 * ( spriteInd - currentSubBufferStart );
      glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
      glBufferData( GL_ARRAY_BUFFER, subBufferSize, posBufferData + currentSubBufferStart, GL_STATIC_DRAW );
      glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 1 ] );
      glBufferData( GL_ARRAY_BUFFER, subBufferSize, texCoordsBufferData + currentSubBufferStart, GL_STATIC_DRAW );
      glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
      //and start a new one
      currentTexGlId = spriteComps[ spriteInd ].baseTexture.glId;
      glBindTexture( GL_TEXTURE_2D, currentTexGlId );
      currentSubBufferStart = spriteInd;
    }
  }
  //TODO measure how expensive these allocations and deallocations are!
  delete[] posBufferData;
  delete[] texCoordsBufferData;
}
