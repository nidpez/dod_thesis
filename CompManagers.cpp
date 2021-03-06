#include "EngineCommon.hpp"

ComponentMap< TransformManager::TransformComp > TransformManager::componentMap;

void TransformManager::initialize() {
}

void TransformManager::shutdown() {
}

void TransformManager::set( EntityHandle entity, Transform transform ) {
  componentMap.set( entity, { entity, transform, transform, 0, 0, 0, 0 }, &TransformManager::remove );
}

void TransformManager::remove( EntityHandle entity ) {
  componentMap.remove( entity );
}

// TODO verify that component indices have not been invalidated
void TransformManager::rotate( const std::vector< ComponentIndex >& indices, const std::vector< float >& rotations ) {
  PROFILE;
  ASSERT( indices.size() == rotations.size(), "" );
  for ( u32 i = 0; i < indices.size(); ++i ) {
    componentMap.components[ indices[ i ] ].local.orientation += rotations[ i ];
    // TODO mark transform component as updated
  }
}

void TransformManager::rotateAround( const std::vector< ComponentIndex >& indices, const std::vector< std::pair< Vec2, float > >& rotations ) {
  PROFILE;
  ASSERT( indices.size() == rotations.size(), "" );
  for ( u32 i = 0; i < indices.size(); ++i ) {  
    ComponentIndex componentInd = indices[ i ];
    Transform transform = componentMap.components[ componentInd ].local;
    Vec2 point = rotations[ i ].first;
    float rotation = rotations[ i ].second;
    transform.orientation += rotation;
    transform.position = rotateVec2( transform.position - point, rotation ) + point;
    componentMap.components[ componentInd ].local = transform;
    // TODO mark transform component as updated
  }
}

void TransformManager::translate( const std::vector< ComponentIndex >& indices, const std::vector< Vec2 >& translations ) {
  PROFILE;
  ASSERT( indices.size() == translations.size(), "" );
  for ( u32 i = 0; i < indices.size(); ++i ) {
    componentMap.components[ indices[ i ] ].local.position += translations[ i ];
    // TODO mark transform component as updated
  }
}

void TransformManager::scale( const std::vector< ComponentIndex >& indices, const std::vector< Vec2 >& scales ) {
  PROFILE;
  ASSERT( indices.size() == scales.size(), "" );
  for ( u32 i = 0; i < indices.size(); ++i ) {
    componentMap.components[ indices[ i ] ].local.scale = scales[ i ];
    // TODO mark transform component as updated
  }
}

void TransformManager::update( const std::vector< ComponentIndex >& indices, const std::vector< Transform >& transforms ) {
  PROFILE;
  ASSERT( indices.size() == transforms.size(), "" );
  for ( u32 i = 0; i < indices.size(); ++i ) {
    componentMap.components[ indices[ i ] ].local = transforms[ i ];
    // TODO mark transform component as updated
  }
}

// TODO verify that component indices have not been invalidated
void TransformManager::get( const std::vector< ComponentIndex >& indices, std::vector< Transform >* result ) {
  result->reserve( indices.size() );
  for ( u32 i = 0; i < indices.size(); ++i ) {
    result->push_back( componentMap.components[ indices[ i ] ].local );
  }
}

void TransformManager::lookup( const std::vector< EntityHandle >& entities, LookupResult* result ) {
  return componentMap.lookup( entities, result );
}

std::vector< EntityHandle >& TransformManager::getLastUpdated() {
  // TODO actually compute which transforms have been updated since last frame
  // component index 0 is not valid
  static std::vector< EntityHandle > result;
  result.clear();
  result.reserve( componentMap.components.size() - 1 );
  for ( u32 entInd = 1; entInd < componentMap.components.size(); ++entInd ) {
    result.push_back( componentMap.components[ entInd ].entity );
  }
  return result;
}

ComponentMap< ColliderManager::ColliderComp > ColliderManager::componentMap;
std::vector< Shape > ColliderManager::transformedShapes;
std::vector< std::vector< Collision > > ColliderManager::collisions;
std::vector< ColliderManager::QuadNode > ColliderManager::quadTree;

