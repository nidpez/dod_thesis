#pragma once

#include "EngineCommon.hpp"
#include "Math.hpp"
#include "Asset.hpp"
#include "Debug.hpp"

const float PIXELS_PER_UNIT = 4.0f;

struct Transform {
  Vec2 position;
  Vec2 scale;
  float orientation;
};

struct RotateAroundArg {
  Vec2 point;
  float rotation;
};

class TransformManager {
  struct TransformComp {
    EntityHandle entity;
    Transform local;
    Transform world;
    ComponentIndex parent;
    ComponentIndex firstChild;
    ComponentIndex nextSibling;
    ComponentIndex prevSibling;
  };
  static std::vector< TransformComp > transformComps;
  static ComponentMap componentMap;
public:
  static void initialize();
  static void shutdown();
  static void set( const std::vector< EntityHandle >& entities, const std::vector< Transform >& transforms );
  static void remove( const std::vector< EntityHandle >& entities );
  static void rotate( const std::vector< EntityHandle >& entities, const std::vector< float >& rotations );
  static void rotateAround( const std::vector< EntityHandle >& entities, const std::vector< RotateAroundArg >& rotations );
  static void translate( const std::vector< EntityHandle >& entities, const std::vector< Vec2 >& translations );
  static void scale( const std::vector< EntityHandle >& entities, const std::vector< Vec2 >& scales );
  static void update( const std::vector< EntityHandle >& entities, const std::vector< Transform >& transforms );
  static std::vector< Transform > get( const std::vector< EntityHandle >& entities );
  static std::vector< EntityHandle > getLastUpdated();
};

struct CircleColliderComp {
  EntityHandle entity;
  Vec2 center;
  float radius;
};

// TODO allow multiple colliders per entity (with linked list?)
class CircleColliderManager {
  struct __CircleColliderComp {
    EntityHandle entity;
    Vec2 center;
    float radius;
    // transform cache
    Vec2 position;
    Vec2 scale;
  };
  static std::vector< __CircleColliderComp > circleColliderComps;
  static ComponentMap componentMap;
public:
  static void initialize();
  static void shutdown();
  static void add( const std::vector< CircleColliderComp >& circleColliders );
  static void addAndFitToSpriteSize( const std::vector< EntityHandle >& entities );
  static void remove( const std::vector< EntityHandle >& entities );
  static void fitToSpriteSize( const std::vector< EntityHandle >& entities );
  static void updateAndCollide();
};

// Axis aligned bounding box 
struct Rect {
  Vec2 min;
  Vec2 max;
};

struct SpriteComp {
  EntityHandle entity;
  TextureHandle textureId;
  Rect texCoords;
  Vec2 size;
};

struct SetSpriteArg {
  EntityHandle entity;
  TextureHandle textureId;
  Rect texCoords;
};
    
class SpriteManager {
  struct __SpriteComp {
    EntityHandle entity;
    TextureHandle textureId;
    Rect texCoords;
    Vec2 size;
    // transform cache
    Vec2 position;
    Vec2 scale;
    float orientation;
    explicit operator SpriteComp() const;
  };
  static std::vector< __SpriteComp > spriteComps;
  static ComponentMap componentMap;
  // rendering data
  struct Pos {
    Vec2 pos;
  };
  struct UV {
    Vec2 uv;
  };
  static RenderInfo renderInfo;
  // TODO merge into single vertex attrib pointer
  static Pos* posBufferData;
  static UV* texCoordsBufferData;
public:
  static void initialize();
  static void shutdown();
  static void set( const std::vector< SetSpriteArg >& sprites );
  static void remove( const std::vector< EntityHandle >& entities );
  static std::vector< SpriteComp > get( const std::vector< EntityHandle >& entities );
  static void updateAndRender();
  static void setOrthoProjection( float aspectRatio, float height );
};

typedef u32 AnimationHandle;
// so we are able to later redefine AnimationFrame as something more robust
// that can handle more than uv flipbook animation
typedef Rect AnimationFrame; 

class AnimationManager {
  static std::vector< AnimationFrame > animations;
public:
  static void initialize();
  static void shutdown();
  static AnimationHandle add( EntityHandle entity, const AnimationFrame* frames, float fps, bool loops, bool autoplay );
  static void play( AnimationHandle animation );
};
