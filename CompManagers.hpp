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
  static void set( EntityHandle entity, Transform transform );
  static void remove( EntityHandle entity );
  static void rotate( EntityHandle entity, float rotation );
  static void rotateAround( EntityHandle entity, Vec2 point, float rotation );
  static void translate( EntityHandle entity, Vec2 translation );
  static void scale( EntityHandle entity, Vec2 scale );
  static void update( EntityHandle entity, Transform transform );
  static Transform get( EntityHandle entity );
  static std::vector< EntityHandle > getLastUpdated();
};

struct CircleCollider {
  EntityHandle entity;
  Vec2 center;
  float radius;
};

// TODO allow multiple colliders per entity (with linked list?)
class CircleColliderManager {
  struct CircleColliderComp {
    CircleCollider circle;
    // transform cache
    Vec2 position;
    Vec2 scale;
  };
  static std::vector< CircleColliderComp > circleColliderComps;
  static ComponentMap componentMap;
public:
  static void initialize();
  static void shutdown();
  static void add( CircleCollider circleCollider );
  static void addAndFitToSpriteSize( EntityHandle entity );
  static void remove( EntityHandle entity );
  static void fitToSpriteSize( EntityHandle entity );
  static void updateAndCollide();
};

// Axis aligned bounding box 
struct Rect {
  Vec2 min;
  Vec2 max;
};

struct Sprite {
  EntityHandle entity;
  TextureHandle textureId;
  Rect texCoords;
  Vec2 size;
};
    
class SpriteManager {
  struct SpriteComp {
    Sprite sprite;
    // transform cache
    Transform transform;
    explicit operator Sprite() const;
  };
  static std::vector< SpriteComp > spriteComps;
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
  static void set( EntityHandle entity, TextureHandle textureId, Rect texCoords );
  static void remove( EntityHandle entity );
  static Sprite get( EntityHandle entity );
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
