#include "EngineCommon.hpp"

#include <ctime>
#include <cstdarg>

/////////////////////// Error handling and logging //////////////////////

FILE* Debug::log;
RenderInfo Debug::renderInfo;
std::vector< Debug::DebugCircle > Debug::circleBufferData;

void Debug::initializeLogger() {
#ifndef NDEBUG
  //logging stuff
  log = fopen( LOG_FILE_NAME, "a" );
  write( "Logging system initialized.\n" );
#endif
}

void Debug::initializeRenderer() {
#ifndef NDEBUG
  // configure buffers
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
  // create shader program
  renderInfo.shaderProgramId = AssetManager::loadShader( "shaders/DebugShape.glsl" );
  // get shader's constants' locations
  renderInfo.projUnifLoc[ 0 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.left" );
  renderInfo.projUnifLoc[ 1 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.right" );
  renderInfo.projUnifLoc[ 2 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.bottom" );
  renderInfo.projUnifLoc[ 3 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.top" );
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
  Debug::writeError( "%s\tAssertion \"%s\" failed\n\tat %s,\n\t\t%s, line %d:\n\t\t",
		      date, failedCond, file, function, line ); 
  va_list args;
  va_start( args, line );
  char* msgFormat = va_arg( args, char* );
  Debug::writeError( msgFormat, args );
  va_end( args );
  Debug::writeError( "\n" );
  Debug::shutdown();
  std::abort();
}

////////////////////////// Drawing debug shapes ///////////////////////////

void Debug::shutdown() {
#ifndef NDEBUG
  // rendering stuff
  glDeleteProgram( renderInfo.shaderProgramId );
  glDeleteVertexArrays( 1, &renderInfo.vaoId );
  glDeleteBuffers( 1, &renderInfo.vboIds[ 0 ] );
  // logging stuff
  fclose( log );
#endif
}

void Debug::drawCircle( Circle circle, Color color ) {
#ifndef NDEBUG
  ASSERT( circle.radius > 0.0f, "Asked to draw a circle of radius %f", circle.radius );
  circleBufferData.push_back( { color, circle.center, circle.radius } );
#endif
}

void Debug::renderAndClear() {
#ifndef NDEBUG
  // configure buffers
  glUseProgram( renderInfo.shaderProgramId );
  glBindVertexArray( renderInfo.vaoId );
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glBufferData( GL_ARRAY_BUFFER, sizeof( DebugCircle ) * circleBufferData.size(), circleBufferData.data(), GL_STATIC_DRAW );
  glDrawArrays( GL_POINTS, 0, circleBufferData.size() );
  circleBufferData.clear();
#endif
}

void Debug::setOrthoProjection( float aspectRatio, float height ) {
#ifndef NDEBUG
  float halfHeight = height / 2.0f;
  glUseProgram( renderInfo.shaderProgramId );
  glUniform1f( renderInfo.projUnifLoc[ 0 ], -halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 1 ], halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 2 ], -halfHeight );
  glUniform1f( renderInfo.projUnifLoc[ 3 ], halfHeight );
#endif
}

/////////////////////////////// Profiling ///////////////////////////////////

// TODO use a macro other than NDEBUG for profiling

AutoProfile::AutoProfile( const char* name ) {
#ifndef NDEBUG
  Profiler::startProfile( name );
#endif
}

AutoProfile::~AutoProfile() {
#ifndef NDEBUG
  Profiler::stopProfile();
#endif
}

std::vector< Profiler::ProfileSample > Profiler::samples;
std::vector< Profiler::SampleNode > Profiler::sampleTree;
Profiler::SampleNodeIndex Profiler::currentNodeInd;
FILE* Profiler::profilerLog;
u32 Profiler::frameNumber;

void Profiler::initialize() {
#ifndef NDEBUG
  frameNumber = 0;
  // push whatever to index 0 of the lists so the real
  // data starts at index 1
  // TODO standarize indices starting at 1
  samples.push_back( { TimePoint(), 0, 0, 0 } );
  sampleTree.push_back( { 0, 0, 0, nullptr, 0 } );
  currentNodeInd = addChildSampleNode( 0, "ROOT" );
  profilerLog = fopen( PROFILER_LOG_FILE_NAME, "w" );
  // TODO standarize writing to logs and handling their file size
  fprintf( profilerLog, "FUNCTION \tCALL COUNT \tACUM INCL \tACUM EXCL \tAVG INCL \tAVG EXCL \n" );
  Debug::write( "Profiler initialized.\n" );
#endif
}

void Profiler::shutdown() {
#ifndef NDEBUG
  fclose( profilerLog );
#endif
}

Profiler::SampleNodeIndex Profiler::addChildSampleNode( SampleNodeIndex nodeInd, const char* name ) {
#ifndef NDEBUG
  ProfileSample newSample = { TimePoint(), 0, 0, 0 };
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
#ifndef NDEBUG
  return sampleTree[ nodeInd ].parent;
#endif
}

Profiler::SampleNodeIndex Profiler::getChildSampleNode( SampleNodeIndex nodeInd, const char* name ) {
#ifndef NDEBUG
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
#ifndef NDEBUG
  if ( name != sampleTree[ currentNodeInd ].name ) {
    currentNodeInd = getChildSampleNode( currentNodeInd, name );
  }
  callSampleNode( currentNodeInd );
#endif
}

void Profiler::stopProfile() {
#ifndef NDEBUG
  if ( returnFromSampleNode( currentNodeInd ) ) {
    currentNodeInd = getParentSampleNode( currentNodeInd );
  } //else this is a recursive function that has not finished
#endif
}

void Profiler::callSampleNode( const SampleNodeIndex nodeInd ) {
#ifndef NDEBUG
  ProfileSample sample = samples[ sampleTree[ nodeInd ].dataInd ];
  ++sample.callCount;
  // if this is the first call of a recursive function
  // or if the function isn't recursive
  // we can start measuring time
  if ( sample.recursionCount++ == 0 ) {
    sample.startTime = Clock::now();
  }
  samples[ sampleTree[ nodeInd ].dataInd ] = sample;
#endif
}
    
bool Profiler::returnFromSampleNode( const SampleNodeIndex nodeInd ) {
#ifndef NDEBUG
  ProfileSample sample = samples[ sampleTree[ nodeInd ].dataInd ];
  // if the function wasn't recursive or ended recursing
  // we can now calculate the elapsed time
  if ( --sample.recursionCount == 0 && sample.callCount != 0 ) {
    TimePoint endTime = Clock::now();
    sample.elapsedNanos += std::chrono::duration_cast< std::chrono::nanoseconds >( endTime - sample.startTime ).count();
  }
  samples[ sampleTree[ nodeInd ].dataInd ] = sample;
  // also return whether the function is recursing
  return sample.recursionCount == 0;
#endif
}

void Profiler::updateOutputsAndReset() {
#ifndef NDEBUG
  fprintf( profilerLog, "%d\n", frameNumber );
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
    // add all its children to the front of the deque
    SampleNodeIndex childNodeInd = node.firstChild;
    u64 acumChildNanos = 0;
    while ( childNodeInd != 0 ) {
      // also calculate exclusive execution time
      acumChildNanos += samples[ sampleTree[ childNodeInd ].dataInd ].elapsedNanos;
      nodeIndsToProcess.push_front( childNodeInd );
      depths.push_front( depth + 1 );
      childNodeInd = sampleTree[ childNodeInd ].nextSibling;
    }
    // display sample
    u32 dataInd = node.dataInd;
    ProfileSample sample = samples[ dataInd ];
    // crappy way of expressing call depth
    for ( u32 i = 0; i < depth; ++i ) {
      fprintf( profilerLog, "-- " );
    }
    if ( nodeInd != 1 ) {       // don't render the ROOT node
      u64 acumExclusiveNanos = sample.elapsedNanos - acumChildNanos;
      fprintf( profilerLog, "%s\t%d\t%ld\t%ld\t%ld\t%ld\n", node.name, sample.callCount, sample.elapsedNanos, acumExclusiveNanos, sample.elapsedNanos / sample.callCount, acumExclusiveNanos / sample.callCount );
    }
    // reset counters
    samples[ dataInd ].callCount = 0;
    samples[ dataInd ].elapsedNanos = 0;
    samples[ dataInd ].recursionCount = 0;
  }  
  ++frameNumber;
#endif
}
