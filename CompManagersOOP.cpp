#include "EngineCommon.hpp"

void Transform::rotate( float rotation ) {
  PROFILE;
  orientation += rotation;
}

void Transform::rotateAround( Vec2 point, float rotation ) {
  PROFILE;
  orientation += rotation;
  position = ( position - point ).rotate( rotation ) + point;
}

void Transform::translate( Vec2 translation ) {
  PROFILE;
  position += translation;
}

void Transform::setScale( Vec2 scale ) {
  PROFILE; 
  this->scale = scale;
}

void Transform::setPosition( Vec2 position ) {
  this->position = position;
}

void Transform::setOrientation( float orientation ) {
  this->orientation = orientation;
}

Vec2 Transform::getPosition() {
  return position;
}

Vec2 Transform::getScale() {
  return scale;
}

float Transform::getOrientation() {
  return orientation;
}

Collider::Collider( Circle shape ) {
  this->shape = new Circle( shape );
  transformedShape = new Circle( shape );
}

Collider::Collider( Rect shape ) {
  this->shape = new Rect( shape );
  transformedShape = new Rect( shape );
}
    
bool Collider::collide( Collider colliderB ) {
  PROFILE;
  Shape* shapeB = colliderB.getTransformedShape();
  return collide( shapeB );
}

bool Collider::collide( Collider colliderB, Collision& collision ) {
  PROFILE;
  Shape* shapeB = colliderB.getTransformedShape();
  return collide( shapeB, collision );
}

bool Collider::collide( const Shape* shapeB ) {
  PROFILE;
  Shape* shapeA = getTransformedShape();
  Circle circleA( Vec2::ZERO, 0 ), circleB( Vec2::ZERO, 0 );
  Rect aaRectA, aaRectB;
  switch ( shapeA->getType() ) {
  case ShapeType::CIRCLE:
    circleA = *static_cast< const Circle* >( shapeA );
    switch ( shapeB->getType() ) {
    case ShapeType::CIRCLE:
      circleB = *static_cast< const Circle* >( shapeB );
      return circleCircleCollide( circleA, circleB );
    case ShapeType::AARECT:
      aaRectB = *static_cast< const Rect* >( shapeB );
      return aaRectCircleCollide( aaRectB, circleA );
    }
  case ShapeType::AARECT:
    aaRectA = *static_cast< const Rect* >( shapeA );
    switch ( shapeB->getType() ) {
    case ShapeType::CIRCLE:
      circleB = *static_cast< const Circle* >( shapeB );
      return aaRectCircleCollide( aaRectA, circleB );
    case ShapeType::AARECT:
      aaRectB = *static_cast< const Rect* >( shapeB );
      return aaRectAARectCollide( aaRectA, aaRectB );
    }
  }
  return false;
}

bool Collider::collide( const Shape* shapeB, Collision& collision ) {
  PROFILE;
  Shape* shapeA = getTransformedShape();
  Circle circleA( Vec2::ZERO, 0 ), circleB( Vec2::ZERO, 0 );
  Rect aaRectA, aaRectB;
  switch ( shapeA->getType() ) {
  case ShapeType::CIRCLE:
    circleA = *static_cast< const Circle* >( shapeA );
    switch ( shapeB->getType() ) {
    case ShapeType::CIRCLE:
      circleB = *static_cast< const Circle* >( shapeB );
      collision.a = this;
      // collision.b = &colliderB;
      return circleCircleCollide( circleA, circleB, collision.normalA, collision.normalB );
    case ShapeType::AARECT:
      aaRectB = *static_cast< const Rect* >( shapeB );
      // collision.a = &colliderB;
      collision.b = this;
      return aaRectCircleCollide( aaRectB, circleA, collision.normalB, collision.normalA );
    }
  case ShapeType::AARECT:
    aaRectA = *static_cast< const Rect* >( shapeA );
    collision.a = this;
    // collision.b = &colliderB;    
    switch ( shapeB->getType() ) {
    case ShapeType::CIRCLE:
      circleB = *static_cast< const Circle* >( shapeB );
      return aaRectCircleCollide( aaRectA, circleB, collision.normalA, collision.normalB );
    case ShapeType::AARECT:
      aaRectB = *static_cast< const Rect* >( shapeB );
      return aaRectAARectCollide( aaRectA, aaRectB, collision.normalA, collision.normalB );
    }
  }
  return false;
}