void ColliderManager::buildQuadTree(Rect boundary) {
  PROFILE;
  quadTree.clear();
  quadTree.push_back( {} );
  QuadNode rootNode = {};
  rootNode.boundary.aaRect = boundary;
  quadTree.push_back( rootNode );
  for ( u32 colliderInd = 1; colliderInd < componentMap.components.size(); ++colliderInd ) {
    insertIntoQuadTree( colliderInd );
  }
  // debug render space partitions 
  for ( u32 nodeInd = 1; nodeInd < quadTree.size(); ++nodeInd ) {
    QuadNode quadNode = quadTree[ nodeInd ];
    if ( !quadNode.isLeaf ) {
      continue;
    }
    Debug::drawRect( quadNode.boundary.aaRect, { 1, 1, 1, 0.3f } );
    for ( int i = 0; i <= quadNode.elements.lastInd; ++i ) {
      ComponentIndex ci = quadNode.elements._[ i ];
      Debug::drawShape( transformedShapes[ ci ], Debug::BLUE );
    }
  }
}

// FIXME if QuadBucket::CAPACITY + 1 colliders are at the center of the node
//       we will subdivide it forever
void ColliderManager::subdivideQuadNode(u32 nodeInd) {
  PROFILE;
  // backup elements
  QuadBucket elements = std::move( quadTree[ nodeInd ].elements );
  quadTree[ nodeInd ].isLeaf = false;
  Vec2 min = quadTree[ nodeInd ].boundary.aaRect.min;
  Vec2 max = quadTree[ nodeInd ].boundary.aaRect.max;
  Vec2 center = min + ( max - min ) / 2.0f;
  u32 lastInd = quadTree.size();
  // top-right
  QuadNode child = {};
  child.boundary.aaRect = { center, max };
  quadTree.push_back( child );
  quadTree[ nodeInd ].childIndices[ 0 ] = lastInd++;
  // bottom-right
  child.boundary.aaRect = { { center.x, min.y }, { max.x, center.y } };
  quadTree.push_back( child );
  quadTree[ nodeInd ].childIndices[ 1 ] = lastInd++;
  // bottom-left
  child.boundary.aaRect = { min, center };
  quadTree.push_back( child );
  quadTree[ nodeInd ].childIndices[ 2 ] = lastInd++;
  // top-left
  child.boundary.aaRect = { { min.x, center.y }, { center.x, max.y } };
  quadTree.push_back( child );
  quadTree[ nodeInd ].childIndices[ 3 ] = lastInd;
  // put elements inside children
  for ( int elemInd = 0; elemInd <= elements.lastInd; ++elemInd ) {
    ComponentIndex colliderInd = elements._[ elemInd ];
    Shape collider = transformedShapes[ colliderInd ];
    for ( int childI = 0; childI < 4; ++childI ) {
      QuadNode& child = quadTree[ quadTree[ nodeInd ].childIndices[ childI ] ];
      if ( collide( collider, child.boundary ) ) {
        child.elements._[ ++child.elements.lastInd ] = colliderInd;
      }
    }
  }
}

void ColliderManager::insertIntoQuadTree(ComponentIndex colliderInd) {
  PROFILE;
  ASSERT( colliderInd < componentMap.components.size(),
          "Component index %d out of bounds", colliderInd );
  std::deque< u32 > nextNodeInds = std::deque< u32 >();
  // index 0 is null
  Shape collider = transformedShapes[ colliderInd ];
  if ( collide( collider, quadTree[ 1 ].boundary ) ) {
    nextNodeInds.push_front( 1 );
  }
#ifndef NDEBUG
  bool inserted = false;
#endif
  while ( !nextNodeInds.empty() ) {
    u32 nodeInd = nextNodeInds.front();
    nextNodeInds.pop_front();
    // try to add collider to this node
    if ( quadTree[ nodeInd ].isLeaf ) {
      // ... if there is still space
      if ( quadTree[ nodeInd ].elements.lastInd < QuadBucket::CAPACITY - 1 ) {
        quadTree[ nodeInd ].elements._[ ++quadTree[ nodeInd ].elements.lastInd ] = colliderInd;
#ifndef NDEBUG
        inserted = true;
#endif
      } else { 
        // if there is no space this cannot be a leaf any more
        subdivideQuadNode( nodeInd );
      }
    }
    if ( !quadTree[ nodeInd ].isLeaf ) {
      // find which children the collider intersects with
      // and add them to the deque
      for ( int i = 0; i < 4; ++i ) {
        u32 childInd = quadTree[ nodeInd ].childIndices[ i ];
        QuadNode child = quadTree[ childInd ];
        if ( collide( collider, child.boundary ) ) {
          nextNodeInds.push_front( childInd );
        }
      }
    }
  }
  ASSERT( inserted, "Collider index %d not inserted into QuadTree", colliderInd ); 
}

