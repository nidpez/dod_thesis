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
#include <random>

typedef int64_t 	s64;
typedef int32_t 	s32;
typedef int16_t 	s16;
typedef int8_t		s8;
typedef uint64_t	u64;
typedef uint32_t 	u32;
typedef uint16_t 	u16;
typedef uint8_t 	u8;

const float PIXELS_PER_UNIT = 4.0f;

//math
struct Vec2 {
  union {
    float x, u;
  };
  union {
    float y, v;
  };
  static const Vec2 zero; 
  static const Vec2 one;
};

Vec2 operator+( Vec2 a, Vec2 b );

Vec2& operator+=( Vec2& a, Vec2 b );

Vec2 operator-( Vec2 a, Vec2 b );

Vec2& operator-=( Vec2& a, Vec2 b );

Vec2& operator-( Vec2& vec );

Vec2 operator*( Vec2 a, Vec2 b );

Vec2& operator*=( Vec2& a, Vec2 b );

Vec2 operator*( Vec2 vec, float scale );

Vec2 operator*( float scale, Vec2 vec );

Vec2& operator*=( Vec2& vec, float scale );

float dot( Vec2 a, Vec2 b );

Vec2 rotateVec2( Vec2 vec, float orientation );

//resources
typedef u32 TextureHandle;

struct TextureAsset {
  const char* name;
  const u32 width, height;
  const u32 glId;
};

class AssetManager {
  static std::vector< TextureAsset > textureAssets;
public:
  static void initialize();
  static void shutdown();
  static std::vector< TextureHandle > loadTextures( std::vector< const char* >& names );
  static void destroyTextures( const std::vector< TextureHandle >& textures );
  static bool isTextureAlive( TextureHandle texture );
  static TextureAsset getTexture( TextureHandle texture );
};
  
//entity and component managers

/*Entity Handle system based on http://bitsquid.blogspot.com.co/2014/08/building-data-oriented-entity-system.html and http://gamesfromwithin.com/managing-data-relationships*/
const u32 HANDLE_INDEX_BITS = 21;
const u32 HANDLE_GENERATION_BITS = 32 - HANDLE_INDEX_BITS;
//With 21 index bits 2 million entities are possible at a time.
const u32 MAX_ENTITIES = 1 << HANDLE_INDEX_BITS;

//index 0 is invalid so an EntityHandle can be set to 0 by default 
struct EntityHandle {
  u32 index : HANDLE_INDEX_BITS;
  u32 generation : HANDLE_GENERATION_BITS;
  operator u32() const;
};

class EntityManager {
  struct Generation { //can't just use u32 since they overflow at different values
    u32 generation : HANDLE_GENERATION_BITS;
  };
  const static u32 MIN_FREE_INDICES = 1024;
  static std::vector< Generation > generations;
  static std::deque< u32 > freeIndices;
public:
  static void initialize();
  static void shutdown();
  static std::vector< EntityHandle > create( u32 amount );
  static void destroy( const std::vector< EntityHandle >& entities );
  static bool isAlive( EntityHandle entity );
};

typedef u32 ComponentIndex;

struct SetComponentMapArg {
  EntityHandle entity;
  ComponentIndex compInd;
};

struct ComponentMap {
  std::unordered_map< u32, ComponentIndex > map;
  void set( const std::vector< SetComponentMapArg >& mappedPairs );
  std::vector< EntityHandle > have( const std::vector< EntityHandle >& entities );
  std::vector< ComponentIndex > lookup( const std::vector< EntityHandle >& entities );
};

struct Transform {
  Vec2 position;
  Vec2 scale;
  float orientation;
};

struct RotateAroundArg {
  Vec2 point;
  float rotation;
};

class TransformManager {
  struct TransformComp {
    EntityHandle entity;
    Transform local;
    Transform world;
    ComponentIndex parent;
    ComponentIndex firstChild;
    ComponentIndex nextSibling;
  };
  static std::vector< TransformComp > transformComps;
  static ComponentMap componentMap;
public:
  static void initialize();
  static void shutdown();
  static void set( const std::vector< EntityHandle >& entities, const std::vector< Transform >& transforms );
  static void remove( const std::vector< EntityHandle >& entities );
  static void rotate( const std::vector< EntityHandle >& entities, const std::vector< float >& rotations );
  static void rotateAround( const std::vector< EntityHandle >& entities, const std::vector< RotateAroundArg >& rotations );
  static void translate( const std::vector< EntityHandle >& entities, const std::vector< Vec2 >& translations );
  static void scale( const std::vector< EntityHandle >& entities, const std::vector< Vec2 >& scales );
  static void update( const std::vector< EntityHandle >& entities, const std::vector< Transform >& transforms );
  static std::vector< Transform > get( const std::vector< EntityHandle >& entities );
  static std::vector< EntityHandle > getLastUpdated();
};

struct CircleColliderComp {
  EntityHandle entity;
  Vec2 center;
  float radius;
};

