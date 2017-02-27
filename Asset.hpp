#pragma once

#include "EngineCommon.hpp"

typedef u32 AssetIndex;

struct TextureAsset {
  const u32 width, height;
  const u32 glId;
};

struct ShaderAsset {  
  const u32 glId;
};

class AssetManager {
  static std::vector< TextureAsset > textureAssets;
  static u32 compileShaderStage( const char* source, const GLenum stage );
public:
  static void initialize();
  static void shutdown();
  static AssetIndex loadTexture( const char* name );
  static u32 loadShader( const char* name );
  static void destroyTexture( AssetIndex texture );
  static bool isTextureAlive( AssetIndex texture );
  static TextureAsset getTexture( AssetIndex texture );
};