void ColliderManager::initialize() {
}

void ColliderManager::shutdown() {
}

void ColliderManager::addCircle( EntityHandle entity, Circle circleCollider ) {
  ASSERT( circleCollider.radius > 0.0f, "A circle collider of radius %f is useless", circleCollider.radius );
  ColliderComp comp {};
  comp.entity = entity;
  comp._.circle = circleCollider;
  comp._.type = ShapeType::CIRCLE;
  componentMap.set( entity, comp, &ColliderManager::remove );
}

void ColliderManager::addAxisAlignedRect( EntityHandle entity, Rect aaRectCollider ) {
  ASSERT( aaRectCollider.min.x < aaRectCollider.max.x &&
          aaRectCollider.min.y < aaRectCollider.max.y,
          "Malformed axis aligned rect collider" );
  ColliderComp comp {};
  comp.entity = entity;
  comp._.aaRect = aaRectCollider;
  comp._.type = ShapeType::AARECT;
  componentMap.set( entity, comp, &ColliderManager::remove );
}

void ColliderManager::remove( EntityHandle entity ) {
  componentMap.remove( entity );
}

void ColliderManager::updateAndCollide() {
  PROFILE;
  if ( componentMap.components.size() == 0 ) {
    return;
  }
  // update local transform cache
  std::vector< EntityHandle > updatedEntities = TransformManager::getLastUpdated();
  LookupResult colliderLookup;
  componentMap.lookup( updatedEntities, &colliderLookup );
  LookupResult transformLookup;
  TransformManager::lookup( colliderLookup.entities, &transformLookup );
  VALIDATE_ENTITIES_EQUAL( colliderLookup.entities, transformLookup.entities );
  // FIXME get world transforms here
  std::vector< Transform > updatedTransforms;
  TransformManager::get( transformLookup.indices, &updatedTransforms );
  for ( u32 trInd = 0; trInd < updatedTransforms.size(); ++trInd ) {
    Transform transform = updatedTransforms[ trInd ];
    ComponentIndex colliderCompInd = colliderLookup.indices[ trInd ];
    componentMap.components[ colliderCompInd ].position = transform.position;
    componentMap.components[ colliderCompInd ].scale = transform.scale;
  }
  transformedShapes.clear();
  transformedShapes.reserve( componentMap.components.size() );
  transformedShapes.push_back( {} );
  for ( u32 colInd = 1; colInd < componentMap.components.size(); ++colInd ) {
    ColliderComp colliderComp = componentMap.components[ colInd ];
    if ( colliderComp._.type == ShapeType::CIRCLE ) {
      float scaleX = colliderComp.scale.x, scaleY = colliderComp.scale.y;
      float maxScale = ( scaleX > scaleY ) ? scaleX : scaleY;
      Vec2 position = colliderComp.position + colliderComp._.circle.center * maxScale;
      float radius = colliderComp._.circle.radius * maxScale;
      transformedShapes.push_back( { { position, radius }, ShapeType::CIRCLE } );
    } else if ( colliderComp._.type == ShapeType::AARECT ) {
      Vec2 min = colliderComp._.aaRect.min * colliderComp.scale + colliderComp.position;
      Vec2 max = colliderComp._.aaRect.max * colliderComp.scale + colliderComp.position;
      Shape transformed = { {}, ShapeType::AARECT };
      transformed.aaRect = { min, max };
      transformedShapes.push_back( transformed );
    }
  }

  // space partitioned collision detection
  // keep the quadtree updated
  // TODO calculate the boundary dynamically 
  Rect boundary = { { -420, -240 }, { 420, 240 } };
  buildQuadTree( boundary );
  
  // detect collisions
  collisions.clear();
  collisions.resize( componentMap.components.size(), {} );
  for ( u32 nodeInd = 1; nodeInd < quadTree.size(); ++nodeInd ) {
    QuadNode quadNode = quadTree[ nodeInd ];
    if ( !quadNode.isLeaf ) {
      continue;
    }
    for ( int i = 0; i < quadNode.elements.lastInd; ++i ) {
      ComponentIndex collI = quadNode.elements._[ i ];
      Shape shapeI = transformedShapes[ collI ];
      for ( int j = i + 1; j <= quadNode.elements.lastInd; ++j ) {
        ComponentIndex collJ = quadNode.elements._[ j ];
        Shape shapeJ = transformedShapes[ collJ ];
        Collision collision;
        if ( collide( shapeI, shapeJ, collision ) ) {
          collisions[ collI ].push_back( collision );
          collisions[ collJ ].push_back( { collision.b, collision.a, collision.normalB, collision.normalA } );
          Debug::drawShape( shapeI, Debug::GREEN );
          Debug::drawShape( shapeJ, Debug::GREEN );
        }
      }
    }
  }
}

