#pragma once

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
  static ComponentMap< TransformComp > componentMap;
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

// TODO allow multiple colliders per entity (with linked list?)
class CircleColliderManager {
  struct CircleColliderComp {
    EntityHandle entity;
    Circle circle;
    // transform cache
    Vec2 position;
    Vec2 scale;
  };
  static ComponentMap< CircleColliderComp > componentMap;
  static std::vector< Circle > transformedCircles;
  static const u8 QUADTREE_BUCKET_CAPACITY = 8;
  struct QuadBucket {
    ComponentIndex _[ QUADTREE_BUCKET_CAPACITY ];
    // TODO standarize indices starting at 1
    s8 lastInd = -1;
  };
  struct QuadNode {
    union {
      QuadBucket elements = {};
      u32 childIndices[ 4 ];
    };
    Rect boundary;
    bool isLeaf = true;
  };
  static std::vector< QuadNode > quadTree;
  static void buildQuadTree(Rect boundary);
  static void subdivideQuadNode(u32 nodeInd);
  static void insertIntoQuadTree(ComponentIndex colliderInd);
public:
  static void initialize();
  static void shutdown();
  static void add( EntityHandle entity, Circle circleCollider );
  static void addAndFitToSpriteSize( EntityHandle entity );
  static void remove( EntityHandle entity );
  static void fitToSpriteSize( EntityHandle entity );
  static void updateAndCollide();
  static bool circleCircleCollide( Circle circleA, Circle circleB );
  static bool circleAABBCollide( Circle circle, Rect aabb );
};

struct Sprite {
  AssetIndex textureId;
  Rect texCoords;
  Vec2 size;
};
    
class SpriteManager {
  struct SpriteComp {
    EntityHandle entity;
    Sprite sprite;
    // transform cache
    Transform transform;
    explicit operator Sprite() const;
  };
  static ComponentMap< SpriteComp > componentMap;
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
  static void set( EntityHandle entity, AssetIndex textureId, Rect texCoords );
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
