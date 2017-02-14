#pragma once

#include "EngineCommon.hpp"

typedef u32 TextureHandle;

struct TextureAsset {
  const char* name;
  const u32 width, height;
  const u32 glId;
};

class AssetManager {
  static std::vector< TextureAsset > textureAssets;
public:
  static void initialize();
  static void shutdown();
  static std::vector< TextureHandle > loadTextures( std::vector< const char* >& names );
  static void destroyTextures( const std::vector< TextureHandle >& textures );
  static bool isTextureAlive( TextureHandle texture );
  static TextureAsset getTexture( TextureHandle texture );
};

// TODO put this shader and material stuff inside AssetManager

s32 loadShaderSourceFile( const char* name, char** source );

s32 compileShader( const char* name, const GLenum type, GLuint* shaderId );

s32 createShaderProgram( GLuint* shaderProgramId, const char* vertShaderId, const char* fragShaderId, const char* geomShaderId = 0 );