//TODO allow multiple colliders per entity (with linked list?)
class CircleColliderManager {
  struct __CircleColliderComp {
    EntityHandle entity;
    Vec2 center;
    float radius;
    //transform cache
    Vec2 position;
    Vec2 scale;
  };
  static std::vector< __CircleColliderComp > circleColliderComps;
  static ComponentMap componentMap;
public:
  static void initialize();
  static void shutdown();
  static void add( const std::vector< CircleColliderComp >& circleColliders );
  static void addAndFitToSpriteSize( const std::vector< EntityHandle >& entities );
  static void remove( const std::vector< EntityHandle >& entities );
  static void fitToSpriteSize( const std::vector< EntityHandle >& entities );
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
  s32 projUnifLoc[ 4 ];
  u32 vboIds[ 2 ]; //expect to mostly use one
  u32 vaoId;
  u32 shaderProgramId;
};

struct SpriteComp {
  EntityHandle entity;
  TextureHandle textureId;
  Rect texCoords;
  Vec2 size;
};

struct SetSpriteArg {
  EntityHandle entity;
  TextureHandle textureId;
  Rect texCoords;
};
    
class SpriteManager {
  struct __SpriteComp {
    EntityHandle entity;
    TextureHandle textureId;
    Rect texCoords;
    Vec2 size;
    //transform cache
    Vec2 position;
    Vec2 scale;
    float orientation;
    explicit operator SpriteComp() const;
  };
  static std::vector< __SpriteComp > spriteComps;
  static ComponentMap componentMap;
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
public:
  static void initialize();
  static void shutdown();
  static void set( const std::vector< SetSpriteArg >& sprites );
  static void remove( const std::vector< EntityHandle >& entities );
  static std::vector< SpriteComp > get( const std::vector< EntityHandle >& entities );
  static void updateAndRender();
  static void setOrthoProjection( float aspectRatio, float height );
};

typedef u32 AnimationHandle;
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
s32 loadShaderSourceFile( const char* name, char** source );

s32 compileShader( const char* name, const GLenum type, GLuint* shaderId );

s32 createShaderProgram( GLuint* shaderProgramId, const char* vertShaderId, const char* fragShaderId, const char* geomShaderId = 0 );


//error handling and logging
void printGlfwError( s32 error, const char* description );

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
  static void writeError( const char* format, ... );
  static void writeError( const char* format, va_list args );
};

void haltWithMessage( const char* failedCond, const char* file, const char* function, s32 line, ... );
  
#ifdef NDEBUG

#define ASSERT( condition, ... ) ( ( void )0 )

#define VALIDATE_ENTITY( entity ) ( ( void )0 )

#define VALIDATE_ENTITIES( entities ) ( ( void )0 )

#else

#ifdef __GNUC__

#define __FUNC__ __PRETTY_FUNCTION__

#else

#define __FUNC__ __func__

#endif

#define ASSERT( condition, ... ) {					\
    if ( !( condition ) ) {						\
      haltWithMessage( #condition, __FILE__, __FUNC__,  __LINE__, __VA_ARGS__ ); \
    }									\
  }

#define VALIDATE_ENTITY( entity )					\
  ASSERT( EntityManager::isAlive( ( entity ) ), "Invalid entity id %d", ( entity ) )

#define VALIDATE_ENTITIES( entities ) {					\
    for ( u32 entInd = 0; entInd < ( entities ).size(); ++entInd ) {	\
      VALIDATE_ENTITY( ( entities )[ entInd ] );			\
    }									\
  }

#endif

//misc
GLFWwindow* createWindowAndGlContext( const char* const windowTitle );