void Collider::fitCircleToSprite() {
  ASSERT( shape->getType() == ShapeType::CIRCLE, "This method is only valid for a circle collider" );
  Vec2 size = entity->getSprite().getSize();
  float maxSize = ( size.getX() > size.getY() ) ? size.getX() : size.getY();
  Circle* circle = static_cast< Circle* >( shape );
  circle->setRadius( maxSize / 2.0f );
}

Shape* Collider::getShape() {
  return shape;
}

Shape* Collider::getTransformedShape() {
  Transform transform = entity->getTransform();
  switch ( shape->getType() ) {
  case ShapeType::CIRCLE:
    {
      Circle circle = *static_cast< Circle* >( shape );
      float scaleX = transform.getScale().getX(), scaleY = transform.getScale().getY();
      float maxScale = ( scaleX > scaleY ) ? scaleX : scaleY;
      Vec2 position = transform.getPosition() + circle.getCenter() * maxScale;
      float radius = circle.getRadius() * maxScale;
      Circle* transformedCircle = static_cast< Circle* >( transformedShape );
      *transformedCircle = Circle( position, radius );
      return transformedShape;
    }
  case ShapeType::AARECT:
    {
      Rect aaRect = *static_cast< Rect* >( shape );
      Vec2 min = aaRect.getMin() * transform.getScale() + transform.getPosition();
      Vec2 max = aaRect.getMax() * transform.getScale() + transform.getPosition();
      Rect* transformedRect = static_cast< Rect* >( transformedShape );
      *transformedRect = Rect( min, max );
      return transformedShape;
    }
  }
  ASSERT( false, "Tried to transform a shape of an unrecognized type" );
  return nullptr;
}

std::vector< Collision > Collider::getCollisions() {
  return collisions;
}
 
 // FIXME take scale into account
bool Collider::circleCircleCollide( Circle circleA, Circle circleB ) {
  PROFILE;
  float radiiSum = circleA.getRadius() + circleB.getRadius();
  return ( circleA.getCenter() - circleB.getCenter() ).sqrMagnitude() <= radiiSum * radiiSum;
}

// FIXME take scale into account
bool Collider::circleCircleCollide( Circle circleA, Circle circleB, Vec2& normalA, Vec2& normalB ) {
  PROFILE;
  Vec2 ab = circleB.getCenter() - circleA.getCenter();
  normalA = ab.normalized();
  normalB = -normalA;

  float radiiSum = circleA.getRadius() + circleB.getRadius();
  return ab.sqrMagnitude() <= radiiSum * radiiSum;
}

// FIXME take scale into account
bool Collider::aaRectCircleCollide( Rect aaRect, Circle circle ) {
  PROFILE;
  // TODO assert integrity of circle and aaRect
  // TODO refactor distance to aaRect function
  // taken from Real-Time Collision Detection - Christer Ericson, 5.2.5 Testing Sphere Against AARECT
  float sqrDistance = 0.0f;
  if ( circle.getCenter().getX() < aaRect.getMin().getX() ) {
    sqrDistance += ( aaRect.getMin().getX() - circle.getCenter().getX() ) * ( aaRect.getMin().getX() - circle.getCenter().getX() );
  }
  if ( circle.getCenter().getX() > aaRect.getMax().getX() ) {
    sqrDistance += ( circle.getCenter().getX() - aaRect.getMax().getX() ) * ( circle.getCenter().getX() - aaRect.getMax().getX() );
  }
  if ( circle.getCenter().getY() < aaRect.getMin().getY() ) {
    sqrDistance += ( aaRect.getMin().getY() - circle.getCenter().getY() ) * ( aaRect.getMin().getY() - circle.getCenter().getY() );
  }
  if ( circle.getCenter().getY() > aaRect.getMax().getY() ) {
    sqrDistance += ( circle.getCenter().getY() - aaRect.getMax().getY() ) * ( circle.getCenter().getY() - aaRect.getMax().getY() );
  }
  return sqrDistance <= circle.getRadius() * circle.getRadius(); 
}

