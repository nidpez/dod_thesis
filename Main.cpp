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
};

Vec2 operator+( Vec2 a, Vec2 b );

Vec2& operator+=( Vec2& a, Vec2 b );

Vec2 operator-( Vec2 a, Vec2 b );

Vec2& operator-=( Vec2& a, Vec2 b );

Vec2 operator*( Vec2 a, Vec2 b );

Vec2& operator*=( Vec2& a, Vec2 b );

float dot( Vec2 a, Vec2 b );

Vec2 rotate( Vec2 vec, float orientation );

//resources
typedef uint32_t TextureHandle;

struct TextureAsset {
  const char* name;
  const uint32_t width, height;
  const uint32_t glId;
};

class AssetManager {
  static std::vector< TextureAsset > textures;
public:
  static void initialize();
  static void shutdown();
  static TextureHandle loadTexture( const char* name );
  static void destroyTexture( TextureHandle texture );
  static bool isTextureAlive( TextureHandle texture );
  static TextureAsset getTexture( TextureHandle texture );
};
  
//entity and component managers

/*Entity Handle system based on http://bitsquid.blogspot.com.co/2014/08/building-data-oriented-entity-system.html and http://gamesfromwithin.com/managing-data-relationships*/
const int HANDLE_INDEX_BITS = 21;
const int HANDLE_GENERATION_BITS = 32 - HANDLE_INDEX_BITS;
//With 21 index bits 2 million entities are possible at a time.
const int MAX_ENTITIES = 1 << HANDLE_INDEX_BITS;

struct EntityHandle {
  uint32_t index : HANDLE_INDEX_BITS;
  uint32_t generation : HANDLE_GENERATION_BITS;
};

uint32_t entityToInt( EntityHandle entity );

class EntityManager {
  struct Generation { //can't just use uint32_t since they overflow at different values
    uint32_t generation : HANDLE_GENERATION_BITS;
  };
  const static uint32_t MIN_FREE_INDICES = 1024;
  static std::vector< Generation > generations;
  static std::deque< uint32_t > freeIndices;
public:
  static void initialize();
  static void shutdown();
  static EntityHandle create();
  static void destroy( EntityHandle entity );
  static bool isAlive( EntityHandle entity );
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
};

class TransformManager {
  static std::vector< TransformComp > transformComps;
  static std::unordered_map< uint32_t, ComponentIndex > map;
  static ComponentIndex lookup( EntityHandle entity );
public:
  static void initialize();
  static void shutdown();
  static void set( EntityHandle entity, Vec2 position, Vec2 scale, float orientation );
  static void remove( EntityHandle entity );
  static bool has( EntityHandle entity );
  static std::vector< TransformComp > getLastUpdated();
};

//TODO allow multiple colliders per entity (with linked list?)
class CircleColliderManager {
  struct CircleColliderComp {
    EntityHandle entity;
    Vec2 center;
    float radius;
    //transform cache
    Vec2 position;
    Vec2 scale;
  };
  static std::vector< CircleColliderComp > circleColliderComps;
  static std::unordered_map< uint32_t, ComponentIndex > map;
  static std::vector< LookupResult > lookup( std::vector< EntityHandle > entities );
public:
  static void initialize();
  static void shutdown();
  static void add( EntityHandle entity, Vec2 center, float radius );
  static void remove( EntityHandle entity );
  static bool has( EntityHandle entity );
  static void updateAndCollide();
};

//Axis aligned bounding box 
struct Rect {
  Vec2 min;
  Vec2 max;
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
    TextureHandle textureId;
    Rect texCoords;
    Vec2 size;
    //transform cache
    Vec2 position;
    Vec2 scale;
    float orientation;
  };
  static std::vector< SpriteComp > spriteComps;
  static std::unordered_map< uint32_t, ComponentIndex > map;
  //rendering data
  struct Pos {
    Vec2 pos;
  };
  struct UV {
    Vec2 uv;
  };
  static RenderInfo renderInfo;
  //TODO merge into single vertex attrib pointer
  static Pos* posBufferData;
  static UV* texCoordsBufferData;
  static std::vector< LookupResult > lookup( std::vector< EntityHandle > entities );
