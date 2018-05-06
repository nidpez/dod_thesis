#include "EngineCommon.hpp"

#include <ctime>
#include <cstdarg>

/////////////////////// Error handling and logging //////////////////////

FILE* Debug::log;
RenderInfo Debug::circleRenderInfo;
RenderInfo Debug::rectRenderInfo;
std::vector< Debug::DebugCircle > Debug::circleBufferData;
std::vector< Debug::DebugRect > Debug::rectBufferData;

void Debug::initializeLogger() {
#ifndef NDEBUG
  log = fopen( LOG_FILE_NAME, "a" );
  write( "Logging system initialized.\n" );
#endif
}

void Debug::shutdownLogger() {
#ifndef NDEBUG
  fclose( log );
#endif
}

void Debug::write( const char* format, ... ) {
  va_list args;
  va_start( args, format );
  write( format, args );
  va_end( args );
}

void Debug::write( const char* format, va_list args ) {
  va_list args2;
  va_copy( args2, args );
  vprintf( format, args );
  va_end( args2 );
}

void Debug::writeError( const char* format, ... ) {
  va_list args;
  va_start( args, format );
  writeError( format, args );
  va_end( args );
}

void Debug::writeError( const char* format, va_list args ) {
  va_list args2;
  va_copy( args2, args );
  vprintf( format, args );
  vfprintf( log, format, args2 );
  va_end( args2 );
}
  
void Debug::haltWithMessage( const char* failedCond, const char* file, const char* function, s32 line, ... ) {  
  std::time_t now = std::time( nullptr );
  char* date = std::ctime( &now );
  writeError( "%s\tAssertion \"%s\" failed\n\tat %s,\n\t\t%s, line %d:\n\t\t",
              date, failedCond, file, function, line ); 
  va_list args;
  va_start( args, line );
  char* msgFormat = va_arg( args, char* );
  writeError( msgFormat, args );
  va_end( args );
  writeError( "\n" );
  shutdownLogger();
  std::abort();
}

////////////////////////// Drawing debug shapes ///////////////////////////

constexpr Color Debug::RED;
constexpr Color Debug::GREEN;
constexpr Color Debug::BLUE;
constexpr Color Debug::WHITE;
constexpr Color Debug::BLACK;