// FIXME take scale into account
bool Collider::aaRectCircleCollide( Rect aaRect, Circle circle, Vec2& normalA, Vec2& normalB ) {
  PROFILE;
  // TODO assert integrity of circle and aaRect
  // taken from Real-Time Collision Detection - Christer Ericson, 5.2.5 Testing Sphere Against AARECT
  normalA = {};
  float x = circle.getCenter().getX();
  if ( x < aaRect.getMin().getX() ) {
    x = aaRect.getMin().getX();
    normalA.setX( -1.0 );
  }
  if ( x > aaRect.getMax().getX() ) {
    x = aaRect.getMax().getX();
    normalA.setX( 1.0 );
  }
  float y = circle.getCenter().getY();
  if ( y < aaRect.getMin().getY() ) {
    y = aaRect.getMin().getY();
    normalA.setY( -1.0 );
  }
  if ( y > aaRect.getMax().getY() ) {
    y = aaRect.getMax().getY();
    normalA.setY( 1.0 );
  }
  normalA = normalA.normalized();
  Vec2 closestPtInRect = { x, y };
  Vec2 circleToRect = closestPtInRect - circle.getCenter();
  normalB = circleToRect.normalized();

  return circleToRect.sqrMagnitude() <= circle.getRadius() * circle.getRadius();
}

// TODO implement
bool Collider::aaRectAARectCollide( Rect aaRectA, Rect aaRectB ) {
  aaRectA = aaRectB = Rect();
  return true;
}

// TODO implement
bool Collider::aaRectAARectCollide( Rect aaRectA, Rect aaRectB, Vec2& normalA, Vec2& normalB ) {
  aaRectA = aaRectB = Rect();
  normalA = normalB = Vec2();
  return false;
}

void Collider::addCollision( Collision collision ) {
  collisions.push_back( collision );
}

void Collider::updateAndCollide() {
  PROFILE;
  // space partitioned collision detection
  // keep the quadtree updated
  std::vector< Collider* > colliders;
  {
    std::vector< Entity* > entities = Entity::getAllEntities();
    for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
      Entity* entity = entities[ entInd ];
      if ( entity->hasCollider() ) {
        Collider& collider = entity->getCollider();
        collider.collisions.clear();
        colliders.push_back( &collider );
      }
    }
  }
  // TODO calculate the boundary dynamically
  Rect boundary = { { -70, -40 }, { 70, 40 } };
  QuadTree quadTree( boundary, colliders );
  std::vector< std::vector< Collider* > > quadTreeGroups = quadTree.getGroupedElements();
  // detect collisions
  for ( u32 groupInd = 0; groupInd < quadTreeGroups.size(); ++groupInd ) {
    std::vector< Collider* > colliderGroup = quadTreeGroups[ groupInd ];
    for ( u8 i = 0; i < colliderGroup.size() - 1; ++i ) {
      Collider* collI = colliderGroup[ i ];
      for ( u8 j = i + 1; j < colliderGroup.size(); ++j ) {
        Collider* collJ = colliderGroup[ j ];
        Collision collision;
        if ( collI->collide( *collJ, collision ) ) {
          collI->addCollision( collision );
          collJ->addCollision( { collision.b, collision.a, collision.normalB, collision.normalA } );
          Debug::drawShape( collI->getTransformedShape(), Debug::GREEN );
          Debug::drawShape( collJ->getTransformedShape(), Debug::GREEN );
        }
      }
    }
  }
}

QuadTree::QuadTree( Rect boundary, std::vector< Collider* >& colliders ) : rootNode( boundary ) {
  PROFILE;
  for ( u32 colliderInd = 0; colliderInd < colliders.size(); ++colliderInd ) {
    insert( colliders[ colliderInd ] );
  }
  // debug render space partitions
  std::stack< QuadNode* > nodeStack;
  nodeStack.push( &rootNode );
  while ( !nodeStack.empty() ) {
    QuadNode* quadNode = nodeStack.top();
    nodeStack.pop();
    if ( !quadNode->isLeaf ) {
      for ( int i = 0; i < 4; ++i ) {
        nodeStack.push( quadNode->children[ i ] );
      }
      continue;
    }
    Debug::drawRect( quadNode->boundary, { 1, 1, 1, 0.3f } );
    for ( int i = 0; i <= quadNode->elements.lastInd; ++i ) {
      Debug::drawShape( quadNode->elements._[ i ]->getTransformedShape(), Debug::BLUE );
    }
  }
}

