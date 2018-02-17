#pragma once

class Transform {
  Vec2 position;
  Vec2 scale;
  float orientation;
public:
  Transform( Vec2 position, Vec2 scale, float orientation ) : position( position ), scale( scale ), orientation( orientation ) {}
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

class Collider {
  static std::vector< Entity > entities;
public:
  static void updateAndCollide();
  virtual bool collide( Collider colliderB ) = 0;
  virtual bool collide( Collider colliderB, Collision& collision ) = 0;
  virtual bool collide( CircleCollider circle ) = 0;
  virtual bool collide( CircleCollider circle, Collision& collision ) = 0;
  virtual bool collide( AARectCollider aaRect ) = 0;
  virtual bool collide( AARectCollider aaRect, Collision& collision ) = 0;

  static bool circleCircleCollide( Circle circleA, Circle circleB );
  static bool circleCircleCollide( Circle circleA, Circle circleB, Vec2& normalA, Vec2& normalB );
  static bool aaRectCircleCollide( Rect aaRect, Circle circle );
  static bool aaRectCircleCollide( Rect aaRect, Circle circle, Vec2& normalA, Vec2& normalB );
  static bool aaRectAARectCollide( Rect aaRectA, Rect aaRectB );
  static bool aaRectAARectCollide( Rect aaRectA, Rect aaRectB, Vec2& normalA, Vec2& normalB );
};

class Sprite;

class CircleCollider : public Collider {
  Circle circle;
public:
  Collider( Circle circle ) : circle( circle ) {}
  bool collide( Collider colliderB );
  bool collide( Collider colliderB, Collision& collision );
  bool collide( CircleCollider circle );
  bool collide( CircleCollider circle, Collision& collision );
  bool collide( AARectCollider aaRect );
  bool collide( AARectCollider aaRect, Collision& collision );
  void fitToSprite( Sprite sprite );
};

class AARectCollider : public Collider {
  Rect aaRect;
public:
  Collider( Rect aaRect ) : aaRect( aaRect ) {}
  bool collide( Collider colliderB );
  bool collide( Collider colliderB, Collision& collision );
  bool collide( CircleCollider circle );
  bool collide( CircleCollider circle, Collision& collision );
  bool collide( AARectCollider aaRect );
  bool collide( AARectCollider aaRect, Collision& collision );
};

class QuadTree {
  struct QuadBucket {
    static const u8 CAPACITY = 8;
    ComponentIndex _[ CAPACITY ];
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
  QuadTree( Rect boundary );
  void insert( Entity entity );
};

class SolidBody {
  Vec2 speed;
public:
  void update( double detlaT );
  void setSpeed( Vec2 speed );
  Vec2 getSpeed();
};
    
class Sprite {
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
  RenderInfo renderInfo;
  // TODO merge into single vertex attrib pointer
  Pos* posBufferData;
  UV* texCoordsBufferData;
public:
  Sprite( AssetIndex textureId, Rect texCoords ) : textureId( textureId ), texCoords( texCoords ) {}
  ~Sprite();
  void updateAndRender();
  void setTextureId( AssetIndex textureId );
  void setTextureCoords( Rect textureCoords );
  AssetIndex getTextureId();
  Rect getTextureCoords();
};