void Debug::initializeRenderer() {
#ifndef NDEBUG
  // circle
  // configure buffers
  glGenVertexArrays( 1, &circleRenderInfo.vaoId );
  glBindVertexArray( circleRenderInfo.vaoId );
  glGenBuffers( 1, &circleRenderInfo.vboIds[ 0 ] );
  glBindBuffer( GL_ARRAY_BUFFER, circleRenderInfo.vboIds[ 0 ] );
  glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )0 );
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )( 4 * sizeof( GLfloat ) ) );
  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )( 6 * sizeof( GLfloat ) ) );
  glEnableVertexAttribArray( 2 );
  glBindVertexArray( 0 );
  // create shader program
  circleRenderInfo.shaderProgramId = AssetManager::loadShader( "shaders/DebugCircle.glsl" );
  // get shader's constants' locations
  circleRenderInfo.projUnifLoc[ 0 ] = glGetUniformLocation( circleRenderInfo.shaderProgramId, "projection.left" );
  circleRenderInfo.projUnifLoc[ 1 ] = glGetUniformLocation( circleRenderInfo.shaderProgramId, "projection.right" );
  circleRenderInfo.projUnifLoc[ 2 ] = glGetUniformLocation( circleRenderInfo.shaderProgramId, "projection.bottom" );
  circleRenderInfo.projUnifLoc[ 3 ] = glGetUniformLocation( circleRenderInfo.shaderProgramId, "projection.top" );
  // rect
  // configure buffers
  // TODO generalize a function to configure the RenderInfo struct like this
  glGenVertexArrays( 1, &rectRenderInfo.vaoId );
  glBindVertexArray( rectRenderInfo.vaoId );
  glGenBuffers( 1, &rectRenderInfo.vboIds[ 0 ] );
  glBindBuffer( GL_ARRAY_BUFFER, rectRenderInfo.vboIds[ 0 ] );
  const int NUM_ATTRIBS = 3;
  std::size_t floatSize = sizeof( GLfloat );
  std::size_t attribTypeSizes[ NUM_ATTRIBS ] = { floatSize, floatSize, floatSize };
  int attribSizes[ NUM_ATTRIBS ] = { 4, 2, 2 };
  std::size_t stride = 0;
  for ( int i = 0; i < NUM_ATTRIBS; ++i ) {
    stride += attribSizes[ i ] * attribTypeSizes[ i ];
  }
  glVertexAttribPointer( 0, attribSizes[ 0 ], GL_FLOAT, GL_FALSE, stride, ( void* )0 );
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 1, attribSizes[ 1 ], GL_FLOAT, GL_FALSE, stride, ( void* )( attribSizes[ 0 ] * attribTypeSizes[ 0 ] ) );
  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 2, attribSizes[ 2 ], GL_FLOAT, GL_FALSE, stride, ( void* )( attribSizes[ 0 ] * attribTypeSizes[ 0 ] + attribSizes[ 1 ] * attribTypeSizes[ 1 ] ) );
  glEnableVertexAttribArray( 2 );
  glBindVertexArray( 0 );
  // create shader program
  rectRenderInfo.shaderProgramId = AssetManager::loadShader( "shaders/DebugRect.glsl" );
  // get shader's constants' locations
  rectRenderInfo.projUnifLoc[ 0 ] = glGetUniformLocation( rectRenderInfo.shaderProgramId, "projection.left" );
  rectRenderInfo.projUnifLoc[ 1 ] = glGetUniformLocation( rectRenderInfo.shaderProgramId, "projection.right" );
  rectRenderInfo.projUnifLoc[ 2 ] = glGetUniformLocation( rectRenderInfo.shaderProgramId, "projection.bottom" );
  rectRenderInfo.projUnifLoc[ 3 ] = glGetUniformLocation( rectRenderInfo.shaderProgramId, "projection.top" );
#endif
}

void Debug::shutdown() {
#ifndef NDEBUG
  // rendering stuff
  glDeleteProgram( circleRenderInfo.shaderProgramId );
  glDeleteVertexArrays( 1, &circleRenderInfo.vaoId );
  glDeleteBuffers( 1, &circleRenderInfo.vboIds[ 0 ] );
  glDeleteProgram( rectRenderInfo.shaderProgramId );
  glDeleteVertexArrays( 1, &rectRenderInfo.vaoId );
  glDeleteBuffers( 1, &rectRenderInfo.vboIds[ 0 ] );
  // logging stuff
  shutdownLogger();
#endif
}

void Debug::drawCircle( Circle circle, Color color ) {
#ifndef NDEBUG
#ifdef DOD
  ASSERT( circle.radius > 0.0f, "Asked to draw a circle of radius %f", circle.radius );
  circleBufferData.push_back( { color, circle.center, circle.radius } );
#elif defined OOP
  ASSERT( circle.getRadius() > 0.0f, "Asked to draw a circle of radius %f", circle.getRadius() );
  circleBufferData.push_back( { color, circle.getCenter(), circle.getRadius() } );
#endif
#else
  UNUSED( circle );
  UNUSED( color );
#endif
}