s32 main() {
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
  s32 windowWidth, windowHeight;
  glfwGetWindowSize( window, &windowWidth, &windowHeight );
  glViewport( 0, 0, windowWidth, windowHeight );
  float aspect = windowWidth / ( float )windowHeight;

  SpriteManager::setOrthoProjection( aspect, 100 );
  DebugRenderer::setOrthoProjection( aspect, 100 );
  
  //load textures
  std::vector< const char* > textureNames = { "astronaut.png", "planetSurface.png" };
  std::vector< TextureHandle > textureHandles = AssetManager::loadTextures( textureNames );
  
  AnimationFrame runFrames[] = {
    { { 0.0f, 0.0f },		{ 1.0f / 5.0f, 1.0f } },
    { { 1.0f / 5.0f, 0.0f },	{ 2.0f / 5.0f, 1.0f } },
    { { 2.0f / 5.0f, 0.0f },	{ 3.0f / 5.0f, 1.0f } },
    { { 3.0f / 5.0f, 0.0f },	{ 4.0f / 5.0f, 1.0f } }
  };
  
  //Rect planetTexCoords = { { 0.0f, 0.0f }, {1.0f, 1.0f } };

  //create entities to test transform component
  std::vector< std::vector< EntityHandle > > entityHandles( 6 );
  u8 blockWidth = 7;
  u8 blockHeight = 10;
  u8 blockSize = blockWidth * blockHeight;
  entityHandles[ 0 ] = EntityManager::create( blockSize );
  entityHandles[ 1 ] = EntityManager::create( blockSize );
  entityHandles[ 2 ] = EntityManager::create( blockSize );
  entityHandles[ 3 ] = EntityManager::create( blockSize );
  entityHandles[ 4 ] = EntityManager::create( blockSize );
  entityHandles[ 5 ] = EntityManager::create( blockSize );

  //same sprite
  SetSpriteArg spriteArg = { { 0, 0 }, textureHandles[ 0 ], static_cast< Rect >( runFrames[ 0 ] ) };
  std::vector< SetSpriteArg > sprites( 6 * blockSize, spriteArg );
  for ( u16 v = 0; v < 6; ++v ) {
    for ( u16 i = 0; i < blockSize; ++i ) {
      sprites[ v * blockSize + i ].entity = entityHandles[ v ][ i ];
    }
  }
  SpriteManager::set( sprites );

  //all positions
  const u8 halfViewportHeight = 50; //viewport height = 100 as defined nowhere
  const u8 halfViewportWidth = halfViewportHeight * aspect;
  const float cellWidth = halfViewportWidth / 10.0f;
  const float cellHeight = halfViewportHeight / 10.0f;
  std::vector< std::vector< Transform > > transforms( 6 );
  for ( u32 v = 0; v < 6; ++v ) {
    for ( u32 i = 0; i < blockWidth; ++i ) {
      for ( u32 j = 0; j < blockHeight; ++j ) {
	float x = static_cast< float >( ( v % 3 ) * blockWidth + i + 0.5f ) * cellWidth - halfViewportWidth;
	float y = static_cast< float >( ( ( v >= 3 ) ? blockHeight : 0 ) + j + 0.5f ) * cellHeight - halfViewportHeight;
	transforms[ v ].push_back( { { x, y }, { 1.0f, 1.0f }, 0.0f } );
      }
    }
    TransformManager::set( entityHandles[ v ], transforms[ v ] );
  }

  std::default_random_engine generator;
  auto frand = std::uniform_real_distribution< float >( -1.0f, 1.0f );
  
  std::vector< float > rotationSpeeds( blockSize );
  std::vector< float > rotations( blockSize );
  for ( u16 i = 0; i < blockSize; ++i ) {
    rotationSpeeds[ i ] = frand( generator );
  }
  
  std::vector< Vec2 > translationSpeeds( blockSize );
  std::vector< Vec2 > translations( blockSize );
  for ( u16 i = 0; i < blockSize; ++i ) {
    translationSpeeds[ i ] = { frand( generator ), frand( generator ) };
  }

  std::vector< RotateAroundArg > rotationsAround( blockSize );
  for ( u16 i = 0; i < blockSize; ++i ) {
    Vec2 offset = { frand( generator ), frand( generator ) };
    rotationsAround[ i ] = { transforms[ 2 ][ i ].position + offset * 5.0f, 0.0f };
  }

  std::vector< Vec2 > scaleSpeeds( blockSize );
  std::vector< Vec2 > scales( blockSize, { 1.0f, 1.0f } );
  for ( u16 i = 0; i < blockSize; ++i ) {
    scaleSpeeds[ i ] = { frand( generator ), frand( generator ) };
  }

  std::vector< Vec2 > initialPositions1( blockSize );
  std::vector< Vec2 > initialPositions2( blockSize );
  std::vector< Vec2 > positions1( blockSize );
  std::vector< Vec2 > positions2( blockSize );
  std::vector< Vec2 > translationSpeeds2( blockSize );
  std::vector< float > rotationSpeeds2( blockSize );
  std::vector< Vec2 > scaleSpeeds2( blockSize );
  std::vector< Vec2 > scales2( blockSize, { 1.0f, 1.0f } );
  for ( u16 i = 0; i < blockSize; ++i ) {
    initialPositions1[ i ] = transforms[ 4 ][ i ].position;
    initialPositions2[ i ] = transforms[ 5 ][ i ].position;
    positions1[ i ] = initialPositions1[ i ];
    positions2[ i ] = initialPositions2[ i ];
    translationSpeeds2[ i ] = { frand( generator ), frand( generator ) };
    scaleSpeeds2[ i ] = { frand( generator ), frand( generator ) };
    rotationSpeeds2[ i ] = frand( generator );
  }
  
  double lastChangedParams = glfwGetTime();
  double now;
  
  //main loop      
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
    
    for ( u16 i = 0; i < blockSize; ++i ) {
      rotations[ i ] = rotationSpeeds[ i ] * deltaT * 10.0f;
    }
    now = glfwGetTime();
    bool timeToChangeParams = false;
    if ( now - lastChangedParams >= 1.0f ) {
      timeToChangeParams = true;
      lastChangedParams = now;
    }
    for ( u16 i = 0; i < blockSize; ++i ) {
      if ( timeToChangeParams ) {
	translationSpeeds[ i ] = -translationSpeeds[ i ];
      }
      translations[ i ] = translationSpeeds[ i ] * deltaT * 10.0f;
    }
    for ( u16 i = 0; i < blockSize; ++i ) {
      rotationsAround[ i ].rotation = rotationSpeeds[ i ] * deltaT * 10.0f;
    }
    for ( u16 i = 0; i < blockSize; ++i ) {
      if ( timeToChangeParams ) {
	scaleSpeeds[ i ] = -scaleSpeeds[ i ];
      }
      scales[ i ] += scaleSpeeds[ i ] * deltaT;
    }
    for ( u16 i = 0; i < blockSize; ++i ) {
      positions1[ i ] += translationSpeeds2[ i ] * deltaT * 5.0f;
      Vec2 pos = positions1[ i ];
      pos.x = std::sin( pos.x );
      pos.y = std::sin( pos.y );
      transforms[ 4 ][ i ].position = initialPositions1[ i ] + pos * 5.0f;
      transforms[ 4 ][ i ].orientation += rotationSpeeds[ i ] * deltaT * 10.0f;
    }
    for ( u16 i = 0; i < blockSize; ++i ) {
      positions2[ i ] += translationSpeeds2[ i ] * deltaT * 5.0f;
      Vec2 pos = positions1[ i ];
      pos.x = std::sin( pos.x );
      pos.y = std::sin( pos.y );
      transforms[ 5 ][ i ].position = initialPositions2[ i ] + pos * 5.0f;
      scales2[ i ] += scaleSpeeds2[ i ] * deltaT * 3.0f;
      Vec2 scale = scales2[ i ];
      scale.x = std::sin( scales2[ i ].x );
      scale.y = std::sin( scales2[ i ].y );
      transforms[ 5 ][ i ].scale = scale * 1.5f;
    } 
    TransformManager::rotate( entityHandles[ 0 ], rotations );
    TransformManager::translate( entityHandles[ 1 ], translations );
    TransformManager::rotateAround( entityHandles[ 2 ], rotationsAround );
    TransformManager::scale( entityHandles[ 3 ], scales );
    TransformManager::update( entityHandles[ 4 ], transforms[ 4 ] );
    TransformManager::update( entityHandles[ 5 ], transforms[ 5 ] );
    
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
  AssetManager::destroyTextures( textureHandles );
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


s32 loadShaderSourceFile( const char* name, char** source ) {
  s32 error = 0;
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

s32 compileShader( const char* name, const GLenum type, GLuint* shaderId ) {
  GLchar* source;
  s32 fileError = loadShaderSourceFile( name, &source );
  if ( fileError ) {
    printf( "Unable to open vertex shader source file '%s': %s\n", name,
	    strerror( fileError ) );
    return 1;
  } 
  *shaderId = glCreateShader( type );
  glShaderSource( *shaderId, 1, ( const char** )&source, nullptr );
  glCompileShader( *shaderId );
  s32 compiled; 
  glGetShaderiv( *shaderId, GL_COMPILE_STATUS, &compiled );
  if ( compiled == GL_FALSE ) {
    s32 maxLength;
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

s32 createShaderProgram( GLuint* shaderProgramId, const char* vertShaderFile, const char* fragShaderFile, const char* geomShaderFile ) {
  GLuint vertShaderId;
  s32 error = compileShader( vertShaderFile, GL_VERTEX_SHADER, &vertShaderId );
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
  s32 linked = 0;
  glGetProgramiv( *shaderProgramId, GL_LINK_STATUS, ( s32 * )&linked );
  if( linked == GL_FALSE ) {
    s32 maxLength;
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
  glDeleteShader( vertShaderId );
  if ( geomShaderFile != 0 ) {
    glDetachShader( *shaderProgramId, geomShaderId );
    glDeleteShader( geomShaderId );
  }
  glDetachShader( *shaderProgramId, fragShaderId );
  glDeleteShader( fragShaderId );
  return 0;
}

void printGlfwError( s32 error, const char* description ) {
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
}

void Logger::shutdown() {
  fclose( log );
}

void Logger::write( const char* format, ... ) {
  va_list args;
  va_start( args, format );
  write( format, args );
  va_end( args );
}

void Logger::write( const char* format, va_list args ) {
  va_list args2;
  va_copy( args2, args );
  vprintf( format, args );
  va_end( args2 );
}

void Logger::writeError( const char* format, ... ) {
  va_list args;
  va_start( args, format );
  writeError( format, args );
  va_end( args );
}

void Logger::writeError( const char* format, va_list args ) {
  va_list args2;
  va_copy( args2, args );
  vprintf( format, args );
  vfprintf( log, format, args2 );
  va_end( args2 );
}
  
void haltWithMessage( const char* failedCond, const char* file, const char* function, s32 line, ... ) {  
  std::time_t now = std::time( nullptr );
  char* date = std::ctime( &now );
  Logger::writeError( "%s\tAssertion '%s' failed at %s, %s, line %d:\n\t",
		      date, failedCond, file, function, line ); 
  va_list args;
  va_start( args, line );
  char* msgFormat = va_arg( args, char* );
  Logger::writeError( msgFormat, args );
  va_end( args );
  Logger::writeError( "\n" );
  Logger::shutdown();
  std::abort();
}

const Vec2 Vec2::zero = {};
const Vec2 Vec2::one = { 1.0f, 1.0f };

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

Vec2& operator-( Vec2& vec ) {
  vec = Vec2::zero - vec;
  return vec;
}

Vec2 operator*( Vec2 a, Vec2 b ){
  return { a.x * b.x, a.y * b.y };
}

Vec2& operator*=( Vec2& a, Vec2 b ) {
  a = a * b;
  return a;
}

Vec2 operator*( Vec2 vec, float scale ) {
  return { vec.x * scale, vec.y * scale };
}

Vec2 operator*( float scale, Vec2 vec ) {
  return { vec.x * scale, vec.y * scale };
}

Vec2& operator*=( Vec2& vec, float scale ) {
  vec = vec * scale;
  return vec;
}

float dot( Vec2 a, Vec2 b ) {
  return a.x * b.x + a.y * b.y;
}

Vec2 rotateVec2( Vec2 vec, float orientation ){
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
  s32 error = createShaderProgram( &renderInfo.shaderProgramId,
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
  for ( u32 i = 0; i < circles.size(); ++i ) {
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
std::deque< u32 > EntityManager::freeIndices;

EntityHandle::operator u32() const {
  return this->generation << HANDLE_INDEX_BITS | this->index;
}

void EntityManager::initialize() {
}

void EntityManager::shutdown() {
}

std::vector< EntityHandle > EntityManager::create( u32 amount ){
  std::vector< EntityHandle > newEntities( amount );
  for ( u32 entInd = 0; entInd < amount; ++entInd ) {
    u32 index;
    if ( freeIndices.size() < MIN_FREE_INDICES ) {
      generations.push_back( { 0 } );
      index = generations.size();    
      ASSERT( index < MAX_ENTITIES, "Tried to create more than %d entities", MAX_ENTITIES ); 
    } else {
      index = freeIndices.front();
      freeIndices.pop_front();
    }
    EntityHandle newEntity = { index, generations[ index - 1 ].generation };
    newEntities[ entInd ] = newEntity;
  }
  return newEntities;
}

bool EntityManager::isAlive( EntityHandle entity ) {
  return entity.index > 0 && generations[ entity.index - 1 ].generation == entity.generation;
}

void ComponentMap::set( const std::vector< SetComponentMapArg >& mappedPairs ) {
  for ( u32 pairInd = 0; pairInd < mappedPairs.size(); ++pairInd ) {
    EntityHandle entity = mappedPairs[ pairInd ].entity;
    ComponentIndex compInd = mappedPairs[ pairInd ].compInd;
    bool inserted = map.insert( { entity, compInd } ).second;  
    ASSERT( inserted, "Could not map entity %d to component index %d", entity, compInd );
  }
}

std::vector< EntityHandle > ComponentMap::have( const std::vector< EntityHandle >& entities ) {
  VALIDATE_ENTITIES( entities );  
  std::vector< EntityHandle > result;
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    if ( map.count( entities[ entInd ] ) ) {
      result.push_back( entities[ entInd ] );
    }
  }
  return result;
}

std::vector< ComponentIndex > ComponentMap::lookup( const std::vector< EntityHandle >& entities ) {
  VALIDATE_ENTITIES( entities );
  std::vector< ComponentIndex > result( entities.size() );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    auto iterator = map.find( entities[ entInd ] );
    ASSERT( iterator != map.end(), "Entity %d has no given component", entities[ entInd ] );
    result[ entInd ] = iterator->second;
  }
  return result;
}

std::vector< TransformManager::TransformComp > TransformManager::transformComps;
ComponentMap TransformManager::componentMap;

void TransformManager::initialize() {
}

void TransformManager::shutdown() {
}

void TransformManager::set( const std::vector< EntityHandle >& entities, const std::vector< Transform >& transforms ) {
  VALIDATE_ENTITIES( entities );
  std::vector< SetComponentMapArg > mappedPairs( transforms.size() );
  for ( u32 trInd = 0; trInd < transforms.size(); ++trInd ) {
    Transform transform = transforms[ trInd ];
    EntityHandle entity = entities[ trInd ];
    transformComps.push_back( { entity, transform, transform, 0, 0, 0 } );
    u32 compInd = transformComps.size() - 1;
    mappedPairs[ trInd ] = { entity, compInd };
  }
  componentMap.set( mappedPairs );
}

void TransformManager::rotate( const std::vector< EntityHandle >& entities, const std::vector< float >& rotations ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    transformComps[ componentInds[ entInd ] ].local.orientation += rotations[ entInd ];
  }
  //TODO mark transform components as updated
}

void TransformManager::rotateAround( const std::vector< EntityHandle >& entities, const std::vector< RotateAroundArg >& rotations ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    ComponentIndex componentInd = componentInds[ entInd ];
    RotateAroundArg rotateArg = rotations[ entInd ];
    Transform transform = transformComps[ componentInd ].local;
    transform.orientation += rotateArg.rotation;
    transform.position = rotateVec2( transform.position - rotateArg.point, rotateArg.rotation ) + rotateArg.point;
    transformComps[ componentInd ].local = transform;
  }
  //TODO mark transform components as updated
}

void TransformManager::translate( const std::vector< EntityHandle >& entities, const std::vector< Vec2 >& translations ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    transformComps[ componentInds[ entInd ] ].local.position += translations[ entInd ];
  }
  //TODO mark transform components as updated
}

void TransformManager::scale( const std::vector< EntityHandle >& entities, const std::vector< Vec2 >& scales ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    transformComps[ componentInds[ entInd ] ].local.scale = scales[ entInd ];
  }
  //TODO mark transform components as updated
}

void TransformManager::update( const std::vector< EntityHandle >& entities, const std::vector< Transform >& transforms ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    ComponentIndex componentInd = componentInds[ entInd ];
    Transform transformArg = transforms[ entInd ];
    Transform transform = transformComps[ componentInd ].local;
    transform.position = transformArg.position;
    transform.scale = transformArg.scale;
    transform.orientation = transformArg.orientation;
    transformComps[ componentInd ].local = transform;
  }
  //TODO mark transform components as updated
}

std::vector< Transform > TransformManager::get( const std::vector< EntityHandle >& entities ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  std::vector< Transform > result( entities.size() );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    result[ entInd ] = transformComps[ componentInds[ entInd ] ].local;
  }
  return result;
}

