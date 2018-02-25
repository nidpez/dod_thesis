#pragma once

class Component {
protected:
  Entity& entity;
  static std::vector< Component& > all;
public:
  Component( Entity& entity );
  Entity& getEntity() {
    return entity;
  }
};

class Transform : public Component {
  Vec2 position;
  Vec2 scale;
  float orientation;
public:
  Transform( Entity& entity, Vec2 position, Vec2 scale, float orientation ) : Component( entity ), position( position ), scale( scale ), orientation( orientation ) {}
  void rotate( float rotation );
  void rotateAround( Vec2 point, float rotation );
  void translate( Vec2 translation );
  void scale( Vec2 scale );
  void setPosition( Vec2 position );
  void setOrientation( float orientation );
  Vec2 getPositon();
  Vec2 getScale();
  float getOrientation();
};

struct Collision {
  Collider a, b;
  Vec2 normalA, normalB;
};

class Collider : public Component {
  static std::vector< Entity > entities;
protected:
  virtual void addCollision( Collision collision ) = 0;
public:
  virtual bool collide( Collider colliderB ) = 0;
  virtual bool collide( Collider colliderB, Collision& collision ) = 0;
  virtual bool collide( CircleCollider circle ) = 0;
  virtual bool collide( CircleCollider circle, Collision& collision ) = 0;
  virtual bool collide( AARectCollider aaRect ) = 0;
  virtual bool collide( AARectCollider aaRect, Collision& collision ) = 0;
  virtual Collider getInWorldCoords() = 0;

  static void updateAndCollide();
  static bool circleCircleCollide( Circle circleA, Circle circleB );
  static bool circleCircleCollide( Circle circleA, Circle circleB, Vec2& normalA, Vec2& normalB );
  static bool aaRectCircleCollide( Rect aaRect, Circle circle );
  static bool aaRectCircleCollide( Rect aaRect, Circle circle, Vec2& normalA, Vec2& normalB );
  static bool aaRectAARectCollide( Rect aaRectA, Rect aaRectB );
  static bool aaRectAARectCollide( Rect aaRectA, Rect aaRectB, Vec2& normalA, Vec2& normalB );
};

class Sprite;

class CircleCollider : public Circle, Collider {
protected:
  void addCollision( Collision collision );
public:
  CircleCollider( Entity& entity, Vec2 center, float radius ) : Collider( entity ), Circle( center, radius ) {}
  bool collide( Collider colliderB );
  bool collide( Collider colliderB, Collision& collision );
  bool collide( CircleCollider circle );
  bool collide( CircleCollider circle, Collision& collision );
  bool collide( AARectCollider aaRect );
  bool collide( AARectCollider aaRect, Collision& collision );
  void fitToSprite( Sprite sprite );
  Collider getInWorldCoords();
};

class AARectCollider : public Rect, Collider {
  void addCollision( Collision collision );
public:
  AARectCollider( Entity& entity, Vec2 min, Vec2 max ) : Collider( entity ), Rect( min, max ) {}
  bool collide( Collider colliderB );
  bool collide( Collider colliderB, Collision& collision );
  bool collide( CircleCollider circle );
  bool collide( CircleCollider circle, Collision& collision );
  bool collide( AARectCollider aaRect );
  bool collide( AARectCollider aaRect, Collision& collision );
  Collider getInWorldCoords();
};

class QuadTree {
  struct QuadBucket {
    static const u8 CAPACITY = 8;
    Collider& _[ CAPACITY ];
    // TODO standarize indices starting at 1
    s8 lastInd = -1;
  };
  class QuadNode {
  protected:
    union {
      QuadBucket elements;
      QuadNode children[ 4 ];
    };
    AARectCollider boundary;
    bool isLeaf;
    QuadNode( AARectCollider boundary ) : elements( {} ), boundary( boundary ), isLeaf( true ) {}
    void subdivide();
  };
  QuadNode rootNode;
public:
  QuadTree( Rect boundary, std::vector< Collider& > colliders );
  void insert( Collider& collider );
};

class SolidBody : public Component {
  Vec2 speed;
public:
  SolidBody( Entity& entity, Vec2 speed ) : Component( entity ), speed( speed ) {}
  void update( double detlaT );
  void setSpeed( Vec2 speed );
  Vec2 getSpeed();
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
  Sprite( Entity& entity, AssetIndex textureId, Rect texCoords );
  ~Sprite();
  void setTextureId( AssetIndex textureId );
  void setTextureCoords( Rect textureCoords );
  AssetIndex getTextureId();
  Rect getTextureCoords();
  static void initialize();
  static void updateAndRender();
  static void setOrthoProjection( float aspectRatio, float height );
};