public:
  static void initialize();
  static void shutdown();
  static void set( EntityHandle entity, TextureHandle textureId, Rect texCoords );
  static void remove( EntityHandle entity );
  static bool has( EntityHandle entity );
  static void updateAndRender();
  static void setOrthoProjection( float aspectRatio, float height );
};

typedef uint32_t AnimationHandle;
//so we are able to later redefine AnimationFrame as something more robust
//that can handle more than uv flipbook animation
typedef Rect AnimationFrame; 

class AnimationManager {
  static std::vector< AnimationFrame > animations;
public:
  static void initialize();
  static void shutdown();
  static AnimationHandle add( EntityHandle entity, const AnimationFrame* frames, float fps, bool loops, bool autoplay );
  static void play( AnimationHandle animation );
};

struct DebugCircle {
  Vec2 position;
  float radius;
  float color[ 4 ];
};

class DebugRenderer {
  static RenderInfo renderInfo;
  static std::vector< DebugCircle > circleBufferData;
public:
  static void initialize();
  static void shutdown();
  static void addCircles( std::vector< DebugCircle > circles );
  static void renderAndClear();
  static void setOrthoProjection( float aspectRatio, float height );
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
  static void write( const char* format, ... );
  static void write( const char* format, va_list args );
};

void haltWithMessage( const char* failedCond, const char* file, int line, ... );

#ifdef NDEBUG
#define ASSERT( condition, ... ) ( ( void )0 )
#else
#define ASSERT( condition, ... ) {					\
    if ( !( condition ) ) {						\
      haltWithMessage( #condition, __FILE__, __LINE__, __VA_ARGS__ );	\
    }									\
  }
#endif

//misc
GLFWwindow* createWindowAndGlContext( const char* const windowTitle );