std::vector< EntityHandle > TransformManager::getLastUpdated() {
  //TODO actually compute which transforms have been updated since last frame
  std::vector< EntityHandle > result( transformComps.size() );
  for ( u32 entInd = 0; entInd < transformComps.size(); ++entInd ) {
    result[ entInd ] = transformComps[ entInd ].entity;
  }
  return result;
}

std::vector< CircleColliderManager::__CircleColliderComp > CircleColliderManager::circleColliderComps;
ComponentMap CircleColliderManager::componentMap;

void CircleColliderManager::initialize() {
}

void CircleColliderManager::shutdown() {
}

void CircleColliderManager::add( const std::vector< CircleColliderComp >& circleColliders ) {
  std::vector< SetComponentMapArg > mappedPairs( circleColliders.size() );
  for ( u32 collInd = 0; collInd < circleColliders.size(); ++collInd ) {
    CircleColliderComp circleColliderComp = circleColliders[ collInd ];
    EntityHandle entity = circleColliderComp.entity;
    VALIDATE_ENTITY( entity );
    float radius = circleColliderComp.radius;
    ASSERT( radius > 0.0f, "A collider of radius %f is useless", radius );
    Vec2 center = circleColliderComp.center;
    circleColliderComps.push_back( { entity, center, radius, { 0, 0 }, { 0, 0 } } );
    u32 compInd = circleColliderComps.size() - 1;
    mappedPairs[ collInd ] = { entity, compInd };
  }
  componentMap.set( mappedPairs );
}

