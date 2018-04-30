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
  static void rotate( const std::vector< ComponentIndex >& indices, const std::vector< float >& rotations );
  static void rotateAround( const std::vector< ComponentIndex >& indices, const std::vector< std::pair< Vec2, float > >& rotations );
  static void translate( const std::vector< ComponentIndex >& indices, const std::vector< Vec2 >& translations );
  static void scale( const std::vector< ComponentIndex >& indices, const std::vector< Vec2 >& scales );
  static void update( const std::vector< ComponentIndex >& indices, const std::vector< Transform >& transforms );
  static void get( const std::vector< ComponentIndex >& indices, std::vector< Transform >* result );
  static void lookup( const std::vector< EntityHandle >& entities, LookupResult* result );
  static std::vector< EntityHandle >& getLastUpdated();
};

struct Collision {
  Shape a, b;
  Vec2 normalA, normalB;
};

// TODO allow multiple colliders per entity (with linked list?)
class ColliderManager {
  struct ColliderComp {
    Shape _;
    // transform cache
    Vec2 position;
    Vec2 scale;
    //
    EntityHandle entity;
  };
  static ComponentMap< ColliderComp > componentMap;
  static std::vector< Shape > transformedShapes;
  static std::vector< std::vector< Collision > > collisions;

  struct QuadBucket {
    static const u8 CAPACITY = 16;
    ComponentIndex _[ CAPACITY ];
    // TODO standarize indices starting at 1
    s8 lastInd = -1;
  };
  struct QuadNode {
    union {
      QuadBucket elements = {};
      u32 childIndices[ 4 ];
    };
    Shape boundary = { {}, ShapeType::AARECT };
    bool isLeaf = true;
  };
  static std::vector< QuadNode > quadTree;
  static void buildQuadTree(Rect boundary);
  static void subdivideQuadNode(u32 nodeInd);
  static void insertIntoQuadTree(ComponentIndex colliderInd);
public:
  static void initialize();
  static void shutdown();
  static void addCircle( EntityHandle entity, Circle circleCollider );
  static void addAxisAlignedRect( EntityHandle entity, Rect aaRectCollider );
  static void remove( EntityHandle entity );
  static void fitCircleToSprite( EntityHandle entity );
  static void updateAndCollide();
  static void lookup( const std::vector< EntityHandle >& entities, LookupResult* result );
  static bool collide( Shape shapeA, Shape shapeB );
  static bool collide( Shape shapeA, Shape shapeB, Collision& collision );
  static bool circleCircleCollide( Circle circleA, Circle circleB );
  static bool circleCircleCollide( Circle circleA, Circle circleB, Vec2& normalA, Vec2& normalB );
  static bool aaRectCircleCollide( Rect aaRect, Circle circle );
  static bool aaRectCircleCollide( Rect aaRect, Circle circle, Vec2& normalA, Vec2& normalB );
  static bool aaRectAARectCollide( Rect aaRectA, Rect aaRectB );
  static bool aaRectAARectCollide( Rect aaRectA, Rect aaRectB, Vec2& normalA, Vec2& normalB );
  static std::vector< std::vector< Collision > >& getCollisions( const std::vector< ComponentIndex >& indices );
};

struct SolidBody {
  Vec2 speed;
};

class SolidBodyManager {
  struct SolidBodyComp {
    Vec2 speed;
    EntityHandle entity;
  };
  static ComponentMap< SolidBodyComp > componentMap;
public:
  static void initialize();
  static void shutdown();
  static void set( EntityHandle entity, SolidBody solidBody );
  static void remove( EntityHandle entity );
  static void setSpeed( const std::vector< ComponentIndex >& indices, std::vector< Vec2 >& speeds );
  static void get( const std::vector< ComponentIndex >& indices, std::vector< SolidBody >* result );
  static void update( double detlaT );
  static void lookup( const std::vector< EntityHandle >& entities, LookupResult* result );
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
  static void get( const std::vector< ComponentIndex >& indices, std::vector< Sprite >* result );
  static void updateAndRender();
  static void setOrthoProjection( float aspectRatio, float height );
  static void lookup( const std::vector< EntityHandle >& entities, LookupResult* result );
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
