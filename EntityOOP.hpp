#pragma once

class Entity {
  Transform transform;
  Collider collider;
  SolidBody solidBody;
  Sprite sprite;
public:
  void setTransform( Transform transform ) { this->transform = transform; }
  void setCollider( Collider collider ) { this->collider = collider; }
  void setSolidBody( SolidBody solidBody ) { this->solidBody = solidBody; }
  void setSprite( Sprite sprite ) { this->sprite = sprite; }
  Transform& getTransform() { return transform; }
  Collider& getCollider() { return collider; }
  SolidBody& getSolidBody() { return solidBody; }
  Sprite& getSprite() { return sprite; }
  void removeTransform() { transform = {}; }
  void removeCollider() { collider = {}; }
  void removeSolidBody() { solidBody = {}; }
  void removeSprite() { sprite = {}; }

  void update( double deltaT ) {
    PROFILE;
    if ( *transform.getEntity() == this && *collider.getEntity() == this && *solidBody.getEntity() == this) {
      solidBody.update( deltaT );
    }
};
