#include "Asset.hpp"

#include <iostream>
#include <cstring>
#include <sstream>

#include "Debug.hpp"

std::vector< TextureAsset > AssetManager::textureAssets;

void AssetManager::initialize() {
}

void AssetManager::shutdown() {
}
 
AssetIndex AssetManager::loadTexture( const char* name ) {
  u32 width, height;
  s32 channels;
  unsigned char* texData = SOIL_load_image( name, reinterpret_cast< int* >( &width ), reinterpret_cast< int* >( &height ), &channels, SOIL_LOAD_RGBA );  
  ASSERT( texData != 0, "Error loading texture %s: %s", name, SOIL_last_result() );
  // pixelart seems to not want to be compressed to DXT
  s32 newWidth = width, newHeight = height;
  u32 glId = SOIL_create_OGL_texture( texData, &newWidth, &newHeight, channels, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y ); 
  SOIL_free_image_data( texData );
  ASSERT( glId > 0, "Error sending texture %s to OpenGL: %s", name, SOIL_last_result() );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
  textureAssets.push_back( { width, height, glId } );
  Debug::write( "Texture '%s' successfully loaded (glId = %d)\n", name, glId );
  return textureAssets.size() - 1;
}

void AssetManager::destroyTexture( AssetIndex texture ) {
  ASSERT( isTextureAlive( texture ), "Invalid texture id %d", texture );  
  glDeleteTextures( 1, &textureAssets[ texture ].glId );
  std::memset(  &textureAssets[ texture ], 0, sizeof( TextureAsset ) );
}

bool AssetManager::isTextureAlive( AssetIndex texture ) {
  return texture < textureAssets.size() && textureAssets[ texture ].glId > 0;
}

TextureAsset AssetManager::getTexture( AssetIndex texture ) {
  ASSERT( isTextureAlive( texture ), "Invalid texture id %d", texture );  
  return textureAssets[ texture ];
}

u32 AssetManager::compileShaderStage( const char* source, const GLenum stage ) {
  u32 shaderId = glCreateShader( stage );
  // add a #define statement to enable the required stage code
  char const* defVert = "#define VERTEX \n";
  char const* defGeom = "#define GEOMETRY \n";
  char const* defFrag = "#define FRAGMENT \n";
  char const* stageDefine = ( stage == GL_VERTEX_SHADER ) ? defVert :
    ( ( stage == GL_GEOMETRY_SHADER ) ? defGeom : defFrag );
  // compile using the #define before the source
  const char* shaderStrings[] = { stageDefine, source };
  glShaderSource( shaderId, 1, shaderStrings, nullptr );
  glCompileShader( shaderId );
  // FIXME getting the compilation error log is pointless with assertions disabled
  s32 compiled; 
  glGetShaderiv( shaderId, GL_COMPILE_STATUS, &compiled );
  if ( compiled == GL_FALSE ) {
    s32 maxLength;
    glGetShaderiv( shaderId, GL_INFO_LOG_LENGTH, &maxLength );
    GLchar* errorLog = ( GLchar* )malloc( sizeof( GLchar ) * maxLength );
    glGetShaderInfoLog( shaderId, maxLength, &maxLength, errorLog );
    glDeleteShader( shaderId );
    ASSERT( compiled, "Shader error:\n\t%s\nShader source:\n\"%s\"\n", errorLog, source );
    free( errorLog ); // don't leak when assertions are disabled 
  }
  return shaderId;
}

AssetIndex AssetManager::loadShader( const char* name ) {
  // using #ifdef technique described in https://software.intel.com/en-us/blogs/2012/03/26/using-ifdef-in-opengl-es-20-shaders
  bool hasGeomStage = false;
  char* source;
  // load the source code from the text file
  {
    using namespace std;
    ifstream file( name );
    ASSERT( file.is_open() && file.good(), "Problem loading shader file %s", name );
    string line;
    stringstream strStream; 
    while ( getline( file, line ) ) {
      strStream << line << endl;
      // find out if GEOMETRY stage is incluided
      u64 ifdefPos = line.find( "#ifdef" );
      if ( ifdefPos != string::npos ) {
        u64 macroPos = line.find( "GEOMETRY", ifdefPos );
        if ( macroPos != string::npos ) {
          hasGeomStage = true;
        }
      }
    }
    file.close();
    string sourceStr = strStream.str();
    source = new char[ sourceStr.length() + 1 ];
    strcpy( source, sourceStr.c_str() );
  }
  // compile the shader stages
  u32 vertShaderId = compileShaderStage( source, GL_VERTEX_SHADER );
  u32 geomShaderId;
  if ( hasGeomStage ) {
    geomShaderId = compileShaderStage( source, GL_GEOMETRY_SHADER );
  }
  u32 fragShaderId = compileShaderStage( source, GL_FRAGMENT_SHADER );
  // link the shader program
  u32 shaderProgramId  = glCreateProgram();
  glAttachShader( shaderProgramId, vertShaderId );
  if ( hasGeomStage ) {
    glAttachShader( shaderProgramId, geomShaderId );
  }
  glAttachShader( shaderProgramId, fragShaderId );
  glLinkProgram( shaderProgramId );
  // report error if assertions are enabled
  // FIXME getting the linking error log is pointless with assertions disabled
  s32 linked = 0;
  glGetProgramiv( shaderProgramId, GL_LINK_STATUS, ( s32 * )&linked );
  if( linked == GL_FALSE ) {
    s32 maxLength;
    glGetProgramiv( shaderProgramId, GL_INFO_LOG_LENGTH, &maxLength );
    GLchar *errorLog = ( GLchar * )malloc( sizeof( GLchar )*maxLength );
    glGetProgramInfoLog( shaderProgramId, maxLength, &maxLength, errorLog );
    glDeleteProgram( shaderProgramId );
    glDeleteShader( vertShaderId );
    if ( hasGeomStage ) {
      glDeleteShader( geomShaderId );
    }
    glDeleteShader( fragShaderId );
    ASSERT( linked, "Shader Program error:\n\t%s\n", errorLog );
    free( errorLog ); // don't leak when assertions are disabled
  }  
  glUseProgram( shaderProgramId );
  glDetachShader( shaderProgramId, vertShaderId );
  glDeleteShader( vertShaderId );
  if ( hasGeomStage ) {
    glDetachShader( shaderProgramId, geomShaderId );
    glDeleteShader( geomShaderId );
  }
  glDetachShader( shaderProgramId, fragShaderId );
  glDeleteShader( fragShaderId );

  return shaderProgramId;
}