int main() {
  //initialize managers
  Logger::initialize();
  GLFWwindow* window = createWindowAndGlContext( "Space Adventure (working title)" );
  EntityManager::initialize();
  TransformManager::initialize();
  CircleColliderManager::initialize();
  SpriteManager::initialize();
  //AnimationManager animationManager;
  AssetManager::initialize();
  DebugRenderer::initialize();

  //configure viewport and orthographic projection
  //TODO put projection info in a Camera component
  int windowWidth, windowHeight;
  glfwGetWindowSize( window, &windowWidth, &windowHeight );
  glViewport( 0, 0, windowWidth, windowHeight );
  float aspect = windowWidth / ( float )windowHeight;

  SpriteManager::setOrthoProjection( aspect, 100 );
  DebugRenderer::setOrthoProjection( aspect, 100 );
  
  //load textures
  TextureHandle astronautTex = AssetManager::loadTexture( "astronaut.png" );
  TextureHandle planetTex = AssetManager::loadTexture( "planetSurface.png" );
  
  //test astronaut sprite entity
  // //bounding geometry
  // float r1 = BASE_SCALE.x / 2.0f;
  EntityHandle astronautId = EntityManager::create();
  TransformManager::set( astronautId, { -30.0f, 30.0f }, { 1.0f, 1.0f }, 0.0f );
  CircleColliderManager::add( astronautId, { 0.0f, 0.0f }, 1.0f );
  AnimationFrame runFrames[] = {
    { { 0.0f, 0.0f },		{ 1.0f / 5.0f, 1.0f } },
    { { 1.0f / 5.0f, 0.0f },	{ 2.0f / 5.0f, 1.0f } },
    { { 2.0f / 5.0f, 0.0f },	{ 3.0f / 5.0f, 1.0f } },
    { { 3.0f / 5.0f, 0.0f },	{ 4.0f / 5.0f, 1.0f } } };
  //AnimationFrame jumpFrames[] = { { { 4.0f / 5.0f, 0.0f }, { 1.0f, 1.0f } } };
  SpriteManager::set( astronautId, astronautTex, ( Rect )runFrames[ 0 ] );
  //example usage code
  // AnimationHandle astronautRunAnimId = animationManager::add( astronautId, runFrames, 24, true, false );
  // AnimationHandle astronautJumpAnimId = animationManager::add( astronautId, jumpFrames, 24, false, false );
  
  //test planet sprite entity
  EntityHandle planetId = EntityManager::create();
  TransformManager::set( planetId, { 0.0f, 15.0f }, { 1.5f, 1.0f }, 4.0f );
  CircleColliderManager::add( planetId, { 0.0f, 0.0f }, 1.0f );
  Rect planetTexCoords = { { 0.0f, 0.0f }, {1.0f, 1.0f } };
  SpriteManager::set( planetId, planetTex, planetTexCoords );
  
  EntityHandle planet2Id = EntityManager::create();
  TransformManager::set( planet2Id, { 30.0f, -30.0f }, { 1.0f, 1.0f }, 0.0f );
  SpriteManager::set( planet2Id, planetTex, planetTexCoords );
  
  //main loop      
  // float tempPos[ 2 ];
  // tempPos[ 0 ] = pos1.x;
  // tempPos[ 1 ] = pos1.y;
  // float angle2 = 0.0f;
  double t1 = glfwGetTime();
  double t2;
  double deltaT = 0.0;
  Logger::write( "About to enter main loop.\n" );
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
    CircleColliderManager::updateAndCollide();
    
    //render scene
    glClear( GL_COLOR_BUFFER_BIT );
    SpriteManager::updateAndRender();
  
    // //render debug shapes
    DebugRenderer::renderAndClear();
    
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
  AssetManager::destroyTexture( astronautTex );
  AssetManager::destroyTexture( planetTex );
  Logger::write( "Resources freed.\n" );

  glfwDestroyWindow( window );
  glfwTerminate();

  //shut down managers
  AssetManager::shutdown();
  SpriteManager::shutdown();
  CircleColliderManager::shutdown();
  TransformManager::shutdown();
  EntityManager::shutdown();
  Logger::shutdown();
  
  return 0;
}