// FIXME if QuadBucket::CAPACITY + 1 colliders are at the center of the node
//       we will subdivide it forever
void QuadTree::QuadNode::subdivide() {
  PROFILE;
  // backup elements
  QuadBucket __elements = std::move( elements );
  isLeaf = false;
  Vec2 min = boundary.getMin();
  Vec2 max = boundary.getMax();
  Vec2 center = min + ( max - min ) / 2.0f;
  // top-right
  children[ 0 ] = new QuadNode( Rect( { center, max } ) );
  // bottom-right
  children[ 1 ] = new QuadNode( Rect( { { center.getX(), min.getY() }, { max.getX(), center.getY() } } ) );
  // bottom-left
  children[ 2 ] = new QuadNode( Rect( { min, center } ) );
  // top-left
  children[ 3 ] = new QuadNode( Rect( { { min.getX(), center.getY() }, { center.getX(), max.getY() } } ) );
  // put elements inside children
  for ( int elemInd = 0; elemInd <= __elements.lastInd; ++elemInd ) {
    Collider* collider = __elements._[ elemInd ];
    for ( int childI = 0; childI < 4; ++childI ) {
      QuadNode* child = children[ childI ];
      if ( collider->collide( &child->boundary ) ) {
        child->elements._[ ++child->elements.lastInd ] = collider;
      }
    }
  }
}

void QuadTree::insert( Collider* collider ) {
  PROFILE;
  std::deque< QuadNode* > nextNodes = std::deque< QuadNode* >();
  if ( collider->collide( &rootNode.boundary ) ) {
    nextNodes.push_front( &rootNode );
  }
#ifndef NDEBUG
  bool inserted = false;
#endif
  while ( !nextNodes.empty() ) {
    QuadNode* node = nextNodes.front();
    nextNodes.pop_front();
    // try to add collider to this node
    if ( node->isLeaf ) {
      // ... if there is still space
      if ( node->elements.lastInd < QuadNode::QuadBucket::CAPACITY - 1 ) {
        node->elements._[ ++node->elements.lastInd ] = collider;
#ifndef NDEBUG
        inserted = true;
#endif
      } else { 
        // if there is no space this cannot be a leaf any more
        node->subdivide();
      }
    }
    if ( !node->isLeaf ) {
      // find which children the collider intersects with
      // and add them to the deque
      for ( int i = 0; i < 4; ++i ) {
        QuadNode* child = node->children[ i ];
        if ( collider->collide( &child->boundary ) ) {
          nextNodes.push_front( child );
        }
      }
    }
  }
  ASSERT( inserted, "Collider of entity %d not inserted into QuadTree", collider->getEntity() ); 
}

std::vector< std::vector< Collider* > > QuadTree::getGroupedElements() {
  PROFILE;
  std::vector< std::vector< Collider* > > quadGroups;
  std::stack< QuadNode* > nextNodes;
  nextNodes.push( &rootNode );
  while ( !nextNodes.empty() ) {
    QuadNode* quadNode = nextNodes.top();
    nextNodes.pop();
    if ( !quadNode->isLeaf ) {
      for ( int childI = 0; childI < 4; ++childI ) {
        nextNodes.push( quadNode->children[ childI ] );
      }
      continue;
    }
    int groupSize = quadNode->elements.lastInd + 1;
    if ( groupSize > 0 ) {
      std::vector< Collider* > group;
      group.reserve( groupSize );
      for ( int i = 0; i < groupSize; ++i ) {
        group.push_back( quadNode->elements._[ i ] );
      }
      quadGroups.push_back( group );
    }
  }
  return quadGroups;
}

void SolidBody::setSpeed( Vec2 speed ) {
  this->speed = speed;
}

Vec2 SolidBody::getSpeed() {
  return speed;
}