void ColliderManager::lookup( const std::vector< EntityHandle >& entities, LookupResult* result ) {
  return componentMap.lookup( entities, result );
}

std::vector< std::vector< Collision > >& ColliderManager::getCollisions( const std::vector< ComponentIndex >& indices ) {
  PROFILE;
  static std::vector< std::vector< Collision > > requestedCollisions;
  requestedCollisions.clear();
  requestedCollisions.reserve( indices.size() );
  for ( u32 entI = 0; entI < indices.size(); ++entI ) {
    ComponentIndex compInd = indices[ entI ];
    requestedCollisions.push_back( collisions[ compInd ] );
  }
  return requestedCollisions;
}

bool ColliderManager::collide( Shape shapeA, Shape shapeB ) {
  PROFILE;
  switch ( shapeA.type ) {
  case ShapeType::CIRCLE:
    switch ( shapeB.type ) {
    case ShapeType::CIRCLE:
      return circleCircleCollide( shapeA.circle, shapeB.circle );
    case ShapeType::AARECT:
      return aaRectCircleCollide( shapeB.aaRect, shapeA.circle );
    }
  case ShapeType::AARECT:
    switch ( shapeB.type ) {
    case ShapeType::CIRCLE:
      return aaRectCircleCollide( shapeA.aaRect, shapeB.circle );
    case ShapeType::AARECT:
      return aaRectAARectCollide( shapeA.aaRect, shapeB.aaRect );
    }
  }
  return false;
}

bool ColliderManager::collide( Shape shapeA, Shape shapeB, Collision& collision ) {
  PROFILE;
  switch ( shapeA.type ) {
  case ShapeType::CIRCLE:
    switch ( shapeB.type ) {
    case ShapeType::CIRCLE:
      collision.a = shapeA;
      collision.b = shapeB;
      return circleCircleCollide( shapeA.circle, shapeB.circle, collision.normalA, collision.normalB );
    case ShapeType::AARECT:
      collision.a = shapeB;
      collision.b = shapeA;
      return aaRectCircleCollide( shapeB.aaRect, shapeA.circle, collision.normalB, collision.normalA );
    }
  case ShapeType::AARECT:
    collision.a = shapeA;
    collision.b = shapeB;    
    switch ( shapeB.type ) {
    case ShapeType::CIRCLE:
      return aaRectCircleCollide( shapeA.aaRect, shapeB.circle, collision.normalA, collision.normalB );
    case ShapeType::AARECT:
      return aaRectAARectCollide( shapeA.aaRect, shapeB.aaRect, collision.normalA, collision.normalB );
    }
  }
  return false;
}
 
 // FIXME take scale into account
bool ColliderManager::circleCircleCollide( Circle circleA, Circle circleB ) {
  PROFILE;
  float radiiSum = circleA.radius + circleB.radius;
  return sqrMagnitude( circleA.center - circleB.center ) <= radiiSum * radiiSum;
}

// FIXME take scale into account
bool ColliderManager::circleCircleCollide( Circle circleA, Circle circleB, Vec2& normalA, Vec2& normalB ) {
  PROFILE;
  Vec2 ab = circleB.center - circleA.center;
  normalA = normalized( ab );
  normalB = -normalA;

  float radiiSum = circleA.radius + circleB.radius;
  return sqrMagnitude( ab ) <= radiiSum * radiiSum;
}