void Debug::drawRect( Rect rect, Color color ) {
#ifndef NDEBUG
#ifdef DOD
  ASSERT( rect.min.x != rect.max.x && rect.min.y != rect.max.y, "Asked to draw a malformed rectangle ( ( %f, %f ), ( %f, %f ) )", rect.min.x, rect.min.y, rect.max.x, rect.max.y );
  rectBufferData.push_back( { color, rect.min, rect.max } );
#elif defined OOP
  ASSERT( rect.getMin().getX() != rect.getMax().getX() && rect.getMin().getY() != rect.getMax().getY(), "Asked to draw a malformed rectangle ( ( %f, %f ), ( %f, %f ) )", rect.getMin().getX(), rect.getMin().getY(), rect.getMax().getX(), rect.getMax().getY() );
  rectBufferData.push_back( { color, rect.getMin(), rect.getMax() } );
#endif
#else
  UNUSED( rect );
  UNUSED( color );
#endif
}

#ifdef DOD
void Debug::drawShape( Shape shape, Color color ) {
  switch ( shape.type ) {
  case ShapeType::CIRCLE:
    drawCircle( shape.circle, color );
    break;
  case ShapeType::AARECT:
    drawRect( shape.aaRect, color );
    break;
  }
}
#elif defined OOP
void Debug::drawShape( Shape* shape, Color color ) {
  switch ( shape->getType() ) {
  case ShapeType::CIRCLE:
    drawCircle( *static_cast< Circle* >( shape ), color );
    break;
  case ShapeType::AARECT:
    drawRect( *static_cast< Rect* >( shape ), color );
    break;
  }
}
#endif

void Debug::renderAndClear() {
#ifndef NDEBUG
  // configure buffers and render circles
  glUseProgram( circleRenderInfo.shaderProgramId );
  glBindVertexArray( circleRenderInfo.vaoId );
  glBindBuffer( GL_ARRAY_BUFFER, circleRenderInfo.vboIds[ 0 ] );
  glBufferData( GL_ARRAY_BUFFER, sizeof( DebugCircle ) * circleBufferData.size(), circleBufferData.data(), GL_STATIC_DRAW );
  glDrawArrays( GL_POINTS, 0, circleBufferData.size() );
  circleBufferData.clear();
  // render rectangles
  glUseProgram( rectRenderInfo.shaderProgramId );
  glBindVertexArray( rectRenderInfo.vaoId );
  glBindBuffer( GL_ARRAY_BUFFER, rectRenderInfo.vboIds[ 0 ] );
  glBufferData( GL_ARRAY_BUFFER, sizeof( DebugRect ) * rectBufferData.size(), rectBufferData.data(), GL_STATIC_DRAW );
  glDrawArrays( GL_POINTS, 0, rectBufferData.size() );
  rectBufferData.clear();  
#endif
}

void Debug::setOrthoProjection( float aspectRatio, float height ) {
#ifndef NDEBUG
  float halfHeight = height / 2.0f;
  glUseProgram( circleRenderInfo.shaderProgramId );
  glUniform1f( circleRenderInfo.projUnifLoc[ 0 ], -halfHeight * aspectRatio );
  glUniform1f( circleRenderInfo.projUnifLoc[ 1 ], halfHeight * aspectRatio );
  glUniform1f( circleRenderInfo.projUnifLoc[ 2 ], -halfHeight );
  glUniform1f( circleRenderInfo.projUnifLoc[ 3 ], halfHeight );
  glUseProgram( rectRenderInfo.shaderProgramId );
  glUniform1f( rectRenderInfo.projUnifLoc[ 0 ], -halfHeight * aspectRatio );
  glUniform1f( rectRenderInfo.projUnifLoc[ 1 ], halfHeight * aspectRatio );
  glUniform1f( rectRenderInfo.projUnifLoc[ 2 ], -halfHeight );
  glUniform1f( rectRenderInfo.projUnifLoc[ 3 ], halfHeight );
#else
  UNUSED( aspectRatio );
  UNUSED( height );
#endif
}

/////////////////////////////// Profiling ///////////////////////////////////

AutoProfile::AutoProfile( const char* name ) {
#ifdef PROFILING
  Profiler::startProfile( name );
#endif
}

AutoProfile::~AutoProfile() {
#ifdef PROFILING
  Profiler::stopProfile();
#endif
}

