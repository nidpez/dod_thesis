#pragma once

#include <fstream>
#include <chrono>
#include <string>

#ifdef PROFILING
#include "papi.h"
#endif

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
  struct DebugRect {
    Color color;
    Vec2 min, max;
  };
  static RenderInfo circleRenderInfo;
  static RenderInfo rectRenderInfo;
  static std::vector< DebugCircle > circleBufferData;
  static std::vector< DebugRect > rectBufferData;
public:
  static constexpr Color RED   = { 1, 0, 0, 1 }; 
  static constexpr Color GREEN = { 0, 1, 0, 1 }; 
  static constexpr Color BLUE  = { 0, 0, 1, 1 }; 
  static constexpr Color WHITE = { 1, 1, 1, 1 }; 
  static constexpr Color BLACK = { 0, 0, 0, 1 }; 
  // write messages
  static void initializeLogger();
  static void shutdownLogger();
  static void shutdown();
  static void write( const char* format, ... );
  static void write( const char* format, va_list args );
  static void writeError( const char* format, ... );
  static void writeError( const char* format, va_list args );
  static void haltWithMessage( const char* failedCond, const char* file, const char* function, s32 line, ... );
  // draw basic shapes
  static void initializeRenderer();
  static void drawCircle( Circle circle, Color color );
  static void drawRect( Rect rect, Color color );
#ifdef DOD
  static void drawShape( Shape shape, Color color );
#elif defined OOP
  static void drawShape( Shape* shape, Color color );
#endif
  static void renderAndClear();
  static void setOrthoProjection( float aspectRatio, float height );
};

#ifdef __GNUC__

#define __FUNC__ __PRETTY_FUNCTION__

#else

#define __FUNC__ __func__

#endif
  
#ifdef NDEBUG

#define ASSERT( condition, ... ) ( ( void )0 )

#else

#define ASSERT( condition, ... ) {                                      \
    if ( !( condition ) ) {                                             \
      Debug::haltWithMessage( #condition, __FILE__, __FUNC__,  __LINE__, __VA_ARGS__ ); \
    }                                                                   \
  }

#endif

// based on chapter 9.8 'In-Game Profiling' of the book 'Game Engine Architecture' by Jason Gregory, second edition

typedef std::chrono::time_point< std::chrono::high_resolution_clock > TimePoint;
typedef std::chrono::high_resolution_clock Clock;

struct AutoProfile {
  AutoProfile( const char* name );
  ~AutoProfile();
};

class Profiler {
#ifdef DOD
  static constexpr const char* PROFILER_LOG_FILE_NAME = "DODprofilerLog.csv";
#elif defined OOP
  static constexpr const char* PROFILER_LOG_FILE_NAME = "OOPprofilerLog.csv";
#endif
  static FILE* profilerLog;
  
  static u32 frameNumber;
  
  static constexpr const u8 NUM_PERF_COUNTERS = 3;
  static const s32 PERF_COUNTER_CODES[ NUM_PERF_COUNTERS ];
  static const char* PERF_COUNTER_NAMES[ NUM_PERF_COUNTERS ];
  static s32 perfCounters;
  
  struct ProfileSample {
    long long startPerfCounts[ NUM_PERF_COUNTERS ];
    long long deltaPerfCounts[ NUM_PERF_COUNTERS ];
    TimePoint startTime;
    u64 elapsedNanos;
    u32 callCount;
    u32 recursionCount;
  };
  static std::vector< ProfileSample > samples;

  // SampleNodeIndex & ProfileSampleIndex value 0 is reserved to mean null,
  // so these indices must start from 1
  // TODO standarize indices starting at 1
  typedef u32 ProfileSampleIndex;
  typedef u32 SampleNodeIndex;
  struct SampleNode {
    SampleNodeIndex parent;
    SampleNodeIndex firstChild;
    SampleNodeIndex nextSibling;
    const char* name;
    ProfileSampleIndex dataInd;
  };
  static std::vector< SampleNode > sampleTree;
  static SampleNodeIndex currentNodeInd;
  static SampleNodeIndex addChildSampleNode( SampleNodeIndex nodeInd, const char* name );
  static SampleNodeIndex getParentSampleNode( const SampleNodeIndex nodeInd );
  static SampleNodeIndex getChildSampleNode( SampleNodeIndex nodeInd, const char* name );
  static void callSampleNode( const SampleNodeIndex nodeInd );
  static bool returnFromSampleNode( const SampleNodeIndex nodeInd );
  
public:
  static void initialize();
  static void shutdown();
  static void startProfile( const char* name );
  static void stopProfile();
  static void updateOutputsAndReset();  
};

#ifndef PROFILING

#define PROFILE_BLOCK( name ) ( ( void )0 )

#define PROFILE ( ( void )0 )

#else

#define PROFILE_BLOCK( name ) AutoProfile p( name )

#define PROFILE PROFILE_BLOCK( __FUNC__ )

#endif
