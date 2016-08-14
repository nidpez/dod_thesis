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

const float PIXELS_PER_UNIT = 4.0f;

int loadShaderSourceFile( const char* name, char** source );

int compileShader( const char* name, const GLenum type, GLuint* shaderId );

int createShaderProgram( const GLuint vertShaderId, const GLuint geomShaderId, const GLuint fragShaderId, GLuint* shaderProgramId );

void printGlfwError( int error, const char* description );

void APIENTRY glDebugCallback( GLenum source, GLenum type, GLuint id, GLenum severity,
			       GLsizei length, const GLchar* message, const void* userParam );

void printShaderError( GLuint shaderId, const char* shaderName );

int main() {
  glfwSetErrorCallback( printGlfwError );
  
  if ( !glfwInit() ) {
    //TODO handle errors
    printf( "GLFW failed to initialize!\n" );
    return 1;
  }
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
  GLFWwindow* window = glfwCreateWindow( mode->width, mode->height , "Space Adventure (working title)", monitor, nullptr );
  if ( !window ) {
    printf( "Error creating the window!\n" );
    return 1;
  }
  glfwMakeContextCurrent( window );
  /*NOTE: glfwSwapInterval( 1 ) causes an error on the dev machine where terminating
    the program leaves the last rendered frame there covering everything.
    TODO: investigate glfwSwapInterval bug
   */
  glfwSwapInterval( 0 );
  printf( "Window created.\nOpenGL version %d.%d used.\n", glfwGetWindowAttrib( window, GLFW_CONTEXT_VERSION_MAJOR ), glfwGetWindowAttrib( window, GLFW_CONTEXT_VERSION_MINOR ) );

  glewExperimental = GL_TRUE;
  GLenum glewError = glewInit();
  if ( glewError != GLEW_OK ) {
    printf( "GLEW failed to initialize! %s\n", glewGetErrorString( glewError ) );
    return 1;
  }
  if ( !GLEW_VERSION_3_3 ) {
    //is this even possible when glfw already gave us a 3.3 context?
    printf( "Required OpenGL version 3.3 unavailable, says GLEW!\n" );
    return 1;
  }
  printf( "OpenGL functionality successfully loaded.\n" );

  //setup opengl debugging
#ifndef NDEBUG
  if ( GLEW_KHR_debug ) {
    printf( "Core KHR Debug extension found.\n" );
    glDebugMessageCallback( glDebugCallback, nullptr );
    glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
  } else {
    printf( "Core KHR Debug extension unavailable!\n" );
  }
#endif
  //basic gl configurations
  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
  
  //quad geometry of unit width
  //TODO create geometry that closely fits a sprite's image (maybe?)
  GLfloat positions[] = {
    -0.5f,	-0.5f,
    0.5f,	-0.5f,
    0.5f,	 0.5f,
    -0.5f,	 0.5f };

  //configure buffers
  //create buffers for a single sprite, for now
  GLuint vaoId;
  glGenVertexArrays( 1, &vaoId );
  glBindVertexArray( vaoId );
  GLuint posVboId;
  glGenBuffers( 1, &posVboId );
  glBindBuffer( GL_ARRAY_BUFFER, posVboId );
  glBufferData( GL_ARRAY_BUFFER, sizeof( positions ), positions, GL_STATIC_DRAW );
  glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, ( void* )0 );
  glEnableVertexAttribArray( 0 );
  GLuint uvVboId;
  glGenBuffers( 1, &uvVboId );
  glBindBuffer( GL_ARRAY_BUFFER, uvVboId );
  //glBufferData is different for every sprite
  glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, ( void* )0 );
  glEnableVertexAttribArray( 1 );
  glBindVertexArray( 0 );

  //create shader program
  GLuint vertexShaderId;
  int error = compileShader( "shaders/SpriteUnlit.vert", GL_VERTEX_SHADER, &vertexShaderId );
  if ( error ) {
    return error;
  } 
  GLuint fragmentShaderId;  
  error = compileShader( "shaders/SpriteUnlit.frag", GL_FRAGMENT_SHADER, &fragmentShaderId );
  if ( error ) {
    return error;
  }   
  GLuint shaderProgramId = glCreateProgram();
  glAttachShader( shaderProgramId, vertexShaderId );
  glAttachShader( shaderProgramId, fragmentShaderId );
  glLinkProgram( shaderProgramId );
  GLint linked = 0;
  glGetProgramiv( shaderProgramId, GL_LINK_STATUS, ( int * )&linked );
  if( linked == GL_FALSE ) {
    GLint maxLength;
    glGetProgramiv( shaderProgramId, GL_INFO_LOG_LENGTH, &maxLength );
    GLchar *errorLog = ( GLchar * )malloc( sizeof( GLchar )*maxLength );
    glGetProgramInfoLog( shaderProgramId, maxLength, &maxLength, errorLog );
    printf( "Shader Program error:\n\t%s\n", errorLog );
    free( errorLog );
    glDeleteProgram( shaderProgramId );
    glDeleteShader( vertexShaderId );
    glDeleteShader( fragmentShaderId );
    return 1;
  }  
  glUseProgram( shaderProgramId );
  glDetachShader( shaderProgramId, vertexShaderId );
  glDetachShader( shaderProgramId, fragmentShaderId );
  glDeleteShader( vertexShaderId );
  glDeleteShader( fragmentShaderId );

  //get shader's constants' locations
  const GLint
    projLeftUnifLoc = glGetUniformLocation( shaderProgramId, "projection.left" );
  const GLint
    projRightUnifLoc = glGetUniformLocation( shaderProgramId, "projection.right" );
  const GLint
    projBottomUnifLoc = glGetUniformLocation( shaderProgramId, "projection.bottom" );
  const GLint
    projTopUnifLoc = glGetUniformLocation( shaderProgramId, "projection.top" );
  const GLint
    entityPositionUnifLoc = glGetUniformLocation( shaderProgramId, "entityPosition" );
  const GLint
    entityOrientationUnifLoc = glGetUniformLocation( shaderProgramId, "entityOrientation" );
  const GLint
    entityScaleUnifLoc = glGetUniformLocation( shaderProgramId, "entityScale" );
  
  //load textures
  const char* tex1FileName = "astronaut.png";
  GLuint tex1Id;
  glGenTextures( 1, &tex1Id );
  glBindTexture( GL_TEXTURE_2D, tex1Id );
  int w1, h1;
  unsigned char* texData1 = SOIL_load_image( tex1FileName, &w1, &h1, nullptr, SOIL_LOAD_RGBA );
  if ( texData1 == 0 ) {
    printf( "Error loading texture %s: %s\n", tex1FileName, SOIL_last_result() );
    return 1;
  }
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w1, h1, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData1 );
  SOIL_free_image_data( texData1 );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  
  const char* tex2FileName = "planetSurface.png";
  GLuint tex2Id;
  glGenTextures( 1, &tex2Id );
  glBindTexture( GL_TEXTURE_2D, tex2Id );
  int w2, h2;
  unsigned char* texData2 = SOIL_load_image( tex2FileName, &w2, &h2, nullptr, SOIL_LOAD_RGBA );
  if ( texData2 == 0 ) {
    printf( "Error loading texture %s: %s\n", tex2FileName, SOIL_last_result() );
    return 1;
  }
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w2, h2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData2 );
  SOIL_free_image_data( texData2 );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

  //configure viewport and orthographic projection
  glViewport( 0, 0, mode->width, mode->height );
  float aspect = mode->width / ( float )mode->height;
  glUniform1f( projLeftUnifLoc, -50 * aspect );
  glUniform1f( projRightUnifLoc, 50 * aspect );
  glUniform1f( projBottomUnifLoc, -50 );
  glUniform1f( projTopUnifLoc, 50 );
  
  //debug circle geometry
  GLfloat circlePositions[] = {
    //position		//radius	//color
    30.0f, 30.0f,	50.0f,		0.0f, 1.0f, 0.0f, 1.0f,
    45.0f, 30.0f,	20.0f,		0.0f, 1.0f, 0.0f, 0.7f };
  
  //configure buffers
  GLuint circlesVaoId;
  glGenVertexArrays( 1, &circlesVaoId );
  glBindVertexArray( circlesVaoId );
  GLuint circlesVboId;
  glGenBuffers( 1, &circlesVboId );
  glBindBuffer( GL_ARRAY_BUFFER, circlesVboId );
  glBufferData( GL_ARRAY_BUFFER, sizeof( circlePositions ), circlePositions, GL_STATIC_DRAW );
  glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )0 );
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 1, 1, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )( 2 * sizeof( GLfloat ) ) );
  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, 7 * sizeof( GLfloat ), ( void* )( 3 * sizeof( GLfloat ) ) );
  glEnableVertexAttribArray( 2 );
  glBindVertexArray( 0 );

  //create shader program
  GLuint circlesVertexShaderId;
  error = compileShader( "shaders/DebugShape.vert", GL_VERTEX_SHADER, &circlesVertexShaderId );
  if ( error ) {
    return error;
  }  
  GLuint geometryShaderId;
  error = compileShader( "shaders/DebugShape.geom", GL_GEOMETRY_SHADER, &geometryShaderId );
  if ( error ) {
    return error;
  }
  GLuint circlesFragmentShaderId;
  error = compileShader( "shaders/DebugShape.frag", GL_FRAGMENT_SHADER, &circlesFragmentShaderId );
  if ( error ) {
    return error;
  }  
  GLuint circlesShaderProgramId;
  error = createShaderProgram( circlesVertexShaderId, geometryShaderId, circlesFragmentShaderId, &circlesShaderProgramId );
  if ( error ) {
    return error;
  }

  //get shader's constants' locations
  const GLint
    circlesProjLeftUnifLoc = glGetUniformLocation( circlesShaderProgramId, "projection.left" );
  const GLint
    circlesProjRightUnifLoc = glGetUniformLocation( circlesShaderProgramId, "projection.right" );
  const GLint
    circlesProjBottomUnifLoc = glGetUniformLocation( circlesShaderProgramId, "projection.bottom" );
  const GLint
    circlesProjTopUnifLoc = glGetUniformLocation( circlesShaderProgramId, "projection.top" );
  /*const GLint
    cirlcesEntityScaleUnifLoc = glGetUniformLocation( circlesShaderProgramId, "entityScale" );*/

  //configure viewport and orthographic projection
  glUniform1f( circlesProjLeftUnifLoc, -50 * aspect );
  glUniform1f( circlesProjRightUnifLoc, 50 * aspect );
  glUniform1f( circlesProjBottomUnifLoc, -50 );
  glUniform1f( circlesProjTopUnifLoc, 50 );

  //configure keyboard input
  //TODO use key callback instead
  glfwSetInputMode( window, GLFW_STICKY_KEYS, 1 );

  //astronaut transform
  float pos1[] = { -30.0f, 30.0f };
  float angle = 0.0f;
  float frameW1 = w1 / 5.0f;
  const float BASE_SCALE[] = { frameW1 / PIXELS_PER_UNIT, h1 / PIXELS_PER_UNIT };
  float scale[] = { 1.0f, 1.0f };
  float speed = 20.0f;
  //bounding geometry
  float r1 = BASE_SCALE[ 0 ] / 2.0f;
  //uv coords
  //use inverted Y coords to work with SOIL
  GLfloat texCoords1DefaultX[] = {
    0.0f,		1.0f,
    1.0f / 5.0f,	1.0f,
    1.0f / 5.0f,	0.0f,
    0.0f,		0.0f };
  GLfloat texCoords1FlippedX[] = {
    1.0f / 5.0f,	1.0f,
    0.0f,		1.0f,
    0.0f,		0.0f,
    1.0f / 5.0f,	0.0f };
  GLfloat texCoords1[ 8 ];
  for ( int i = 0; i < 8; ++i ) {
    texCoords1[ i ] = texCoords1DefaultX[ i ];
  }

  //planet transform
  float pos2[] = { 0.0f, 0.0f };
  //bounding geometry
  float r2 = ( w2 / 2.0f ) / PIXELS_PER_UNIT;
  //uv coords
  //use inverted Y coords to work with SOIL
  GLfloat texCoords2[] = {
    0.0f,	1.0f,
    1.0f,	1.0f,
    1.0f,	0.0f,
    0.0f,	0.0f };

  //synch debug shapes with objects they are meant to help debug
  glBindVertexArray( circlesVaoId );
  circlePositions[ 0 ] = pos1[ 0 ];
  circlePositions[ 1 ] = pos1[ 1 ];
  circlePositions[ 2 ] = r1;
  circlePositions[ 7 ] = pos2[ 0 ];
  circlePositions[ 8 ] = pos2[ 1 ];
  circlePositions[ 9 ] = r2;
  glBindBuffer( GL_ARRAY_BUFFER, circlesVboId );
  glBufferData( GL_ARRAY_BUFFER, sizeof( circlePositions ), circlePositions, GL_STATIC_DRAW );
  
  //main loop      
  float tempPos[ 2 ];
  tempPos[ 0 ] = pos1[ 0 ];
  tempPos[ 1 ] = pos1[ 1 ];
  float angle2 = 0.0f;
  double t1 = glfwGetTime();
  double t2;
  double deltaT = 0.0;
  while ( !glfwWindowShouldClose( window ) ) {
    //process input
    glfwPollEvents();
    if ( glfwGetKey( window, GLFW_KEY_ESCAPE ) == GLFW_PRESS ) {
      glfwSetWindowShouldClose( window, true );
    }
    //move with WASD
    angle = 0.0f;
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
      if ( dir[ 0 ] < 0 ) {	
	for ( int i = 0; i < 8; ++i ) {
	  texCoords1[ i ] = texCoords1FlippedX[ i ];
	}
      } else {
	for ( int i = 0; i < 8; ++i ) {
	  texCoords1[ i ] = texCoords1DefaultX[ i ];
	}
      }
      //update position
      //TODO calculate new deltaT or keep using last frame's?
      tempPos[ 0 ] += dir[ 0 ] * speed * deltaT;
      tempPos[ 1 ] += dir[ 1 ] * speed * deltaT;
      //update orientation
      if ( dir[ 0 ] < 0 ) {
	angle = atan2( -dir[ 1 ], -dir[ 0 ] );
      } else {
	angle = atan2( dir[ 1 ], dir[ 0 ] );
      }
    }

    //detect collisions
    float dx = tempPos[ 0 ] - pos2[ 0 ];
    float dy = tempPos[ 1 ] - pos2[ 1 ];
    float dist = sqrt( dx * dx + dy * dy );
    if ( dist >= r1 + r2 ) {
      //no collision detected
      pos1[ 0 ] = tempPos[ 0 ];
      pos1[ 1 ] = tempPos[ 1 ];
    } else {
      tempPos[ 0 ] = pos1[ 0 ];
      tempPos[ 1 ] = pos1[ 1 ];
    }
    
    //render scene
    glClear( GL_COLOR_BUFFER_BIT );
  
    glUseProgram( shaderProgramId );
    glBindVertexArray( vaoId );
  
    //test quad 1 (astronaut)
    glBindBuffer( GL_ARRAY_BUFFER, uvVboId );
    glBufferData( GL_ARRAY_BUFFER, sizeof( texCoords1 ), texCoords1, GL_STATIC_DRAW );
    glBindTexture( GL_TEXTURE_2D, tex1Id );
    glUniform2f( entityPositionUnifLoc, pos1[ 0 ], pos1[ 1 ] );
    glUniform1f( entityOrientationUnifLoc, angle );
    glUniform2f( entityScaleUnifLoc, BASE_SCALE[ 0 ] * scale[ 0 ],
		 BASE_SCALE[ 1 ] * scale[ 1 ] );
    glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
  
    //the test quad 2
    glBindBuffer( GL_ARRAY_BUFFER, uvVboId );
    glBufferData( GL_ARRAY_BUFFER, sizeof( texCoords2 ), texCoords2, GL_STATIC_DRAW );
    glBindTexture( GL_TEXTURE_2D, tex2Id );
    glUniform2f( entityPositionUnifLoc, pos2[ 0 ], pos2[ 1 ] );
    glUniform1f( entityOrientationUnifLoc, angle2-=0.005f );
    glUniform2f( entityScaleUnifLoc, w2 / PIXELS_PER_UNIT, h2 / PIXELS_PER_UNIT );    
    glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );

    //TODO render debug shapes
    glUseProgram( circlesShaderProgramId );
    glBindVertexArray( circlesVaoId );
    circlePositions[ 0 ] = pos1[ 0 ];
    circlePositions[ 1 ] = pos1[ 1 ];
    circlePositions[ 7 ] = pos2[ 0 ];
    circlePositions[ 8 ] = pos2[ 1 ];
    glBindBuffer( GL_ARRAY_BUFFER, circlesVboId );
    glBufferData( GL_ARRAY_BUFFER, sizeof( circlePositions ), circlePositions, GL_STATIC_DRAW );
    glDrawArrays( GL_POINTS, 0, 2 );
    
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
    //printf( "fps = %f\n", 1.0f / deltaT );
  }
  printf( "Main loop exited.\n" );
  
  //free OpenGL resources
  glUseProgram( 0 );
  glDeleteProgram( shaderProgramId );
  glDeleteVertexArrays( 1, &vaoId );
  glDeleteBuffers( 1, &posVboId );
  glDeleteBuffers( 1, &uvVboId );
  glDeleteTextures( 1, &tex1Id );
  glDeleteTextures( 1, &tex2Id );
  glDeleteProgram( circlesShaderProgramId );
  glDeleteVertexArrays( 1, &circlesVaoId );
  glDeleteBuffers( 1, &circlesVboId );
  printf( "Resources freed.\n" );

  glfwDestroyWindow( window );
  glfwTerminate();
  
  return 0;
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
  