std::vector< Profiler::ProfileSample > Profiler::samples;
std::vector< Profiler::SampleNode > Profiler::sampleTree;
Profiler::SampleNodeIndex Profiler::currentNodeInd;
FILE* Profiler::profilerLog;
u32 Profiler::frameNumber;
int Profiler::perfCounters;
const s32 Profiler::PERF_COUNTER_CODES[] = {
  PAPI_L1_TCM, // Level 1 cache misses
  PAPI_L2_TCM, // Level 2 cache misses
  //PAPI_L3_TCM, // Level 3 cache misses
  PAPI_BR_MSP, // Conditional branch instructions mispredicted
};
const char* Profiler::PERF_COUNTER_NAMES[] = { "L1", "L2"/*, "L3"*/, "BRANCH MISP" };

void Profiler::initialize() {
#ifdef PROFILING
  frameNumber = 0;
  // push whatever to index 0 of the lists so the real
  // data starts at index 1
  // TODO standarize indices starting at 1
  samples.push_back( { {}, {}, TimePoint(), 0, 0, 0 } );
  sampleTree.push_back( { 0, 0, 0, nullptr, 0 } );
  currentNodeInd = addChildSampleNode( 0, "ROOT" );
  profilerLog = fopen( PROFILER_LOG_FILE_NAME, "w" );
  // initialize Performance API
  s32 result = PAPI_library_init( PAPI_VER_CURRENT );
  ASSERT( result > 0, "Error initializing PAPI library" ); 
  ASSERT( result == PAPI_VER_CURRENT, "PAPI lib version mismatch (found %d, but required %d)", result, PAPI_VER_CURRENT );
  result = PAPI_is_initialized();
  ASSERT( result == PAPI_LOW_LEVEL_INITED, "Error initializing PAPI library" );
  perfCounters = PAPI_NULL;
  result = PAPI_create_eventset( &perfCounters );
  ASSERT( result == PAPI_OK, "PAPI_create_eventset failed" );
  result = PAPI_add_events( perfCounters, const_cast< s32* >(PERF_COUNTER_CODES), NUM_PERF_COUNTERS );
  const char* errorDesc;
  switch ( result ) {
  case PAPI_EINVAL:
    errorDesc = "One or more of the arguments is invalid"; break;
  case PAPI_ENOMEM:
    errorDesc = "Insufficient memory to complete the operation"; break;
  case PAPI_ENOEVST:
    errorDesc = "The event set specified does not exist"; break;
  case PAPI_EISRUN:
    errorDesc = "The event set is currently counting events"; break;
  case PAPI_ECNFLCT:
    errorDesc = "The underlying counter hardware can not count this event and other events in the event set simultaneously"; break;
  case PAPI_ENOEVNT:
    errorDesc = "The PAPI preset is not available on the underlying hardware"; break;
  case PAPI_EBUG:
    errorDesc = "Internal error"; break;
  default:
    char buffer[ 100 ];
    sprintf( buffer, "%d consecutive elements succeeded before the error", result );
    errorDesc = buffer;
  }
  ASSERT( result == PAPI_OK, "PAPI_add_events failed (%s)", errorDesc );
  result = PAPI_start( perfCounters );
  ASSERT( result == PAPI_OK, "PAPI_start failed" );
  // lock this process to a single core so as to avoid cache misses caused
  // by the OS switching the core in which it is executed
  cpu_set_t cpuSet;
  CPU_ZERO( &cpuSet );
  CPU_SET( sched_getcpu(), &cpuSet );
  result = sched_setaffinity( 0, sizeof( cpuSet ), &cpuSet );
  ASSERT( result == 0, "sched_setaffinity failed" );
#ifdef NDEBUG
  UNUSED( result );
  UNUSED( errorDesc );
#endif
  Debug::write( "Profiler initialized.\n" );
#endif
}