void SolidBody::update( double deltaT ) {
  PROFILE;
  std::vector< Entity* > entities = Entity::getAllEntities();
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    Entity* entity = entities[ entInd ];
    if ( entity->hasSolidBody() ) {
      std::vector< Collision > collisions = entity->getCollider().getCollisions();
      // move solid body
      SolidBody& solidBody = entity->getSolidBody();
      Vec2 speed = solidBody.getSpeed();
      Vec2 normal = {};
      for ( u32 i = 0; i < collisions.size(); ++i ) {
        if ( speed.dot( collisions[ i ].normalB ) <= 0.0f ) {
          normal += collisions[ i ].normalB;
        }
      }
      if ( normal.getX() != 0 || normal.getY() != 0 ) {
        // reflect direction
        normal = normal.normalized();
        float vDotN = speed.dot( normal );
        speed = speed - 2.0f * vDotN * normal;
      }
      solidBody.setSpeed( speed );
      entity->getTransform().translate( speed * deltaT );
    }
  }
}

RenderInfo Sprite::renderInfo;
Sprite::Pos* Sprite::posBufferData;
Sprite::UV* Sprite::texCoordsBufferData;

void Sprite::initialize() {
  // configure buffers
  glGenVertexArrays( 1, &renderInfo.vaoId );
  glBindVertexArray( renderInfo.vaoId );
  glGenBuffers( 2, renderInfo.vboIds );
  // positions buffer
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 0, ( void* )0 );
  glEnableVertexAttribArray( 0 );
  // texture coordinates buffer
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 1 ] );
  glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 0, ( void* )0 );
  glEnableVertexAttribArray( 1 );
  glBindVertexArray( 0 );
  // create shader program
  renderInfo.shaderProgramId = AssetManager::loadShader( "shaders/SpriteUnlit.glsl" );
  // get shader's constants' locations
  renderInfo.projUnifLoc[ 0 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.left" );
  renderInfo.projUnifLoc[ 1 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.right" );
  renderInfo.projUnifLoc[ 2 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.bottom" );
  renderInfo.projUnifLoc[ 3 ] = glGetUniformLocation( renderInfo.shaderProgramId, "projection.top" );
}

void Sprite::shutdown() {
  glDeleteProgram( renderInfo.shaderProgramId );
  glDeleteVertexArrays( 1, &renderInfo.vaoId );
  glDeleteBuffers( 2, renderInfo.vboIds );
}

Sprite::Sprite( AssetIndex textureId, Rect texCoords ) : texCoords( texCoords ) {
  setTextureId( textureId );
}

void Sprite::setTextureId( AssetIndex textureId ) {
  ASSERT( AssetManager::isTextureAlive( textureId ), "Invalid texture id %d", textureId );
  this->textureId = textureId;
  TextureAsset texture = AssetManager::getTexture( textureId );
  float width = texture.width * ( texCoords.getMax().getU() - texCoords.getMin().getU() ) / PIXELS_PER_UNIT;
  float height = texture.height * ( texCoords.getMax().getV() - texCoords.getMin().getV() ) / PIXELS_PER_UNIT;
  size = { width, height };
}
    
void Sprite::setTextureCoords( Rect textureCoords ) {
  this->texCoords = textureCoords;
}
    
AssetIndex Sprite::getTextureId() {
  return textureId;
}
    
Rect Sprite::getTextureCoords() {
  return texCoords;
}

Vec2 Sprite::getSize() {
  return size;
}
    
void Sprite::setOrthoProjection( float aspectRatio, float height ) {
  float halfHeight = height / 2.0f;
  glUseProgram( renderInfo.shaderProgramId );
  glUniform1f( renderInfo.projUnifLoc[ 0 ], -halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 1 ], halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 2 ], -halfHeight );
  glUniform1f( renderInfo.projUnifLoc[ 3 ], halfHeight );
}