// FIXME take scale into account
bool ColliderManager::aaRectCircleCollide( Rect aaRect, Circle circle ) {
  PROFILE;
  // TODO assert integrity of circle and aaRect
  // TODO refactor distance to aaRect function
  // taken from Real-Time Collision Detection - Christer Ericson, 5.2.5 Testing Sphere Against AARECT
  float sqrDistance = 0.0f;
  if ( circle.center.x < aaRect.min.x ) {
    sqrDistance += ( aaRect.min.x - circle.center.x ) * ( aaRect.min.x - circle.center.x );
  }
  if ( circle.center.x > aaRect.max.x ) {
    sqrDistance += ( circle.center.x - aaRect.max.x ) * ( circle.center.x - aaRect.max.x );
  }
  if ( circle.center.y < aaRect.min.y ) {
    sqrDistance += ( aaRect.min.y - circle.center.y ) * ( aaRect.min.y - circle.center.y );
  }
  if ( circle.center.y > aaRect.max.y ) {
    sqrDistance += ( circle.center.y - aaRect.max.y ) * ( circle.center.y - aaRect.max.y );
  }
  return sqrDistance <= circle.radius * circle.radius; 
}

// FIXME take scale into account
bool ColliderManager::aaRectCircleCollide( Rect aaRect, Circle circle, Vec2& normalA, Vec2& normalB ) {
  PROFILE;
  // TODO assert integrity of circle and aaRect
  // taken from Real-Time Collision Detection - Christer Ericson, 5.2.5 Testing Sphere Against AARECT
  normalA = {};
  float x = circle.center.x;
  if ( x < aaRect.min.x ) {
    x = aaRect.min.x;
    normalA.x = -1.0;
  }
  if ( x > aaRect.max.x ) {
    x = aaRect.max.x;
    normalA.x = 1.0;
  }
  float y = circle.center.y;
  if ( y < aaRect.min.y ) {
    y = aaRect.min.y;
    normalA.y = -1.0;
  }
  if ( y > aaRect.max.y ) {
    y = aaRect.max.y;
    normalA.y = 1.0;
  }
  normalA = normalized( normalA );
  Vec2 closestPtInRect = { x, y };
  Vec2 circleToRect = closestPtInRect - circle.center;
  normalB = normalized( circleToRect );

  return sqrMagnitude( circleToRect ) <= circle.radius * circle.radius;
}

bool ColliderManager::aaRectAARectCollide( Rect aaRectA, Rect aaRectB ) {
  PROFILE;
  bool xOverlap = aaRectA.min.x <= aaRectB.max.x && aaRectA.max.x >= aaRectB.min.x;
  bool yOverlap = aaRectA.min.y <= aaRectB.max.y && aaRectA.max.y >= aaRectB.min.y;
  return xOverlap && yOverlap;
}

// TODO implement
bool ColliderManager::aaRectAARectCollide( Rect aaRectA, Rect aaRectB, Vec2& normalA, Vec2& normalB ) {
  aaRectA = aaRectB = {};
  normalA = normalB = {};
  return false;
}

void ColliderManager::fitCircleToSprite( EntityHandle entity ) {
  std::vector< EntityHandle > entities = { entity };
  LookupResult lookupResult;
  SpriteManager::lookup( entities, &lookupResult );
  VALIDATE_ENTITIES_EQUAL( entities, lookupResult.entities );
  std::vector< Sprite > sprites;
  SpriteManager::get( lookupResult.indices, &sprites );
  Vec2 size = sprites[ 0 ].size;
  float maxSize = ( size.x > size.y ) ? size.x : size.y;
  Circle circleCollider = { {}, maxSize / 2.0f };
  addCircle( entity, circleCollider );
}

ComponentMap< SolidBodyManager::SolidBodyComp > SolidBodyManager::componentMap;

void SolidBodyManager::initialize() {
}

void SolidBodyManager::shutdown() {
}

void SolidBodyManager::set( EntityHandle entity, SolidBody solidBody ) {
  componentMap.set( entity, { solidBody.speed, entity }, &SolidBodyManager::remove );
}

void SolidBodyManager::remove( EntityHandle entity ) {
  componentMap.remove( entity );
}

void SolidBodyManager::setSpeed( const std::vector< ComponentIndex >& indices, std::vector< Vec2 >& speeds ) {
  PROFILE;
  ASSERT( indices.size() == speeds.size(), "" );
  for ( u32 i = 0; i < indices.size(); ++i ) {
    componentMap.components[ indices[ i ] ].speed = speeds[ i ];
  }
}

