#pragma once

#include "EngineCommon.hpp"

typedef u32 AssetIndex;

struct TextureAsset {
  const u32 width, height;
  const u32 glId;
};

class AssetManager {
  static std::vector< TextureAsset > textureAssets;
  static u32 compileShaderStage( const char* source, const GLenum stage );
public:
  static void initialize();
  static void shutdown();
  static AssetIndex loadTexture( const char* name );
  // using #ifdef technique described in https://software.intel.com/en-us/blogs/2012/03/26/using-ifdef-in-opengl-es-20-shaders
  // so on a single shaderName.glsl file every stage goes between #ifdef and #endif, like:
  // #ifdef STAGE_NAME
  // 		stage code ...
  // #endif
  // STAGE_NAME can be one of VERTEX, GEOMETRY, or FRAGMENT
  static u32 loadShader( const char* name );
  static void destroyTexture( AssetIndex texture );
  static bool isTextureAlive( AssetIndex texture );
  static TextureAsset getTexture( AssetIndex texture );
};