void CircleColliderManager::addAndFitToSpriteSize( const std::vector< EntityHandle >& entities ) {
  std::vector< CircleColliderComp > circleColliders( entities.size() );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    circleColliders[ entInd ] = { entities[ entInd ], {}, 1.0f };
  }
  add( circleColliders );
  fitToSpriteSize( entities );
}
  
void CircleColliderManager::updateAndCollide() {
  if ( circleColliderComps.size() == 0 ) {
    return;
  }
  //update local transform cache
  std::vector< EntityHandle > updatedEntities = TransformManager::getLastUpdated();
  //TODO get world transforms here
  std::vector< Transform > updatedTransforms = TransformManager::get( updatedEntities );
  updatedEntities = componentMap.have( updatedEntities );
  std::vector< ComponentIndex > updatedCircleColliders = componentMap.lookup( updatedEntities );
  for ( u32 trInd = 0; trInd < updatedTransforms.size(); ++trInd ) {
    Transform transform = updatedTransforms[ trInd ];
    ComponentIndex circleColliderCompInd = updatedCircleColliders[ trInd ];
    circleColliderComps[ circleColliderCompInd ].position = transform.position;
    circleColliderComps[ circleColliderCompInd ].scale = transform.scale;
  }
  //TODO do frustrum culling
  std::vector< DebugCircle > circles;
  circles.reserve( circleColliderComps.size() );
  for ( u32 colInd = 0; colInd < circleColliderComps.size(); ++colInd ) {
    __CircleColliderComp circleColliderComp = circleColliderComps[ colInd ];
    DebugCircle circle;
    float scaleX = circleColliderComp.scale.x, scaleY = circleColliderComp.scale.y;
    float maxScale = ( scaleX > scaleY ) ? scaleX : scaleY;
    circle.position = circleColliderComp.position + circleColliderComp.center * maxScale;
    circle.radius = circleColliderComp.radius * maxScale;
    circle.color[ 0 ] =  0.0f;
    circle.color[ 1 ] =  1.0f;
    circle.color[ 2 ] =  0.0f;
    circle.color[ 3 ] =  1.0f;
    circles.push_back( circle );
  }
  DebugRenderer::addCircles( circles );
}

