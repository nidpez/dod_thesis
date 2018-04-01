#pragma once

class Entity;

class Component {
protected:
  Entity* entity;
public:
  void setEntity( Entity* entity ) {
    this->entity = entity;
  }
  Entity* getEntity() {
    return entity;
  }
  virtual ~Component() = default;
};

class Transform : public Component {
  Vec2 position;
  Vec2 scale;
  float orientation;
public:
  Transform() : position(), scale(), orientation( 0.0f ) {}
  Transform( Vec2 position, Vec2 scale, float orientation ) : position( position ), scale( scale ), orientation( orientation ) {}
  void rotate( float rotation );
  void rotateAround( Vec2 point, float rotation );
  void translate( Vec2 translation );
  void setScale( Vec2 scale );
  void setPosition( Vec2 position );
  void setOrientation( float orientation );
  Vec2 getPosition();
  Vec2 getScale();
  float getOrientation();
};

class Collision;

class Collider : public Component {
  Shape* shape;
  Shape* transformedShape;
  std::vector< Collision > collisions;
  void addCollision( Collision collision );
public:
  Collider() {}
  Collider( Circle shape );
  Collider( Rect shape );
  bool collide( Collider colliderB );
  bool collide( Collider colliderB, Collision& collision );
  bool collide( const Shape* shape );
  bool collide( const Shape* shape, Collision& collision );
  void fitCircleToSprite();
  Shape* getShape();
  Shape* getTransformedShape();
  std::vector< Collision > getCollisions();

  static void updateAndCollide();
  static bool circleCircleCollide( Circle circleA, Circle circleB );
  static bool circleCircleCollide( Circle circleA, Circle circleB, Vec2& normalA, Vec2& normalB );
  static bool aaRectCircleCollide( Rect aaRect, Circle circle );
  static bool aaRectCircleCollide( Rect aaRect, Circle circle, Vec2& normalA, Vec2& normalB );
  static bool aaRectAARectCollide( Rect aaRectA, Rect aaRectB );
  static bool aaRectAARectCollide( Rect aaRectA, Rect aaRectB, Vec2& normalA, Vec2& normalB );
};

struct Collision {
  Collider *a, *b;
  Vec2 normalA, normalB;
};

class QuadTree {
  class QuadNode {
  public:
    static std::list< QuadNode > allNodes;
    struct QuadBucket {
      static const u8 CAPACITY = 16;
      Collider* _[ CAPACITY ];
      // TODO standarize indices starting at 1
      s8 lastInd = -1;
    };
    union {
      QuadBucket elements;
      QuadNode* children[ 4 ];
    };
    Rect boundary;
    bool isLeaf;
    QuadNode( Rect boundary ) : elements( {} ), boundary( boundary ), isLeaf( true ) {}
    ~QuadNode() = default;
    void subdivide();
  };
  QuadNode rootNode;
public:
  QuadTree( Rect boundary, std::vector< Collider* >& colliders );
  void insert( Collider* collider );
  std::vector< std::vector< Collider* > > getGroupedElements();
};

class SolidBody : public Component {
  Vec2 speed;
public:
  SolidBody() : speed() {}
  SolidBody( Vec2 speed ) : speed( speed ) {}
  void setSpeed( Vec2 speed );
  Vec2 getSpeed();
  static void update( double deltaT );
};
    
class Sprite : public Component {
  AssetIndex textureId;
  Rect texCoords;
  Vec2 size;
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
  Sprite() : texCoords(), size() {}
  Sprite( AssetIndex textureId, Rect texCoords );
  void setTextureId( AssetIndex textureId );
  void setTextureCoords( Rect textureCoords );
  AssetIndex getTextureId();
  Rect getTextureCoords();
  Vec2 getSize();
  static void initialize();
  static void shutdown();
  static void updateAndRender();
  static void setOrthoProjection( float aspectRatio, float height );
};
