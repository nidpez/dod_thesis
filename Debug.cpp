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

AutoProfile::AutoProfile( const char* name ) {
#ifndef NDEBUG
  this->name = name;
#endif
}

AutoProfile::~AutoProfile() {
#ifndef NDEBUG
#endif
}

std::vector< Profiler::ProfileSample > Profiler::samples;
std::vector< Profiler::SampleNode > Profiler::sampleTree;
Profiler::SampleNode* Profiler::currentNode;
FILE* Profiler::profilerLog;
u32 Profiler::frameNumber;

void Profiler::initialize() {
#ifndef NDEBUG
  frameNumber = 0;
  currentNode = addChildSampleNode( nullptr, "ROOT" );
  profilerLog = fopen( PROFILER_LOG_FILE_NAME, "w" );
  Debug::write( "Profiler initialized.\n" );
#endif
}

void Profiler::shutdown() {
#ifndef NDEBUG
  fclose( profilerLog );
#endif
}

SampleNode* Profiler::addChildSampleNode( const SampleNode* node, const char* name ) {
#ifndef NDEBUG
  samples.push_back( { 0, 0, 1, 0 } ); // uninitialized yet
  ProfileSample* data = &samples.back();
  sampleTree.push_back( { node, nullptr, nullptr, name, data } );
  return &sampleTree.back();
#endif
}

SampleNode* Profiler::getParentSampleNode( const SampleNode* node ) {
#ifndef NDEBUG
  return node->parent;
#endif
}

SampleNode* Profiler::getChildSampleNode( const SampleNode* node, const char* name ) {
#ifndef NDEBUG
  if ( node->firstChild != nullptr ) {
    SampleNode* child = node->firstChild;
    do {
      // TODO check effectivity of pointer comparison of sample node names
      if ( child->name == name ) {
        return child;
      }
      child = child->nextSibling;
    } while ( child != nullptr );
  }
  return addChildSampleNode( node, name );
#endif
}

void Profiler::startProfile( const char* name ) {
#ifndef NDEBUG
  if ( name != currentNode->name ) {
    currentNode = getChildSampleNode( currentNode, name );
  }
  callSampleNode( currentNode );
#endif
}

void Profiler::stopProfile() {
#ifndef NDEBUG
  if ( returnFromSampleNode( currentNode ) ) {
    currentNode = getParentSampleNode( currentNode );
  } //else this is a recursive function that has not finished
#endif
}

void Profiler::callSampleNode( const SampleNode* node ) {
#ifndef NDEBUG
  ProfileSample sample = *node->data;
  ++sample.callCount;
  // if this is the first call of a recursive function
  // or if the function isn't recursive
  // we can start measuring time
  if ( sample.recursionCount++ == 0 ) {
    sample.startTime = Clock::now();
  }
  *node->data = sample;
#endif
}
    
bool Profiler::returnFromSampleNode( const SampleNode* node ) {
#ifndef NDEBUG
  ProfileSample sample = *node->data;
  // if the function wasn't recursive or ended recursing
  // we can now calculate the elapsed time
  if ( --sample.recursionCount == 0 && sample.callCount != 0 ) {
    TimePoint endTime = Clock::now();
    sample.elapsedNanos += std::chrono::duration_cast< std::chrono::nanoseconds >( endTime - startTime ).count();
  }
  *node->data = sample;
  // also return whether the function is recursing
  return sample.recursionCount == 0;
#endif
}

void Profiler::updateOutputsAndReset() {
#ifndef NDEBUG
  fprintf( profilerLog, "%d\n", frameNumber++ );
  // TODO re-implement
#endif
}