void Sprite::updateAndRender() {
  PROFILE;
  std::vector< Entity* > entities = Entity::getAllEntities();
  std::vector< Sprite* > sprites;
  for ( u32 i = 0; i < entities.size(); ++i ) {
    Entity* entity = entities[ i ];
    if ( entity->hasSprite() ) {
      sprites.push_back( &entity->getSprite() );
    }
  }
  if ( sprites.size() == 0 ) {
    return;
  }
  // build vertex buffer and render for sprites with same texture
  glUseProgram( renderInfo.shaderProgramId );
  glBindVertexArray( renderInfo.vaoId );
  // TODO don't render every sprite every time
  u32 spritesToRenderCount = sprites.size();
  // TODO use triangle indices to reduce vertex count
  u32 vertsPerSprite = 6; 
  posBufferData = new Pos[ spritesToRenderCount * vertsPerSprite ];
  texCoordsBufferData = new UV[ spritesToRenderCount * vertsPerSprite ];
  // build the positions buffer
  Vec2 baseGeometry[] = {
    { -0.5f,	-0.5f },
    { 0.5f,	 0.5f },
    { -0.5f,	 0.5f },
    { -0.5f,	-0.5f },
    { 0.5f,	-0.5f },
    { 0.5f,	 0.5f }
  };
  for ( u32 spriteInd = 0; spriteInd < spritesToRenderCount; ++spriteInd ) {
    Sprite* sprite = sprites[ spriteInd ];
    Transform transform = sprite->getEntity()->getTransform();
    for ( u32 vertInd = 0; vertInd < vertsPerSprite; ++vertInd ) {
      Vec2 vert = baseGeometry[ vertInd ] * sprite->size * transform.getScale();
      vert = vert.rotate( transform.getOrientation() );
      vert += transform.getPosition();
      posBufferData[ spriteInd * vertsPerSprite + vertInd ].pos = vert;
    }
  } 
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glBufferData( GL_ARRAY_BUFFER, spritesToRenderCount * vertsPerSprite * sizeof( Pos ), posBufferData, GL_STATIC_DRAW );
  // TODO measure how expensive these allocations and deallocations are!
  delete[] posBufferData;
  // build the texture coordinates buffer
  for ( u32 spriteInd = 0; spriteInd < spritesToRenderCount; ++spriteInd ) {
    Sprite* sprite = sprites[ spriteInd ];
    Vec2 max = sprite->getTextureCoords().getMax();
    Vec2 min = sprite->getTextureCoords().getMin();
    Vec2 texCoords[] = {
      min, max, { min.getU(), max.getV() },
      min, { max.getU(), min.getV() }, max
    };
    for ( u32 vertInd = 0; vertInd < vertsPerSprite; ++vertInd ) {
      texCoordsBufferData[ spriteInd * vertsPerSprite + vertInd ].uv = texCoords[ vertInd ];
    }
  }
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 1 ] );
  glBufferData( GL_ARRAY_BUFFER, spritesToRenderCount * vertsPerSprite * sizeof( UV ), texCoordsBufferData, GL_STATIC_DRAW );
  // TODO measure how expensive these allocations and deallocations are!
  delete[] texCoordsBufferData;
  // issue render commands
  // TODO keep sorted by texture id
  u32 currentTexId = sprites[ 0 ]->textureId;  
  ASSERT( AssetManager::isTextureAlive( currentTexId ), "Invalid texture id %d", currentTexId );  
  u32 currentTexGlId = AssetManager::getTexture( currentTexId ).glId;
  glBindTexture( GL_TEXTURE_2D, currentTexGlId );
  // mark where a sub-buffer with sprites sharing a texture ends and a new one begins
  u32 currentSubBufferStart = 0;
  for ( u32 spriteInd = 0; spriteInd < spritesToRenderCount; ++spriteInd ) {
    if ( sprites[ spriteInd ]->textureId != currentTexId ) {
      // send current vertex sub-buffer and render it
      u32 spriteCountInSubBuffer = spriteInd - currentSubBufferStart;
      glDrawArrays( GL_TRIANGLES, vertsPerSprite * currentSubBufferStart, vertsPerSprite * spriteCountInSubBuffer );
      // and start a new one
      currentTexId = sprites[ spriteInd ]->textureId;      
      ASSERT( AssetManager::isTextureAlive( currentTexId ), "Invalid texture id %d", currentTexId );
      currentTexGlId = AssetManager::getTexture( currentTexId ).glId;
      glBindTexture( GL_TEXTURE_2D, currentTexGlId );
      currentSubBufferStart = spriteInd;
    }
  }
  // render the last sub-buffer
  u32 spriteCountInSubBuffer = spritesToRenderCount - currentSubBufferStart;
  glDrawArrays( GL_TRIANGLES, vertsPerSprite * currentSubBufferStart, vertsPerSprite * spriteCountInSubBuffer );
}
