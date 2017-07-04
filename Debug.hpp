#pragma once

#include <fstream>
#include <chrono>
#include <string>

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

// based on chapter 9.8 'In-Game Profiling' of the book 'Game Engine Architecture' by Jason Gregory, second edition

typedef TimePoint std::chrono::time_point< std::chrono::high_resolution_clock >;
typedef Clock std::chrono::high_resolution_clock;

struct AutoProfile {
  AutoProfile( const char* name );
  ~AutoProfile();
};

class Profiler {
  static constexpr const char* PROFILER_LOG_FILE_NAME = "profilerLog.csv";
  static FILE* profilerLog;
  
  static u32 frameNumber;
  
  struct ProfileSample {
    TimePoint startTime;
    u64 elapsedNanos;
    u32 callCount;
    u32 recursionCount;
  };
  static std::vector< ProfileSample > samples;
  
  struct SampleNode {
    SampleNode* parent;
    SampleNode* firstChild;
    SampleNode* nextSibling;
    const char* name;
    ProfileSample* data;
  };
  static std::vector< SampleNode > sampleTree;
  static SampleNode* currentNode;
  static SampleNode* addChildSampleNode( const SampleNode* node, const char* name );
  static SampleNode* getParentSampleNode( const SampleNode* node );
  static SampleNode* getChildSampleNode( const SampleNode* node, const char* name );
  static void callSampleNode( const SampleNode* node );
  static bool returnFromSampleNode( const SampleNode* node );
  
public:
  static void initialize();
  static void shutdown();
  static void startProfile( const char* name );
  static void stopProfile();
  static void updateOutputsAndReset();  
};

#ifdef NDEBUG

#define PROFILE_BLOCK( name ) ( ( void )0 )

#define PROFILE ( ( void )0 )

#else

#define PROFILE_BLOCK( name ) AutoProfile p( name )

#define PROFILE PROFILE_BLOCK( __FUNC__ )

#endif
