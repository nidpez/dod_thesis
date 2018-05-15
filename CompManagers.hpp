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
  };
  // static bool ( *collisionFunctions[ ShapeType::LAST + 1 ][ ShapeType::LAST + 1 ] )( Shape a, Shape b, Vec2& normalA, Vec2& normalB );
  static ComponentMap< ColliderComp > componentMap[ ShapeType::LAST + 1 ];
  static std::vector< EntityHandle > entities[ ShapeType::LAST + 1 ];
  static std::vector< Shape > transformedShapes[ ShapeType::LAST + 1 ];
  static std::array< std::vector< std::vector< Collision > >, ShapeType::LAST + 1 > collisions;

  struct QuadBucket {
    static const u8 CAPACITY = 16;
    ComponentIndex _[ ShapeType::LAST + 1 ][ CAPACITY ];
    u8 size[ ShapeType::LAST + 1 ] = {};
    u8 totalSize = 0;
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
  static void buildQuadTree( Rect boundary );
  static void subdivideQuadNode( u32 nodeInd );
  static void insertIntoQuadTree( std::vector< ComponentIndex > colliderInds[ ShapeType::LAST + 1 ] );
public:
  static void initialize();
  static void shutdown();
  static void addCircle( EntityHandle entity, Circle circleCollider );
  static void addAxisAlignedRect( EntityHandle entity, Rect aaRectCollider );
  static void remove( EntityHandle entity );
  static void fitCircleToSprite( EntityHandle entity );
  static void updateAndCollide();
  // static bool collide( Shape shapeA, Shape shapeB );
  // static bool collide( Shape shapeA, Shape shapeB, Collision& collision );
  static bool circleCircleCollide( Circle circleA, Circle circleB );
  static bool circleCircleCollide( Circle circleA, Circle circleB, Vec2& normalA, Vec2& normalB );
  static void lookup( const std::vector< EntityHandle >& entities, std::array< LookupResult, ShapeType::LAST + 1 >* result );
  static bool circleCircleCollide( Shape a, Shape b );
  static bool circleCircleCollide( Shape a, Shape b, Vec2& normalA, Vec2& normalB );
  static bool aaRectCircleCollide( Shape a, Shape b );
  static bool aaRectCircleCollide( Shape a, Shape b, Vec2& normalA, Vec2& normalB );
  static bool aaRectAARectCollide( Shape a, Shape b );
  static bool aaRectAARectCollide( Shape a, Shape b, Vec2& normalA, Vec2& normalB );
  static std::array< std::vector< std::vector< Collision > >, ShapeType::LAST + 1 >& getCollisions();
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