GLFWwindow* createWindowAndGlContext( const char* const windowTitle ) {  
  glfwSetErrorCallback( printGlfwError );  
  ASSERT( glfwInit(), "GLFW failed to initialize" );  
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
  ASSERT( window, "Error creating the window" );  
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
  ASSERT( glewError == GLEW_OK, "GLEW failed to initialize: %s", glewGetErrorString( glewError ) );  
  //is this even possible when glfw already gave us a 3.3 context?
  ASSERT( GLEW_VERSION_3_3, "Required OpenGL version 3.3 unavailable, says GLEW" );    
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
    return;
    // severityStr = ( char* )"Notification";
    // break;
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

void Logger::write( const char* format, ... ) {
  va_list args;
  va_start( args, format );
  write( format, args );
  va_end( args );
}

void Logger::write( const char* format, va_list args ) {
  //TODO should we open and close the log file here?
  //TODO measure performance
  va_list args2;
  va_copy( args2, args );
  vprintf( format, args );
  vfprintf( log, format, args2 );
  va_end( args2 );
  //TODO assert failure
}
  
void haltWithMessage( const char* failedCond, const char* file, int line, ... ) {
  Logger::write( "Assertion %s failed at %s, %s, line %d: ",
		 failedCond, file, __func__, line ); 
  va_list args;
  va_start( args, line );
  char* msgFormat = va_arg( args, char* );
  Logger::write( msgFormat, args );
  va_end( args );
  Logger::write( "\n" );
  Logger::shutdown();
  std::abort();
}

Vec2 operator+( Vec2 a, Vec2 b ) {
  return { a.x + b.x, a.y + b.y };
}

Vec2& operator+=( Vec2& a, Vec2 b ) {
  a = a + b;
  return a;
}

Vec2 operator-( Vec2 a, Vec2 b ) {
  return { a.x - b.x, a.y - b.y };
}

Vec2& operator-=( Vec2& a, Vec2 b ) {
  a = a - b;
  return a;
}

Vec2 operator*( Vec2 a, Vec2 b ){
  return { a.x * b.x, a.y * b.y };
}

Vec2& operator*=( Vec2& a, Vec2 b ) {
  a = a * b;
  return a;
}

float dot( Vec2 a, Vec2 b ) {
  return a.x * b.x + a.y * b.y;
}

Vec2 rotate( Vec2 vec, float orientation ){
  float _cos = cos( orientation );
  float _sin = sin( orientation );
  return { vec.x * _cos - vec.y * _sin, vec.y * _cos + vec.x * _sin };
}

RenderInfo DebugRenderer::renderInfo;
std::vector< DebugCircle > DebugRenderer::circleBufferData;

void DebugRenderer::initialize() {
#ifndef NDEBUG
  //configure buffers
  glGenVertexArrays( 1, &renderInfo.vaoId );
  glBindVertexArray( renderInfo.vaoId );
  glGenBuffers( 1, &renderInfo.vboIds[ 0 ] );
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )0 );
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 1, 1, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )( 2 * sizeof( GLfloat ) ) );
  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 2, 4, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )( 3 * sizeof( GLfloat ) ) );
  glEnableVertexAttribArray( 2 );
  glBindVertexArray( 0 );
  //create shader program
  int error = createShaderProgram( &renderInfo.shaderProgramId,
			       "shaders/DebugShape.vert", "shaders/DebugShape.frag",
			       "shaders/DebugShape.geom" );
  if ( error ) {
    //TODO maybe hardcode a default shader here
  }
  //get shader's constants' locations
  renderInfo.projUnifLoc[ 0 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.left" );
  renderInfo.projUnifLoc[ 1 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.right" );
  renderInfo.projUnifLoc[ 2 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.bottom" );
  renderInfo.projUnifLoc[ 3 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.top" );
#endif
}

void DebugRenderer::shutdown() {
#ifndef NDEBUG
  glDeleteProgram( renderInfo.shaderProgramId );
  glDeleteVertexArrays( 1, &renderInfo.vaoId );
  glDeleteBuffers( 1, &renderInfo.vboIds[ 0 ] );
#endif
}

void DebugRenderer::addCircles( std::vector< DebugCircle > circles ) {
#ifndef NDEBUG
  for ( uint32_t i = 0; i < circles.size(); ++i ) {
    float radius = circles[ i ].radius; 
    ASSERT( radius > 0.0f, "Asked to draw a circle of radius %f", radius );
  }
  circleBufferData.insert( circleBufferData.end(), circles.begin(), circles.end() );
#endif
}

void DebugRenderer::renderAndClear() {
#ifndef NDEBUG
  //configure buffers
  glUseProgram( renderInfo.shaderProgramId );
  glBindVertexArray( renderInfo.vaoId );
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glBufferData( GL_ARRAY_BUFFER, sizeof( DebugCircle ) * circleBufferData.size(), circleBufferData.data(), GL_STATIC_DRAW );
  glDrawArrays( GL_POINTS, 0, circleBufferData.size() );
  circleBufferData.clear();
#endif
}

void DebugRenderer::setOrthoProjection( float aspectRatio, float height ) {
#ifndef NDEBUG
  float halfHeight = height / 2.0f;
  glUseProgram( renderInfo.shaderProgramId );
  glUniform1f( renderInfo.projUnifLoc[ 0 ], -halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 1 ], halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 2 ], -halfHeight );
  glUniform1f( renderInfo.projUnifLoc[ 3 ], halfHeight );
#endif
}

std::vector< EntityManager::Generation > EntityManager::generations;
std::deque< uint32_t > EntityManager::freeIndices;

uint32_t entityToInt( EntityHandle entity ) {
  return entity.generation << HANDLE_INDEX_BITS | entity.index;
}

void EntityManager::initialize() {
  generations = std::vector< Generation >();
  freeIndices = std::deque< uint32_t >();
}

void EntityManager::shutdown() {
}

EntityHandle EntityManager::create() {
  uint32_t index;
  if ( freeIndices.size() < MIN_FREE_INDICES ) {
    generations.push_back( { 0 } );
    index = generations.size() - 1;    
    ASSERT( index < MAX_ENTITIES, "Tried to create more than %d entities", MAX_ENTITIES ); 
  } else {
    index = freeIndices.front();
    freeIndices.pop_front();
  }
  EntityHandle newEntity = { index, generations[ index ].generation };
  Logger::write( "Entity %d created\n", newEntity );
  return newEntity;
}

bool EntityManager::isAlive( EntityHandle entity ) {
  return generations[ entity.index ].generation == entity.generation;
}

std::vector< TransformComp > TransformManager::transformComps;
std::unordered_map< uint32_t, ComponentIndex > TransformManager::map;

void TransformManager::initialize() {
  transformComps = std::vector< TransformComp >();
  map = std::unordered_map< uint32_t, ComponentIndex >();
}

void TransformManager::shutdown() {
}

void TransformManager::set( EntityHandle entity, Vec2 position, Vec2 scale, float orientation ) {
  ASSERT( EntityManager::isAlive( entity ), "Invalid entity id %d", entityToInt( entity ) );  
  transformComps.push_back( { entity, position, scale, orientation } );
  uint32_t compInd = transformComps.size() - 1;
  bool inserted = map.insert( { entityToInt( entity ), compInd } ).second;  
  ASSERT( inserted, "Could not map entity %d to component index %d", entityToInt( entity ), compInd );
  Logger::write( "Transform component added to entity %d\n", entityToInt( entity ) );
}

bool TransformManager::has( EntityHandle entity ) {
  ASSERT( EntityManager::isAlive( entity ), "Invalid entity id %d", entityToInt( entity ) );  
  auto iterator = map.find( entityToInt( entity ) );
  if ( iterator != map.end() ) {
    return true;
  }
  return false;
}

ComponentIndex TransformManager::lookup( EntityHandle entity ) {
  ASSERT( EntityManager::isAlive( entity ), "Invalid entity id %d", entityToInt( entity ) );  
  auto iterator = map.find( entityToInt( entity ) ); 
  ASSERT( iterator != map.end(), "Entity %d has no Transform component", entityToInt( entity ) );	  
  return iterator->second;
}

std::vector< TransformComp > TransformManager::getLastUpdated() {
  //TODO actually compute which transforms have been updated since last frame
  return transformComps;
}

std::vector< CircleColliderManager::CircleColliderComp > CircleColliderManager::circleColliderComps;
std::unordered_map< uint32_t, ComponentIndex > CircleColliderManager::map;

void CircleColliderManager::add( EntityHandle entity, Vec2 center, float radius ) {
  ASSERT( EntityManager::isAlive( entity ), "Invalid entity id %d", entityToInt( entity ) );  
  circleColliderComps.push_back( { entity, center, radius, { 0, 0 }, { 0, 0 } } );
  uint32_t compInd = circleColliderComps.size() - 1;
  map.emplace( entityToInt( entity ), compInd );
  Logger::write( "CircleCollider component added to entity %d\n", entityToInt( entity ) );
}

bool CircleColliderManager::has( EntityHandle entity ) {
  ASSERT( EntityManager::isAlive( entity ), "Invalid entity id %d", entityToInt( entity ) );  
  auto iterator = map.find( entityToInt( entity ) );
  if ( iterator != map.end() ) {
    return true;
  }
  return false;
}

void CircleColliderManager::initialize() {
  circleColliderComps = std::vector< CircleColliderComp >();
  map = std::unordered_map< uint32_t, ComponentIndex >();
}

void CircleColliderManager::shutdown() {
}

std::vector< LookupResult > CircleColliderManager::lookup( std::vector< EntityHandle > entities ) {
  //TODO refactor this duplicated code
  std::vector< LookupResult > result( entities.size() );
  for ( uint32_t entityInd = 0; entityInd < entities.size(); ++entityInd ) {
    auto iterator = map.find( entityToInt( entities[ entityInd ] ) );
    if ( iterator != map.end() ) {
      result[ entityInd ] = { iterator->second, true };
    } else {
      result[ entityInd ].found = false;
    }
  }
  return result;
}

void CircleColliderManager::updateAndCollide() {
  if ( circleColliderComps.size() == 0 ) {
    return;
  }
  std::vector< TransformComp > updatedTransforms = TransformManager::getLastUpdated();
  //update local transform cache
  std::vector< EntityHandle > updatedEntities;
  updatedEntities.reserve( updatedTransforms.size() );
  for ( uint32_t trInd = 0; trInd < updatedTransforms.size(); ++trInd ) {
    updatedEntities.push_back( updatedTransforms[ trInd ].entity );
  }
  std::vector< LookupResult > updatedCircleColliders = lookup( updatedEntities );
  for ( uint32_t trInd = 0; trInd < updatedTransforms.size(); ++trInd ) {
    if ( updatedCircleColliders[ trInd ].found ) {
      TransformComp transformComp = updatedTransforms[ trInd ];
      ComponentIndex circleColliderCompInd = updatedCircleColliders[ trInd ].index;
      circleColliderComps[ circleColliderCompInd ].position = transformComp.position;
      circleColliderComps[ circleColliderCompInd ].scale = transformComp.scale;
    }
  }
  //TODO do frustrum culling
  std::vector< DebugCircle > circles;
  circles.reserve( circleColliderComps.size() );
  for ( uint32_t colInd = 0; colInd < circleColliderComps.size(); ++colInd ) {
    CircleColliderComp circleColliderComp = circleColliderComps[ colInd ];
    DebugCircle circle;
    circle.position = circleColliderComp.position + circleColliderComp.center;
    float scaleX = circleColliderComp.scale.x, scaleY = circleColliderComp.scale.y;
    circle.radius = circleColliderComp.radius * ( scaleX > scaleY ) ? scaleX : scaleY;
    circle.color[ 0 ] =  0.0f;
    circle.color[ 1 ] =  1.0f;
    circle.color[ 2 ] =  0.0f;
    circle.color[ 3 ] =  1.0f;
    circles.push_back( circle );
  }
  DebugRenderer::addCircles( circles );
}

std::vector< TextureAsset > AssetManager::textures;

void AssetManager::initialize() {
  textures = std::vector< TextureAsset >();
}

void AssetManager::shutdown() {
}
 
TextureHandle AssetManager::loadTexture( const char* name ) {
  uint32_t glId;
  glGenTextures( 1, &glId );
  glBindTexture( GL_TEXTURE_2D, glId );
  uint32_t width, height;
  unsigned char* texData = SOIL_load_image( name, ( int* )&width, ( int* )&height, nullptr, SOIL_LOAD_RGBA );  
  ASSERT( texData != 0, "Error loading texture %s: %s", name, SOIL_last_result() );  
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData );
  SOIL_free_image_data( texData );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  textures.push_back( { name, width, height, glId } );
  Logger::write( "Texture '%s' successfully loaded (glId = %d)\n", name, glId );
  return textures.size() - 1;
}

