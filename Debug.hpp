#pragma once

#include <fstream>

#include "EngineCommon.hpp"
#include "Math.hpp"

// TODO mix Logger and DebugRenderer into Degug class

/////////////////////// Error handling and logging //////////////////////

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

void printGlfwError( s32 error, const char* description );

void APIENTRY
printOpenglError( GLenum source, GLenum type, GLuint id, GLenum severity,
		  GLsizei length, const GLchar* message, const void* userParam );

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

#define ASSERT( condition, ... ) {                                      \
    if ( !( condition ) ) {                                             \
      haltWithMessage( #condition, __FILE__, __FUNC__,  __LINE__, __VA_ARGS__ ); \
    }                                                                   \
  }

#define VALIDATE_ENTITY( entity )                                       \
  ASSERT( EntityManager::isAlive( ( entity ) ), "Invalid entity id %d", ( entity ) )

#define VALIDATE_ENTITIES( entities ) {                               \
    for ( u32 entInd = 0; entInd < ( entities ).size(); ++entInd ) {	\
      VALIDATE_ENTITY( ( entities )[ entInd ] );                      \
    }                                                                 \
  }

#endif

////////////////////////// Drawing debug shapes ///////////////////////////

class DebugRenderer {
  struct DebugCircle {
    Color color;
    Vec2 position;
    float radius;
  };
  static RenderInfo renderInfo;
  static std::vector< DebugCircle > circleBufferData;
public:
  static void initialize();
  static void shutdown();
  static void addCircle( Circle circle, Color color );
  static void renderAndClear();
  static void setOrthoProjection( float aspectRatio, float height );
};