void SolidBodyManager::get( const std::vector< ComponentIndex >& indices, std::vector< SolidBody >* result ) {
  result->reserve( indices.size() );
  for ( u32 i = 0; i < indices.size(); ++i ) {
    SolidBody solidBody = { componentMap.components[ indices[ i ] ].speed };
    result->push_back( solidBody );
  }
}

void SolidBodyManager::update( double deltaT ) {
  PROFILE;
  std::vector< EntityHandle > entities;
  entities.reserve( componentMap.components.size() );
  // detect collisions and correct positions
  for ( u32 compI = 1; compI < componentMap.components.size(); ++compI ) {
    SolidBodyComp solidBodyComp = componentMap.components[ compI ];
    entities.push_back( solidBodyComp.entity );
  }
  LookupResult colliderLookup;
  ColliderManager::lookup( entities, &colliderLookup );
  VALIDATE_ENTITIES_EQUAL( entities, colliderLookup.entities );
  std::vector< std::vector< Collision > > collisions = ColliderManager::getCollisions( colliderLookup.indices );
  // move solid bodies
  std::vector< Vec2 > translations;
  translations.reserve( componentMap.components.size() );
  for ( u32 compI = 1; compI < componentMap.components.size(); ++compI ) {
    std::vector< Collision > collisionsI = collisions[ compI - 1 ];
    Vec2 normal = {};
    SolidBodyComp solidBodyComp = componentMap.components[ compI ];
    for ( u32 i = 0; i < collisionsI.size(); ++i ) {
      if ( dot( solidBodyComp.speed, collisionsI[ i ].normalB ) <= 0.0f ) {
        normal += collisionsI[ i ].normalB;
      }
    }
    if ( normal.x != 0 || normal.y != 0 ) {
      // reflect direction
      normal = normalized( normal );
      float vDotN = dot( solidBodyComp.speed, normal );
      componentMap.components[ compI ].speed = solidBodyComp.speed - 2.0f * vDotN * normal;
    }
    translations.push_back( solidBodyComp.speed * deltaT );
  }
  LookupResult transformLookup;
  TransformManager::lookup( entities, &transformLookup );
  VALIDATE_ENTITIES_EQUAL( entities, transformLookup.entities );
  TransformManager::translate( transformLookup.indices, translations );
}

void SolidBodyManager::lookup( const std::vector< EntityHandle >& entities, LookupResult* result ) {
  return componentMap.lookup( entities, result );
}

ComponentMap< SpriteManager::SpriteComp > SpriteManager::componentMap;
RenderInfo SpriteManager::renderInfo;
SpriteManager::Pos* SpriteManager::posBufferData;
SpriteManager::UV* SpriteManager::texCoordsBufferData;
 
SpriteManager::SpriteComp::operator Sprite() const {
  return { this->sprite.textureId, this->sprite.texCoords, this->sprite.size };
}

void SpriteManager::initialize() {
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

void SpriteManager::shutdown() {
  glDeleteProgram( renderInfo.shaderProgramId );
  glDeleteVertexArrays( 1, &renderInfo.vaoId );
  glDeleteBuffers( 2, renderInfo.vboIds );
}

void SpriteManager::set( EntityHandle entity, AssetIndex textureId, Rect texCoords ) {
  ASSERT( AssetManager::isTextureAlive( textureId ), "Invalid texture id %d", textureId ); 
  SpriteComp spriteComp = {};
  spriteComp.entity = entity;
  spriteComp.sprite.textureId = textureId;
  spriteComp.sprite.texCoords = texCoords;
  TextureAsset texture = AssetManager::getTexture( textureId );
  float width = texture.width * ( texCoords.max.u - texCoords.min.u ) / PIXELS_PER_UNIT;
  float height = texture.height * ( texCoords.max.v - texCoords.min.v ) / PIXELS_PER_UNIT;
  spriteComp.sprite.size = { width, height };
  componentMap.set( entity, spriteComp, &SpriteManager::remove );
}

void SpriteManager::remove( EntityHandle entity ) {
  componentMap.remove( entity );
}

void SpriteManager::get( const std::vector< ComponentIndex >& indices, std::vector< Sprite >* result ) {
  result->reserve( indices.size() );
  for ( u32 i = 0; i < indices.size(); ++i ) {
    result->push_back( static_cast< Sprite >( componentMap.components[ indices[ i ] ] ) );
  }
}

void SpriteManager::setOrthoProjection( float aspectRatio, float height ) {
  float halfHeight = height / 2.0f;
  glUseProgram( renderInfo.shaderProgramId );
  glUniform1f( renderInfo.projUnifLoc[ 0 ], -halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 1 ], halfHeight * aspectRatio );
  glUniform1f( renderInfo.projUnifLoc[ 2 ], -halfHeight );
  glUniform1f( renderInfo.projUnifLoc[ 3 ], halfHeight );
}