void AssetManager::destroyTexture( TextureHandle texture ) {
  ASSERT( isTextureAlive( texture ), "Invalid texture id %d", texture );  
  glDeleteTextures( 1, &textures[ texture ].glId );
  std::memset(  &textures[ texture ], 0, sizeof( TextureAsset ) );
}

bool AssetManager::isTextureAlive( TextureHandle texture ) {
  return texture < textures.size() && textures[ texture ].glId > 0;
}

TextureAsset AssetManager::getTexture( TextureHandle texture ) {
  ASSERT( isTextureAlive( texture ), "Invalid texture id %d", texture );  
  return textures[ texture ];
}

std::vector< SpriteManager::SpriteComp > SpriteManager::spriteComps;
std::unordered_map< uint32_t, ComponentIndex > SpriteManager::map;
RenderInfo SpriteManager::renderInfo;
SpriteManager::Pos* SpriteManager::posBufferData;
SpriteManager::UV* SpriteManager::texCoordsBufferData;

void SpriteManager::initialize() {
  //configure buffers
  glGenVertexArrays( 1, &renderInfo.vaoId );
  glBindVertexArray( renderInfo.vaoId );
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
    //TODO maybe hardcode a default shader here
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

void SpriteManager::set( EntityHandle entity, TextureHandle textureId, Rect texCoords ) {
  ASSERT( EntityManager::isAlive( entity ), "Invalid entity id %d", entityToInt( entity ) );
  ASSERT( AssetManager::isTextureAlive( textureId ), "Invalid texture id %d", textureId ); 
  SpriteComp spriteComp = {};
  spriteComp.entity = entity;
  spriteComp.textureId = textureId;
  spriteComp.texCoords = texCoords;
  TextureAsset texture = AssetManager::getTexture( textureId );
  float width = texture.width * ( texCoords.max.u - texCoords.min.u ) / PIXELS_PER_UNIT;
  float height = texture.height * ( texCoords.max.v - texCoords.min.v ) / PIXELS_PER_UNIT;
  spriteComp.size = { width, height };
  spriteComps.push_back( spriteComp );
  uint32_t compInd = spriteComps.size() - 1;
  bool inserted = map.insert( { entityToInt( entity ), compInd } ).second;  
  ASSERT( inserted, "Could not map entity %d to component index %d", entityToInt( entity ), compInd );
  Logger::write( "Sprite component added to entity %d\n", entityToInt( entity ) );
}

void SpriteManager::setOrthoProjection( float aspectRatio, float height ) {
  float halfHeight = height / 2.0f;
  glUseProgram( renderInfo.shaderProgramId );
  glUniform1f( renderInfo.projUnifLoc[ 0 ], -halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 1 ], halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 2 ], -halfHeight );
  glUniform1f( renderInfo.projUnifLoc[ 3 ], halfHeight );
}