void CircleColliderManager::fitToSpriteSize( const std::vector< EntityHandle >& entities ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  std::vector< SpriteComp > spriteComps = SpriteManager::get( entities );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    SpriteComp spriteComp = spriteComps[ entInd ];
    ComponentIndex componentInd = componentInds[ entInd ];
    Vec2 size = spriteComp.size;
    float maxSize = ( size.x > size.y ) ? size.x : size.y;
    circleColliderComps[ componentInd ].radius = maxSize / 2.0f;
  }
}

std::vector< TextureAsset > AssetManager::textureAssets;

void AssetManager::initialize() {
}

void AssetManager::shutdown() {
}
 
std::vector< TextureHandle > AssetManager::loadTextures( std::vector< const char* >& names ) {
  std::vector< TextureHandle > textureHandles( names.size() );
  for ( u32 texInd = 0; texInd < names.size(); ++texInd ) {
    u32 width, height;
    s32 channels;
    unsigned char* texData = SOIL_load_image( names[ texInd ], reinterpret_cast< int* >( &width ), reinterpret_cast< int* >( &height ), &channels, SOIL_LOAD_RGBA );  
    ASSERT( texData != 0, "Error loading texture %s: %s", names[ texInd ], SOIL_last_result() );
    //pixelart seems to not want to be compressed to DXT
    s32 newWidth = width, newHeight = height;
    u32 glId = SOIL_create_OGL_texture( texData, &newWidth, &newHeight, channels, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y ); 
    SOIL_free_image_data( texData );
    ASSERT( glId > 0, "Error sending texture %s to OpenGL: %s", names[ texInd ], SOIL_last_result() );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    textureAssets.push_back( { names[ texInd ], width, height, glId } );
    Logger::write( "Texture '%s' successfully loaded (glId = %d)\n", names[ texInd ], glId );
    textureHandles[ texInd ] = textureAssets.size() - 1;
  }
  return textureHandles;
}

