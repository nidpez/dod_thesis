#pragma once

class Entity {
  Transform transform;
  bool hasT = false;
  Collider collider;
  bool hasC = false;
  SolidBody solidBody;
  bool hasSB = false;
  Sprite sprite;
  bool hasS = false;
  static std::vector< Entity* > allEntities;
public:
  Entity();
  
  void setTransform( Transform transform ) {
    transform.setEntity( this );
    this->transform = transform;
    hasT = true;
  }
  void setCollider( Collider collider ) { 
    collider.setEntity( this );
    this->collider = collider;
    hasC = true;
  }
  void setSolidBody( SolidBody solidBody ) {
    solidBody.setEntity( this );
    this->solidBody = solidBody;
    hasSB = true;
  }
  void setSprite( Sprite sprite ) {
    sprite.setEntity( this );
    this->sprite = sprite;
    hasS = true;
  }

  inline bool hasTransform() {
    return hasT;
  }
  inline bool hasCollider() {
    return hasC;
  }
  inline bool hasSolidBody() {
    return hasSB;
  }
  inline bool hasSprite() {
    return hasS;
  }
  
  Transform& getTransform() {
    ASSERT( hasT, "" );
    return transform;
  }
  Collider& getCollider() {
    ASSERT( hasC, "" );
    return collider;
  }
  SolidBody& getSolidBody() {
    ASSERT( hasSB, "" );
    return solidBody;
  }
  Sprite& getSprite() {
    ASSERT( hasS, "" );
    return sprite;
  }
  /*void removeTransform() { transform(); }
  void removeCollider() { collider = {}; }
  void removeSolidBody() { solidBody = {}; }
  void removeSprite() { sprite = {}; }*/

  static std::vector< Entity* > getAllEntities();
};