void SpriteManager::updateAndRender() {
  PROFILE;
  if ( componentMap.components.size() == 0 ) {
    return;
  }
  // update local transform cache
  std::vector< EntityHandle > updatedEntities = TransformManager::getLastUpdated();
  LookupResult spriteLookup;
  componentMap.lookup( updatedEntities, &spriteLookup );
  LookupResult transformLookup;
  TransformManager::lookup( spriteLookup.entities, &transformLookup );
  VALIDATE_ENTITIES_EQUAL( spriteLookup.entities, transformLookup.entities );
  // TODO get world transforms here
  std::vector< Transform > updatedTransforms;
  TransformManager::get( transformLookup.indices, &updatedTransforms );
  for ( u32 trInd = 0; trInd < updatedTransforms.size(); ++trInd ) {
    ComponentIndex spriteInd = spriteLookup.indices[ trInd ];
    componentMap.components[ spriteInd ].transform = updatedTransforms[ trInd ];
  }
  // build vertex buffer and render for sprites with same texture
  glUseProgram( renderInfo.shaderProgramId );
  glBindVertexArray( renderInfo.vaoId );
  // TODO don't render every sprite every time
  u32 spritesToRenderCount = componentMap.components.size();
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
    SpriteComp spriteComp = componentMap.components[ spriteInd ];
    for ( u32 vertInd = 0; vertInd < vertsPerSprite; ++vertInd ) {
      Vec2 vert = baseGeometry[ vertInd ] * spriteComp.sprite.size * spriteComp.transform.scale;
      vert = rotateVec2( vert, spriteComp.transform.orientation );
      vert += spriteComp.transform.position;
      posBufferData[ spriteInd * vertsPerSprite + vertInd ].pos = vert;
    }
  } 
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glBufferData( GL_ARRAY_BUFFER, spritesToRenderCount * vertsPerSprite * sizeof( Pos ), posBufferData, GL_STATIC_DRAW );
  // TODO measure how expensive these allocations and deallocations are!
  delete[] posBufferData;
  // build the texture coordinates buffer
  for ( u32 spriteInd = 0; spriteInd < spritesToRenderCount; ++spriteInd ) {
    SpriteComp spriteComp = componentMap.components[ spriteInd ];
    Vec2 max = spriteComp.sprite.texCoords.max;
    Vec2 min = spriteComp.sprite.texCoords.min;
    Vec2 texCoords[] = {
      min, max, { min.u, max.v },
      min, { max.u, min.v }, max
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
  u32 currentTexId = componentMap.components[ 0 ].sprite.textureId;  
  ASSERT( AssetManager::isTextureAlive( currentTexId ), "Invalid texture id %d", currentTexId );  
  u32 currentTexGlId = AssetManager::getTexture( currentTexId ).glId;
  glBindTexture( GL_TEXTURE_2D, currentTexGlId );
  // mark where a sub-buffer with sprites sharing a texture ends and a new one begins
  u32 currentSubBufferStart = 0;
  for ( u32 spriteInd = 1; spriteInd < spritesToRenderCount; ++spriteInd ) {
    if ( componentMap.components[ spriteInd ].sprite.textureId != currentTexId ) {
      // send current vertex sub-buffer and render it
      u32 spriteCountInSubBuffer = spriteInd - currentSubBufferStart;
      glDrawArrays( GL_TRIANGLES, vertsPerSprite * currentSubBufferStart, vertsPerSprite * spriteCountInSubBuffer );
      // and start a new one
      currentTexId = componentMap.components[ spriteInd ].sprite.textureId;      
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

void SpriteManager::lookup( const std::vector< EntityHandle >& entities, LookupResult* result ) {
  return componentMap.lookup( entities, result );
}