void AssetManager::destroyTextures( const std::vector< TextureHandle >& textures ) {
  for ( u32 texInd = 0; texInd < textures.size(); ++texInd ) {
    TextureHandle texture = textures[ texInd ];
    ASSERT( isTextureAlive( texture ), "Invalid texture id %d", texture );  
    glDeleteTextures( 1, &textureAssets[ texture ].glId );
    std::memset(  &textureAssets[ texture ], 0, sizeof( TextureAsset ) );
  }
}

bool AssetManager::isTextureAlive( TextureHandle texture ) {
  return texture < textureAssets.size() && textureAssets[ texture ].glId > 0;
}

TextureAsset AssetManager::getTexture( TextureHandle texture ) {
  ASSERT( isTextureAlive( texture ), "Invalid texture id %d", texture );  
  return textureAssets[ texture ];
}

std::vector< SpriteManager::__SpriteComp > SpriteManager::spriteComps;
ComponentMap SpriteManager::componentMap;
RenderInfo SpriteManager::renderInfo;
SpriteManager::Pos* SpriteManager::posBufferData;
SpriteManager::UV* SpriteManager::texCoordsBufferData;

SpriteManager::__SpriteComp::operator SpriteComp() const {
  return { this->entity, this->textureId, this->texCoords, this->size };
}

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
  s32 error = createShaderProgram( &renderInfo.shaderProgramId,
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

void SpriteManager::set( const std::vector< SetSpriteArg >& sprites ) {
  std::vector< SetComponentMapArg > mappedPairs( sprites.size() );
  for ( u32 sprInd = 0; sprInd < sprites.size(); ++sprInd ) {
    SetSpriteArg setSpriteArg = sprites[ sprInd ];
    EntityHandle entity = setSpriteArg.entity;
    VALIDATE_ENTITY( entity );
    TextureHandle textureId = setSpriteArg.textureId;
    Rect texCoords = setSpriteArg.texCoords; 
    ASSERT( AssetManager::isTextureAlive( textureId ), "Invalid texture id %d", textureId ); 
    __SpriteComp spriteComp = {};
    spriteComp.entity = entity;
    spriteComp.textureId = textureId;
    spriteComp.texCoords = texCoords;
    TextureAsset texture = AssetManager::getTexture( textureId );
    float width = texture.width * ( texCoords.max.u - texCoords.min.u ) / PIXELS_PER_UNIT;
    float height = texture.height * ( texCoords.max.v - texCoords.min.v ) / PIXELS_PER_UNIT;
    spriteComp.size = { width, height };
    spriteComps.push_back( spriteComp );
    u32 compInd = spriteComps.size() - 1;
    mappedPairs[ sprInd ] = { entity, compInd };
  }
  componentMap.set( mappedPairs );
}

std::vector< SpriteComp > SpriteManager::get( const std::vector< EntityHandle >& entities ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  std::vector< SpriteComp > resultSpriteComps;
  resultSpriteComps.reserve( entities.size() );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    SpriteComp spriteComp = static_cast< SpriteComp >( spriteComps[ componentInds[ entInd ] ] );
    resultSpriteComps.push_back( spriteComp );
  }
  return resultSpriteComps;
}
    
