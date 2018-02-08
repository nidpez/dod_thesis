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
  void setScale( Vec2 scale );
  void setOrientation( float orientation );
  Vec2 getPositon();
  Vec2 getScale();
  float getOrientation();
};

struct Collision {
  Shape a, b;
  Vec2 normalA, normalB;
};

class Collider {
public:
  virtual bool collide( Collider colliderB ) = 0;
  virtual Collision collide( Collider colliderB ) = 0;
};

class Sprite;

class CircleCollider : public Collider {
  Circle circle;
public:
  Collider( Circle circle ) : circle( circle ) {}
  bool collide( Collider colliderB );
  Collision collide( Collider colliderB );
  void fitToSprite( Sprite sprite );
};

class AARectCollider : public Collider {
  Rect aaRect;
public:
  Collider( Rect aaRect ) : aaRect( aaRect ) {}
  bool collide( Collider colliderB );
  Collision collide( Collider colliderB );
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
      QuadBucket elements = {};
      u32 childIndices[ 4 ];
    };
    Shape boundary = { {}, ShapeType::AARECT };
    bool isLeaf = true;
    void subdivide();
  };
  std::vector< QuadNode > nodes;
public:
  QuadTree(Rect boundary);
  void insert(Collider collider);
};

class SolidBodyManager {
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