void Profiler::shutdown() {
#ifdef PROFILING
  PAPI_shutdown();
  fclose( profilerLog );
#endif
}

Profiler::SampleNodeIndex Profiler::addChildSampleNode( SampleNodeIndex nodeInd, const char* name ) {
#ifdef PROFILING
  ProfileSample newSample = { {}, {}, TimePoint(), 0, 0, 0 };
  samples.push_back( newSample ); // uninitialized yet
  ProfileSampleIndex dataInd = samples.size() - 1;
  SampleNode newNode = { nodeInd, 0, 0, name, dataInd };
  sampleTree.push_back( newNode );
  // add new node as child of 'node'
  SampleNodeIndex newNodeInd = sampleTree.size() - 1;
  if ( nodeInd != 0 ) {
    SampleNodeIndex firstChild = sampleTree[ nodeInd ].firstChild;
    if ( firstChild == 0 ) {
      sampleTree[ nodeInd ].firstChild = newNodeInd;
    } else {
      // find the end of the children's linked linst and add the new one there
      SampleNodeIndex siblInd = firstChild;
      SampleNode sibling = sampleTree[ siblInd ];
      while ( sibling.nextSibling != 0 ) {
        siblInd = sibling.nextSibling;
        sibling = sampleTree[ siblInd ];
      }
      sampleTree[ siblInd ].nextSibling = newNodeInd;
    }
  }
  return newNodeInd;
#endif
}

Profiler::SampleNodeIndex Profiler::getParentSampleNode( const SampleNodeIndex nodeInd ) {
#ifdef PROFILING
  return sampleTree[ nodeInd ].parent;
#endif
}

Profiler::SampleNodeIndex Profiler::getChildSampleNode( SampleNodeIndex nodeInd, const char* name ) {
#ifdef PROFILING
  SampleNode node = sampleTree[ nodeInd ];
  if ( node.firstChild != 0 ) {
    SampleNodeIndex childInd = node.firstChild;
    SampleNode child;
    do {
      child = sampleTree[ childInd ];
      if ( child.name == name ) {
        return childInd;
      }
      childInd = child.nextSibling;
    } while ( childInd != 0 );
  }
  return addChildSampleNode( nodeInd, name );
#endif
}

void Profiler::startProfile( const char* name ) {
#ifdef PROFILING
  if ( name != sampleTree[ currentNodeInd ].name ) {
    currentNodeInd = getChildSampleNode( currentNodeInd, name );
  }
  callSampleNode( currentNodeInd );
#endif
}

void Profiler::stopProfile() {
#ifdef PROFILING
  if ( returnFromSampleNode( currentNodeInd ) ) {
    currentNodeInd = getParentSampleNode( currentNodeInd );
  } //else this is a recursive function that has not finished
#endif
}

void Profiler::callSampleNode( const SampleNodeIndex nodeInd ) {
#ifdef PROFILING
  ProfileSample sample = samples[ sampleTree[ nodeInd ].dataInd ];
  ++sample.callCount;
  // if this is the first call of a recursive function
  // or if the function isn't recursive
  // we can start measuring time
  if ( sample.recursionCount++ == 0 ) {
    s32 result = PAPI_read( perfCounters, sample.startPerfCounts );
    ASSERT( result == PAPI_OK, "PAPI_read failed" );
#ifdef NDEBUG
    UNUSED( result );
#endif
    sample.startTime = Clock::now();
  }
  samples[ sampleTree[ nodeInd ].dataInd ] = sample;
#endif
}
    