void SpriteManager::setOrthoProjection( float aspectRatio, float height ) {
  float halfHeight = height / 2.0f;
  glUseProgram( renderInfo.shaderProgramId );
  glUniform1f( renderInfo.projUnifLoc[ 0 ], -halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 1 ], halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 2 ], -halfHeight );
  glUniform1f( renderInfo.projUnifLoc[ 3 ], halfHeight );
}

void SpriteManager::updateAndRender() {
  if ( spriteComps.size() == 0 ) {
    return;
  }
  //update local transform cache
  std::vector< EntityHandle > updatedEntities = TransformManager::getLastUpdated();
  //TODO get world transforms here
  std::vector< Transform > updatedTransforms = TransformManager::get( updatedEntities );
  updatedEntities = componentMap.have( updatedEntities );
  std::vector< ComponentIndex > updatedSprites = componentMap.lookup( updatedEntities );
  for ( u32 trInd = 0; trInd < updatedTransforms.size(); ++trInd ) {
    Transform transform = updatedTransforms[ trInd ];
    ComponentIndex spriteCompInd = updatedSprites[ trInd ];
    spriteComps[ spriteCompInd ].position = transform.position;
    spriteComps[ spriteCompInd ].scale = transform.scale;
    spriteComps[ spriteCompInd ].orientation = transform.orientation;
  }
  //build vertex buffer and render for sprites with same texture
  glUseProgram( renderInfo.shaderProgramId );
  glBindVertexArray( renderInfo.vaoId );
  //TODO don't render every sprite every time
  u32 spritesToRenderCount = spriteComps.size();
  //TODO use triangle indices to reduce vertex count
  u32 vertsPerSprite = 6; 
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
  for ( u32 spriteInd = 0; spriteInd < spritesToRenderCount; ++spriteInd ) {
    __SpriteComp spriteComp = spriteComps[ spriteInd ];
    for ( u32 vertInd = 0; vertInd < vertsPerSprite; ++vertInd ) {
      Vec2 vert = baseGeometry[ vertInd ] * spriteComp.size * spriteComp.scale;
      vert = rotateVec2( vert, spriteComp.orientation );
      vert += spriteComp.position;
      posBufferData[ spriteInd * vertsPerSprite + vertInd ].pos = vert;
    }
  } 
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glBufferData( GL_ARRAY_BUFFER, spritesToRenderCount * vertsPerSprite * sizeof( Pos ), posBufferData, GL_STATIC_DRAW );
  //TODO measure how expensive these allocations and deallocations are!
  delete[] posBufferData;
  //build the texture coordinates buffer
  for ( u32 spriteInd = 0; spriteInd < spritesToRenderCount; ++spriteInd ) {
    __SpriteComp spriteComp = spriteComps[ spriteInd ];
    Vec2 texCoords[] = {
      spriteComp.texCoords.min,
      spriteComp.texCoords.max,
      { spriteComp.texCoords.min.u, spriteComp.texCoords.max.v },
      spriteComp.texCoords.min,
      { spriteComp.texCoords.max.u, spriteComp.texCoords.min.v },
      spriteComp.texCoords.max
    };
    for ( u32 vertInd = 0; vertInd < vertsPerSprite; ++vertInd ) {
      texCoordsBufferData[ spriteInd * vertsPerSprite + vertInd ].uv = texCoords[ vertInd ];
    }
  }
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 1 ] );
  glBufferData( GL_ARRAY_BUFFER, spritesToRenderCount * vertsPerSprite * sizeof( UV ), texCoordsBufferData, GL_STATIC_DRAW );
  //TODO measure how expensive these allocations and deallocations are!
  delete[] texCoordsBufferData;
  //issue render commands
  //TODO keep sorted by texture id
  u32 currentTexId = spriteComps[ 0 ].textureId;  
  ASSERT( AssetManager::isTextureAlive( currentTexId ), "Invalid texture id %d", currentTexId );  
  u32 currentTexGlId = AssetManager::getTexture( currentTexId ).glId;
  glBindTexture( GL_TEXTURE_2D, currentTexGlId );
  //mark where a sub-buffer with sprites sharing a texture ends and a new one begins
  u32 currentSubBufferStart = 0;
  for ( u32 spriteInd = 1; spriteInd < spritesToRenderCount; ++spriteInd ) {
    if ( spriteComps[ spriteInd ].textureId != currentTexId ) {
      //send current vertex sub-buffer and render it
      u32 spriteCountInSubBuffer = spriteInd - currentSubBufferStart;
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
  u32 spriteCountInSubBuffer = spritesToRenderCount - currentSubBufferStart;
  glDrawArrays( GL_TRIANGLES, vertsPerSprite * currentSubBufferStart, vertsPerSprite * spriteCountInSubBuffer );
}
