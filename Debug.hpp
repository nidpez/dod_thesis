#pragma once

#include <fstream>
#include <vector>

/////////////////////// Error handling and logging ////////////////////
/////////////////////// and Drawing debug shapes //////////////////////

class Debug {
  static constexpr const char* LOG_FILE_NAME = "log.txt";
  static FILE* log;
  struct DebugCircle {
    Color color;
    Vec2 position;
    float radius;
  };
  static RenderInfo renderInfo;
  static std::vector< DebugCircle > circleBufferData;
public:
  // write messages
  static void initializeLogger();
  static void shutdown();
  static void write( const char* format, ... );
  static void write( const char* format, va_list args );
  static void writeError( const char* format, ... );
  static void writeError( const char* format, va_list args );
  static void haltWithMessage( const char* failedCond, const char* file, const char* function, s32 line, ... );
  // draw basic shapes
  static void initializeRenderer();
  static void drawCircle( Circle circle, Color color );
  static void renderAndClear();
  static void setOrthoProjection( float aspectRatio, float height );
};
  
#ifdef NDEBUG

#define ASSERT( condition, ... ) ( ( void )0 )

#else

#ifdef __GNUC__

#define __FUNC__ __PRETTY_FUNCTION__

#else

#define __FUNC__ __func__

#endif

#define ASSERT( condition, ... ) {                                      \
    if ( !( condition ) ) {                                             \
      Debug::haltWithMessage( #condition, __FILE__, __FUNC__,  __LINE__, __VA_ARGS__ ); \
    }                                                                   \
  }

#endif