#ifndef NDEBUG
  GLint compiled; 
  glGetShaderiv( *shaderId, GL_COMPILE_STATUS, &compiled );
  if ( compiled == GL_FALSE ) {
    printShaderError( *shaderId, ( char* )"Vertex Shader DebugShape" );
    printf( "Shader source:\n\"%s\"\n", source );
    glDeleteShader( *shaderId );
    return 1;
    //TODO on shader error replace with default ugly shader instead of quitting
  }
#endif
  delete[] source;
  return 0;
}

int createShaderProgram( const GLuint vertShaderId, const GLuint geomShaderId, const GLuint fragShaderId, GLuint* shaderProgramId ) {
  *shaderProgramId  = glCreateProgram();
  glAttachShader( *shaderProgramId, vertShaderId );
  glAttachShader( *shaderProgramId, geomShaderId );
  glAttachShader( *shaderProgramId, fragShaderId );
  glLinkProgram( *shaderProgramId );
  GLint linked = 0;
  glGetProgramiv( *shaderProgramId, GL_LINK_STATUS, ( int * )&linked );
  if( linked == GL_FALSE ) {
    GLint maxLength;
    glGetProgramiv( *shaderProgramId, GL_INFO_LOG_LENGTH, &maxLength );
    GLchar *errorLog = ( GLchar * )malloc( sizeof( GLchar )*maxLength );
    glGetProgramInfoLog( *shaderProgramId, maxLength, &maxLength, errorLog );
    printf( "Shader Program error:\n\t%s\n", errorLog );
    free( errorLog );
    glDeleteProgram( *shaderProgramId );
    glDeleteShader( vertShaderId );
    glDeleteShader( geomShaderId );
    glDeleteShader( fragShaderId );
    return 1;
  }  
  glUseProgram( *shaderProgramId );
  glDetachShader( *shaderProgramId, vertShaderId );
  glDetachShader( *shaderProgramId, geomShaderId );
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
  printf( "GLFW error %s ocurred:\n\t%s\n", errorName, description );
}

void APIENTRY glDebugCallback( GLenum source, GLenum type, GLuint id, GLenum severity,
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
    severityStr = ( char* )"Notification";
    break;
  default :
    severityStr = ( char* )"Undefined";
  }
  if ( length <= 0 ) { //just to use the length param
    message = "Error reporting the error";
  }
  if ( userParam != nullptr ) { //just to use userParam 
    userParamStr = ( char* )userParam;
  } else {
    userParamStr = ( char* )"Undefined";
  }
  printf( "OpenGL debug message "
	  "(Src: %s, Type: %s, Severity: %s, ID: %d, Extra: %s):"
	  "\n\t%s\n",
	  sourceStr, typeStr, severityStr, id, userParamStr, message );
}

void printShaderError( GLuint shaderId, const char* shaderName ) {
  GLint maxLength;
  glGetShaderiv( shaderId, GL_INFO_LOG_LENGTH, &maxLength );
  GLchar* errorLog = ( GLchar* )malloc( sizeof( GLchar ) * maxLength );
  glGetShaderInfoLog( shaderId, maxLength, &maxLength, errorLog );
  printf( "Shader error in %s:\n\t%s\n", shaderName, errorLog );
  free( errorLog );
}
