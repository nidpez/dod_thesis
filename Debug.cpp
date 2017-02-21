#include "Debug.hpp"

#include <ctime>
#include <cstdarg>

#include "Asset.hpp"

/////////////////////// Error handling and logging //////////////////////

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
  if ( length <= 0 ) { // just to use the length param
    message = "Error reporting the error!";
  }
  if ( userParam != nullptr ) { // just to use userParam 
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

////////////////////////// Drawing debug shapes ///////////////////////////

RenderInfo DebugRenderer::renderInfo;
std::vector< DebugRenderer::DebugCircle > DebugRenderer::circleBufferData;

void DebugRenderer::initialize() {
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
  s32 error = createShaderProgram( &renderInfo.shaderProgramId,
			       "shaders/DebugShape.vert", "shaders/DebugShape.frag",
			       "shaders/DebugShape.geom" );
  if ( error ) {
    // TODO maybe hardcode a default shader here
  }
  // get shader's constants' locations
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

void DebugRenderer::drawCircle( Circle circle, Color color ) {
#ifndef NDEBUG
  ASSERT( circle.radius > 0.0f, "Asked to draw a circle of radius %f", circle.radius );
  circleBufferData.push_back( { color, circle.center, circle.radius } );
#endif
}

void DebugRenderer::renderAndClear() {
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

