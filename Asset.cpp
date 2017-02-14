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
 
std::vector< TextureHandle > AssetManager::loadTextures( std::vector< const char* >& names ) {
  std::vector< TextureHandle > textureHandles( names.size() );
  for ( u32 texInd = 0; texInd < names.size(); ++texInd ) {
    u32 width, height;
    s32 channels;
    unsigned char* texData = SOIL_load_image( names[ texInd ], reinterpret_cast< int* >( &width ), reinterpret_cast< int* >( &height ), &channels, SOIL_LOAD_RGBA );  
    ASSERT( texData != 0, "Error loading texture %s: %s", names[ texInd ], SOIL_last_result() );
    // pixelart seems to not want to be compressed to DXT
    s32 newWidth = width, newHeight = height;
    u32 glId = SOIL_create_OGL_texture( texData, &newWidth, &newHeight, channels, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y ); 
    SOIL_free_image_data( texData );
    ASSERT( glId > 0, "Error sending texture %s to OpenGL: %s", names[ texInd ], SOIL_last_result() );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); 
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    textureAssets.push_back( { names[ texInd ], width, height, glId } );
    Logger::write( "Texture '%s' successfully loaded (glId = %d)\n", names[ texInd ], glId );
    textureHandles[ texInd ] = textureAssets.size() - 1;
  }
  return textureHandles;
}

void AssetManager::destroyTextures( const std::vector< TextureHandle >& textures ) {
  for ( u32 texInd = 0; texInd < textures.size(); ++texInd ) {
    TextureHandle texture = textures[ texInd ];
    ASSERT( isTextureAlive( texture ), "Invalid texture id %d", texture );  
    glDeleteTextures( 1, &textureAssets[ texture ].glId );
    std::memset(  &textureAssets[ texture ], 0, sizeof( TextureAsset ) );
  }
}

bool AssetManager::isTextureAlive( TextureHandle texture ) {
  return texture < textureAssets.size() && textureAssets[ texture ].glId > 0;
}

TextureAsset AssetManager::getTexture( TextureHandle texture ) {
  ASSERT( isTextureAlive( texture ), "Invalid texture id %d", texture );  
  return textureAssets[ texture ];
}

s32 loadShaderSourceFile( const char* name, char** source ) {
  s32 error = 0;
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

s32 compileShader( const char* name, const GLenum type, GLuint* shaderId ) {
  GLchar* source;
  s32 fileError = loadShaderSourceFile( name, &source );
  if ( fileError ) {
    printf( "Unable to open vertex shader source file '%s': %s\n", name,
	    strerror( fileError ) );
    return 1;
  } 
  *shaderId = glCreateShader( type );
  glShaderSource( *shaderId, 1, ( const char** )&source, nullptr );
  glCompileShader( *shaderId );
  s32 compiled; 
  glGetShaderiv( *shaderId, GL_COMPILE_STATUS, &compiled );
  if ( compiled == GL_FALSE ) {
    s32 maxLength;
    glGetShaderiv( *shaderId, GL_INFO_LOG_LENGTH, &maxLength );
    GLchar* errorLog = ( GLchar* )malloc( sizeof( GLchar ) * maxLength );
    glGetShaderInfoLog( *shaderId, maxLength, &maxLength, errorLog );
    Logger::write( "Shader error:\n\t%s\n", errorLog );
    free( errorLog );
    Logger::write( "Shader source:\n\"%s\"\n", source );
    glDeleteShader( *shaderId );
    return 1;
    // TODO on shader error replace with default ugly shader instead of quitting
  }
  delete[] source;
  return 0;
}

s32 createShaderProgram( GLuint* shaderProgramId, const char* vertShaderFile, const char* fragShaderFile, const char* geomShaderFile ) {
  GLuint vertShaderId;
  s32 error = compileShader( vertShaderFile, GL_VERTEX_SHADER, &vertShaderId );
  if ( error ) {
    return error;
  } 
  GLuint geomShaderId;
  if ( geomShaderFile != 0 ) {
    error = compileShader( geomShaderFile, GL_GEOMETRY_SHADER,
			   &geomShaderId );
  }
  GLuint fragShaderId;  
  error = compileShader( fragShaderFile, GL_FRAGMENT_SHADER, &fragShaderId );
  if ( error ) {
    return error;
  }
  *shaderProgramId  = glCreateProgram();
  glAttachShader( *shaderProgramId, vertShaderId );
  if ( geomShaderFile != 0 ) {
    glAttachShader( *shaderProgramId, geomShaderId );
  }
  glAttachShader( *shaderProgramId, fragShaderId );
  glLinkProgram( *shaderProgramId );
  s32 linked = 0;
  glGetProgramiv( *shaderProgramId, GL_LINK_STATUS, ( s32 * )&linked );
  if( linked == GL_FALSE ) {
    s32 maxLength;
    glGetProgramiv( *shaderProgramId, GL_INFO_LOG_LENGTH, &maxLength );
    GLchar *errorLog = ( GLchar * )malloc( sizeof( GLchar )*maxLength );
    glGetProgramInfoLog( *shaderProgramId, maxLength, &maxLength, errorLog );
    printf( "Shader Program error:\n\t%s\n", errorLog );
    free( errorLog );
    glDeleteProgram( *shaderProgramId );
    glDeleteShader( vertShaderId );
    if ( geomShaderFile != 0 ) {
      glDeleteShader( geomShaderId );
    }
    glDeleteShader( fragShaderId );
    return 1;
  }  
  glUseProgram( *shaderProgramId );
  glDetachShader( *shaderProgramId, vertShaderId );
  glDeleteShader( vertShaderId );
  if ( geomShaderFile != 0 ) {
    glDetachShader( *shaderProgramId, geomShaderId );
    glDeleteShader( geomShaderId );
  }
  glDetachShader( *shaderProgramId, fragShaderId );
  glDeleteShader( fragShaderId );
  return 0;
}
