#include <vector>
#include <random>

#include "EngineCommon.hpp"
#include "Math.hpp"
#include "Debug.hpp"
#include "Asset.hpp"
#include "CompManagers.hpp"

#include "TransformTest.hpp"

// misc
GLFWwindow* createWindowAndGlContext( const char* const windowTitle );

s32 main() {
  // initialize managers
  Logger::initialize();
  GLFWwindow* window = createWindowAndGlContext( "Space Adventure (working title)" );
  EntityManager::initialize();
  TransformManager::initialize();
  CircleColliderManager::initialize();
  SpriteManager::initialize();
  // AnimationManager animationManager;
  AssetManager::initialize();
  DebugRenderer::initialize();
  
  // configure viewport and orthographic projection
  // TODO put projection info in a Camera component
  s32 windowWidth, windowHeight;
  glfwGetWindowSize( window, &windowWidth, &windowHeight );
  glViewport( 0, 0, windowWidth, windowHeight );
  float aspect = windowWidth / ( float )windowHeight;

  SpriteManager::setOrthoProjection( aspect, 100 );
  DebugRenderer::setOrthoProjection( aspect, 100 );

  TransformTest::initialize( aspect );
  
  // main loop      
  double t1 = glfwGetTime();
  double t2;
  double deltaT = 0.0;
  Logger::write( "About to enter main loop.\n" );
  while ( !glfwWindowShouldClose( window ) ) {
    // process input
    glfwPollEvents();
    if ( glfwGetKey( window, GLFW_KEY_ESCAPE ) == GLFW_PRESS ) {
      glfwSetWindowShouldClose( window, true );
    }
    // move with WASD
    // angle = 0.0f;
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
      // normalize direction vector
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
      // update position
      // TODO calculate new deltaT or keep using last frame's?
      // tempPos[ 0 ] += dir[ 0 ] * speed * deltaT;
      // tempPos[ 1 ] += dir[ 1 ] * speed * deltaT;
      // // update orientation
      // if ( dir[ 0 ] < 0 ) {
      // 	angle = atan2( -dir[ 1 ], -dir[ 0 ] );
      // } else {
      // 	angle = atan2( dir[ 1 ], dir[ 0 ] );
      // }
    }
    
    //detect collisions
    // float dx = tempPos[ 0 ] - pos2[ 0 ];
    // float dy = tempPos[ 1 ] - pos2[ 1 ];
    // float dist = sqrt( dx * dx + dy * dy );
    // if ( dist >= r1 + r2 ) {
    //   // no collision detected
    //   pos1.x = tempPos[ 0 ];
    //   pos1.y = tempPos[ 1 ];
    // } else {
    //   tempPos[ 0 ] = pos1.x;
    //   tempPos[ 1 ] = pos1.y;
    // }
    CircleColliderManager::updateAndCollide();

    TransformTest::update( deltaT );
    
    // render scene
    glClear( GL_COLOR_BUFFER_BIT );
    SpriteManager::updateAndRender();
  
    // // render debug shapes
    DebugRenderer::renderAndClear();
    
    glfwSwapBuffers( window );

    // pseudo v-sync at 60fps
    t2 = glfwGetTime();
    deltaT = t2 - t1;
    if ( deltaT < ( 1 / 60.0 ) ) {
      double remaining = ( 1 / 60.0 ) - deltaT;
      // printf( "d = %f, 1/60 = %f, rem = %f, nanos = %f\n", deltaT, 1 / 60.0, remaining, remaining * 1.0e+9 );
      timespec amount = { 0, ( long )( remaining * 1.0e+9 ) };
      nanosleep( &amount, &amount );
      
      t2 = glfwGetTime();
      deltaT = t2 - t1;
    }
    t1 = t2;
  }
  Logger::write( "Main loop exited.\n" );
  
  // free OpenGL resources
  glUseProgram( 0 );
  Logger::write( "Resources freed.\n" );

  glfwDestroyWindow( window );
  glfwTerminate();

  TransformTest::shutdown();
  
  // shut down managers
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
    (Code for creating a "windowed fullscreen" window taken from the official GLFW docs: http:// www.glfw.org/docs/latest/window.html#window_full_screen.)
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
  // configure keyboard input
  glfwSetInputMode( window, GLFW_STICKY_KEYS, 1 );
  Logger::write( "Window created.\nOpenGL version %d.%d used.\n", glfwGetWindowAttrib( window, GLFW_CONTEXT_VERSION_MAJOR ), glfwGetWindowAttrib( window, GLFW_CONTEXT_VERSION_MINOR ) );  
  // initialize advanced opengl functionality
  glewExperimental = GL_TRUE;
  GLenum glewError = glewInit();  
  ASSERT( glewError == GLEW_OK, "GLEW failed to initialize: %s", glewGetErrorString( glewError ) );  
  // is this even possible when glfw already gave us a 3.3 context?
  ASSERT( GLEW_VERSION_3_3, "Required OpenGL version 3.3 unavailable, says GLEW" );    
  Logger::write( "OpenGL functionality successfully loaded.\n" );

  // setup opengl debugging
#ifndef NDEBUG
  if ( GLEW_KHR_debug ) {
    Logger::write( "Core KHR Debug extension found.\n" );
    glDebugMessageCallback( printOpenglError, nullptr );
    glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
  } else {
    Logger::write( "Core KHR Debug extension unavailable!\n" );
  }
#endif
  // basic gl configurations
  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

  return window;
}