std::vector< LookupResult > SpriteManager::lookup( std::vector< EntityHandle > entities ) {
  //TODO refactor this duplicated code
  std::vector< LookupResult > result( entities.size() );
  for ( uint32_t entityInd = 0; entityInd < entities.size(); ++entityInd ) {
    auto iterator = map.find( entityToInt( entities[ entityInd ] ) );
    if ( iterator != map.end() ) {
      result[ entityInd ] = { iterator->second, true };
    } else {
      result[ entityInd ].found = false;
    }
  }
  return result;
}

void SpriteManager::updateAndRender() {
  if ( spriteComps.size() == 0 ) {
    return;
  }
  std::vector< TransformComp > updatedTransforms = TransformManager::getLastUpdated();
  //update local transform cache
  std::vector< EntityHandle > updatedEntities;
  updatedEntities.reserve( updatedTransforms.size() );
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
  //TODO don't render every sprite every time
  uint32_t spritesToRenderCount = spriteComps.size();
  //TODO use triangle indices to reduce vertex count
  uint32_t vertsPerSprite = 6; 
  posBufferData = new Pos[ spritesToRenderCount * vertsPerSprite ];
  texCoordsBufferData = new UV[ spritesToRenderCount * vertsPerSprite ];
  //build the positions buffer
  Vec2 baseGeometry[] = {
    { -0.5f,	-0.5f },
    { 0.5f,	 0.5f },
    { -0.5f,	 0.5f },
    { -0.5f,	-0.5f },
    { 0.5f,	-0.5f },
    { 0.5f,	 0.5f }
  };
  for ( uint32_t spriteInd = 0; spriteInd < spritesToRenderCount; ++spriteInd ) {
    SpriteComp spriteComp = spriteComps[ spriteInd ];
    for ( uint32_t vertInd = 0; vertInd < vertsPerSprite; ++vertInd ) {
      Vec2 vert = rotate( baseGeometry[ vertInd ], spriteComp.orientation );
      vert *= spriteComp.size * spriteComp.scale;
      vert += spriteComp.position;
      posBufferData[ spriteInd * vertsPerSprite + vertInd ].pos = vert;
    }
  } 
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glBufferData( GL_ARRAY_BUFFER, spritesToRenderCount * vertsPerSprite * sizeof( Pos ), posBufferData, GL_STATIC_DRAW );
  //TODO measure how expensive these allocations and deallocations are!
  delete[] posBufferData;
  //build the texture coordinates buffer
  for ( uint32_t spriteInd = 0; spriteInd < spritesToRenderCount; ++spriteInd ) {
    SpriteComp spriteComp = spriteComps[ spriteInd ];
    Vec2 texCoords[] = {
      spriteComp.texCoords.min,
      spriteComp.texCoords.max,
      { spriteComp.texCoords.min.u, spriteComp.texCoords.max.v },
      spriteComp.texCoords.min,
      { spriteComp.texCoords.max.u, spriteComp.texCoords.min.v },
      spriteComp.texCoords.max
    };
    for ( uint32_t vertInd = 0; vertInd < vertsPerSprite; ++vertInd ) {
      texCoordsBufferData[ spriteInd * vertsPerSprite + vertInd ].uv = texCoords[ vertInd ];
    }
  }
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 1 ] );
  glBufferData( GL_ARRAY_BUFFER, spritesToRenderCount * vertsPerSprite * sizeof( UV ), texCoordsBufferData, GL_STATIC_DRAW );
  //TODO measure how expensive these allocations and deallocations are!
  delete[] texCoordsBufferData;
  //issue render commands
  //TODO maybe sort by texture id
  uint32_t currentTexId = spriteComps[ 0 ].textureId;  
  ASSERT( AssetManager::isTextureAlive( currentTexId ), "Invalid texture id %d", currentTexId );  
  uint32_t currentTexGlId = AssetManager::getTexture( currentTexId ).glId;
  glBindTexture( GL_TEXTURE_2D, currentTexGlId );
  //mark where a sub-buffer with sprites sharing a texture ends and a new one begins
  uint32_t currentSubBufferStart = 0;
  for ( uint32_t spriteInd = 1; spriteInd < spritesToRenderCount; ++spriteInd ) {
    if ( spriteComps[ spriteInd ].textureId != currentTexId ) {
      //send current vertex sub-buffer and render it
      uint32_t spriteCountInSubBuffer = spriteInd - currentSubBufferStart;
      glDrawArrays( GL_TRIANGLES, vertsPerSprite * currentSubBufferStart, vertsPerSprite * spriteCountInSubBuffer );
      //and start a new one
      currentTexId = spriteComps[ spriteInd ].textureId;      
      ASSERT( AssetManager::isTextureAlive( currentTexId ), "Invalid texture id %d", currentTexId );
      currentTexGlId = AssetManager::getTexture( currentTexId ).glId;
      glBindTexture( GL_TEXTURE_2D, currentTexGlId );
      currentSubBufferStart = spriteInd;
    }
  }
  //render the last sub-buffer
  uint32_t spriteCountInSubBuffer = spritesToRenderCount - currentSubBufferStart;
  glDrawArrays( GL_TRIANGLES, vertsPerSprite * currentSubBufferStart, vertsPerSprite * spriteCountInSubBuffer );
}