bool Profiler::returnFromSampleNode( const SampleNodeIndex nodeInd ) {
#ifdef PROFILING
  ProfileSample sample = samples[ sampleTree[ nodeInd ].dataInd ];
  // if the function wasn't recursive or ended recursing
  // we can now calculate the elapsed time
  if ( --sample.recursionCount == 0 && sample.callCount != 0 ) {
    TimePoint endTime = Clock::now();
    sample.elapsedNanos += std::chrono::duration_cast< std::chrono::nanoseconds >( endTime - sample.startTime ).count();
    long long endPerfCounts[ NUM_PERF_COUNTERS ];
    s32 result = PAPI_read( perfCounters, endPerfCounts );
    ASSERT( result == PAPI_OK, "PAPI_read failed" );
#ifdef NDEBUG
    UNUSED( result );
#endif
    for ( u8 i = 0; i < NUM_PERF_COUNTERS; ++i ) {
      sample.deltaPerfCounts[ i ] += endPerfCounts[ i ] - sample.startPerfCounts[ i ];
    }
  }
  samples[ sampleTree[ nodeInd ].dataInd ] = sample;
  // also return whether the function is recursing
  return sample.recursionCount == 0;
#endif
}

void Profiler::updateOutputsAndReset() {
#ifdef PROFILING
  // TODO standarize writing to logs and handling their file size
  fprintf( profilerLog, "%d \tCALL COUNT \tACUM INCL \tACUM EXCL \tAVG INCL \tAVG EXCL", frameNumber );
  for ( u8 i = 0; i < NUM_PERF_COUNTERS; ++i ) {
    fprintf( profilerLog, " \t%s", PERF_COUNTER_NAMES[ i ] );
  }
  fprintf( profilerLog, "\n" );
  std::deque< SampleNodeIndex > nodeIndsToProcess;
  // index 0 is null
  nodeIndsToProcess.push_front( 1 );
  std::deque< u32 > depths;
  depths.push_front( 0 );
  while( !nodeIndsToProcess.empty() ) {
    SampleNodeIndex nodeInd = nodeIndsToProcess.front();
    nodeIndsToProcess.pop_front();
    SampleNode node = sampleTree[ nodeInd ];
    u32 depth = depths.front();
    depths.pop_front();
    u32 dataInd = node.dataInd;
    ProfileSample sample = samples[ dataInd ];
    u64 acumExclusiveNanos = sample.elapsedNanos;
    // add all its children to the front of the deque
    SampleNodeIndex childNodeInd = node.firstChild;
    while ( childNodeInd != 0 ) {
      // also calculate exclusive values
      acumExclusiveNanos -= samples[ sampleTree[ childNodeInd ].dataInd ].elapsedNanos;
      nodeIndsToProcess.push_front( childNodeInd );
      depths.push_front( depth + 1 );
      childNodeInd = sampleTree[ childNodeInd ].nextSibling;
    }
    // display sample
    // don't render unused nodes
    if ( sample.callCount > 0 ) {
      // crappy way of expressing call depth
      for ( u32 i = 0; i < depth; ++i ) {
        fprintf( profilerLog, "-- " );
      }
      fprintf( profilerLog, "%s\t%d\t%ld\t%ld\t%ld\t%ld", node.name, sample.callCount,
               sample.elapsedNanos, acumExclusiveNanos, sample.elapsedNanos / sample.callCount, acumExclusiveNanos / sample.callCount );
      for ( u8 i = 0; i < NUM_PERF_COUNTERS; ++i ) {
        fprintf( profilerLog, "\t%lld", sample.deltaPerfCounts[ i ] );
      }
      fprintf( profilerLog, "\n" );
    }
    // reset counters
    samples[ dataInd ].callCount = 0;
    samples[ dataInd ].elapsedNanos = 0;
    samples[ dataInd ].recursionCount = 0;
    for ( u8 i = 0; i < NUM_PERF_COUNTERS; ++i ) {
      samples[ dataInd ].deltaPerfCounts[ i ] = 0;
    }
  }
  ++frameNumber;
  int result = PAPI_reset( perfCounters );
  ASSERT( result == PAPI_OK, "PAPI_reset failed" );
#ifdef NDEBUG
  UNUSED( result );
#endif
#endif
}
