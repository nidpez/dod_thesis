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

ComponentMap< ColliderManager::ColliderComp > ColliderManager::componentMap[ ShapeType::LAST ];
std::vector< EntityHandle > ColliderManager::entities[ ShapeType::LAST ];
std::vector< Shape > ColliderManager::transformedShapes[ ShapeType::LAST ];
std::array< std::vector< std::vector< Collision > >, ShapeType::LAST > ColliderManager::collisions;
std::vector< ColliderManager::QuadNode > ColliderManager::quadTree;

void ColliderManager::buildQuadTree( Rect boundary ) {
  PROFILE;
  quadTree.clear();
  quadTree.push_back( {} );
  QuadNode rootNode = {};
  rootNode.boundary.aaRect = boundary;
  quadTree.push_back( rootNode );
  std::vector< ComponentIndex > colliderInds[ ShapeType::LAST ];
  for ( u8 shapeType = 0; shapeType < ShapeType::LAST; ++shapeType ) {
    colliderInds[ shapeType ].reserve( componentMap[ shapeType ].components.size() );
    for ( u32 colliderInd = 1; colliderInd < componentMap[ shapeType ].components.size(); ++colliderInd ) {
      colliderInds[ shapeType ].push_back( colliderInd );
    }
  }
  insertIntoQuadTree( colliderInds );
  // debug render space partitions 
  for ( u32 nodeInd = 1; nodeInd < quadTree.size(); ++nodeInd ) {
    QuadNode quadNode = quadTree[ nodeInd ];
    if ( !quadNode.isLeaf ) {
      continue;
    }
    Debug::drawRect( quadNode.boundary.aaRect, { 1, 1, 1, 0.3f } );
    for ( int i = 0; i < quadNode.elements.size[ ShapeType::CIRCLE ]; ++i ) {
      ComponentIndex ci = quadNode.elements._[ ShapeType::CIRCLE ][ i ];
      Debug::drawCircle( transformedShapes[ ShapeType::CIRCLE ][ ci ].circle, Debug::BLUE );
    }
    for ( int i = 0; i < quadNode.elements.size[ ShapeType::AARECT ]; ++i ) {
      ComponentIndex ci = quadNode.elements._[ ShapeType::AARECT ][ i ];
      Debug::drawRect( transformedShapes[ ShapeType::AARECT ][ ci ].aaRect, Debug::BLUE );
    }
  }
}

// FIXME if QuadBucket::CAPACITY + 1 colliders are at the center of the node
//       we will subdivide it forever
void ColliderManager::subdivideQuadNode( u32 nodeInd ) {
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
  bool ( *collisionFunctions[ ShapeType::LAST ] )( Shape a, Shape b );
  collisionFunctions[ ShapeType::CIRCLE ] = &aaRectCircleCollide;
  collisionFunctions[ ShapeType::AARECT ] = &aaRectAARectCollide;
  for ( u8 shapeType = 0; shapeType < ShapeType::LAST; ++shapeType ) {
    bool ( *collide )( Shape a, Shape b ) = collisionFunctions[ shapeType ];
    for ( int elemInd = 0; elemInd < elements.size[ shapeType ]; ++elemInd ) {
      ComponentIndex colliderInd = elements._[ shapeType ][ elemInd ];
      Shape collider = transformedShapes[ shapeType ][ colliderInd ];
      for ( int childI = 0; childI < 4; ++childI ) {
        QuadNode& child = quadTree[ quadTree[ nodeInd ].childIndices[ childI ] ];
        if ( collide( child.boundary, collider ) ) {
          child.elements._[ shapeType ][ child.elements.size[ shapeType ]++ ] = colliderInd;
          ++child.elements.totalSize;
        }
      }
    }
  }
}

void ColliderManager::insertIntoQuadTree( std::vector< ComponentIndex > colliderInds[ ShapeType::LAST ] ) {
  PROFILE;
  bool ( *collisionFunctions[ ShapeType::LAST ] )( Shape a, Shape b );
  collisionFunctions[ ShapeType::CIRCLE ] = &aaRectCircleCollide;
  collisionFunctions[ ShapeType::AARECT ] = &aaRectAARectCollide;
  
  std::deque< u32 > nextNodeInds;
  for ( u8 shapeType = 0; shapeType < ShapeType::LAST; ++shapeType ) {
    bool ( *collide )( Shape a, Shape b ) = collisionFunctions[ shapeType ];
    
    for ( u32 colI = 0; colI < colliderInds[ shapeType ].size(); ++colI ) {
      ComponentIndex colliderInd = colliderInds[ shapeType ][ colI ];
      ASSERT( colliderInd > 0 && colliderInd < componentMap[ shapeType ].components.size(),
              "Component index %d out of bounds", colliderInd );
      nextNodeInds.clear();
      // index 0 is null
      Shape collider = transformedShapes[ shapeType ][ colliderInd ];
      if ( collide( quadTree[ 1 ].boundary, collider ) ) {
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
          if ( quadTree[ nodeInd ].elements.totalSize < QuadBucket::CAPACITY ) {
            quadTree[ nodeInd ].elements._[ shapeType ][ quadTree[ nodeInd ].elements.size[ shapeType ]++ ] = colliderInd;
            ++quadTree[ nodeInd ].elements.totalSize;
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
            if ( collide( child.boundary, collider ) ) {
              nextNodeInds.push_front( childInd );
            }
          }
        }
      }
      ASSERT( inserted, "Collider index %d not inserted into QuadTree", colliderInd ); 
    }
  }
}

void ColliderManager::initialize() {
}

void ColliderManager::shutdown() {
}

void ColliderManager::addCircle( EntityHandle entity, Circle circleCollider ) {
  ASSERT( circleCollider.radius > 0.0f, "A circle collider of radius %f is useless", circleCollider.radius );
  ColliderComp comp {};
  comp._.circle = circleCollider;
  comp._.type = ShapeType::CIRCLE;
  componentMap[ ShapeType::CIRCLE ].set( entity, comp, &ColliderManager::remove );
  entities[ ShapeType::CIRCLE ].push_back( entity );
}

void ColliderManager::addAxisAlignedRect( EntityHandle entity, Rect aaRectCollider ) {
  ASSERT( aaRectCollider.min.x < aaRectCollider.max.x &&
          aaRectCollider.min.y < aaRectCollider.max.y,
          "Malformed axis aligned rect collider" );
  ColliderComp comp {};
  comp._.aaRect = aaRectCollider;
  comp._.type = ShapeType::AARECT;
  componentMap[ ShapeType::AARECT ].set( entity, comp, &ColliderManager::remove );
  entities[ ShapeType::AARECT ].push_back( entity );
}

void ColliderManager::remove( EntityHandle entity ) {
  UNUSED( entity );
  // for ( u8 shapeType = 0; shapeType < ShapeType::LAST; ++shapeType ) {
  //   componentMap[ shapeType ].remove( entity );
  // }
}

void ColliderManager::updateAndCollide() {
  PROFILE;
  // update local transform cache
  for ( u8 shapeType = 0; shapeType < ShapeType::LAST; ++shapeType ) {
    std::vector< EntityHandle > _entities = entities[ shapeType ];
    LookupResult colliderLookup;
    componentMap[ shapeType ].lookup( _entities, &colliderLookup );
    LookupResult transformLookup;
    TransformManager::lookup( colliderLookup.entities, &transformLookup );
    VALIDATE_ENTITIES_EQUAL( colliderLookup.entities, transformLookup.entities );
    // FIXME get world transforms here
    std::vector< Transform > updatedTransforms;
    TransformManager::get( transformLookup.indices, &updatedTransforms );
    for ( u32 trInd = 0; trInd < updatedTransforms.size(); ++trInd ) {
      Transform transform = updatedTransforms[ trInd ];
      ComponentIndex colliderCompInd = colliderLookup.indices[ trInd ];
      componentMap[ shapeType ].components[ colliderCompInd ].position = transform.position;
      componentMap[ shapeType ].components[ colliderCompInd ].scale = transform.scale;
    }
  }

  // update transformed shapes
  for ( u8 shapeType = 0; shapeType < ShapeType::LAST; ++shapeType ) {
    transformedShapes[ shapeType ].clear();
    transformedShapes[ shapeType ].reserve( componentMap[ shapeType ].components.size() );
    transformedShapes[ shapeType ].push_back( {} );
  }
  for ( u32 colInd = 1; colInd < componentMap[ ShapeType::CIRCLE ].components.size(); ++colInd ) {
    ColliderComp colliderComp = componentMap[ ShapeType::CIRCLE ].components[ colInd ];
    float scaleX = colliderComp.scale.x, scaleY = colliderComp.scale.y;
    float maxScale = ( scaleX > scaleY ) ? scaleX : scaleY;
    Vec2 position = colliderComp.position + colliderComp._.circle.center * maxScale;
    float radius = colliderComp._.circle.radius * maxScale;
    transformedShapes[ ShapeType::CIRCLE ].push_back( { { position, radius }, ShapeType::CIRCLE } );
  }
  for ( u32 colInd = 1; colInd < componentMap[ ShapeType::AARECT ].components.size(); ++colInd ) {
    ColliderComp colliderComp = componentMap[ ShapeType::AARECT ].components[ colInd ];
    Vec2 min = colliderComp._.aaRect.min * colliderComp.scale + colliderComp.position;
    Vec2 max = colliderComp._.aaRect.max * colliderComp.scale + colliderComp.position;
    Shape transformed = { {}, ShapeType::AARECT };
    transformed.aaRect = { min, max };
    transformedShapes[ ShapeType::AARECT ].push_back( transformed );
  }

  // space partitioned collision detection
  // keep the quadtree updated
  // TODO calculate the boundary dynamically 
  Rect boundary = { { -420, -240 }, { 420, 240 } };
  buildQuadTree( boundary );
  
  // detect collisions
  for ( u8 shapeType = 0; shapeType < ShapeType::LAST; ++shapeType ) {
    collisions[ shapeType ].clear();
    collisions[ shapeType ].resize( componentMap[ shapeType ].components.size(), {} );
  }
  for ( u32 nodeInd = 1; nodeInd < quadTree.size(); ++nodeInd ) {
    QuadNode quadNode = quadTree[ nodeInd ];
    if ( !quadNode.isLeaf ) {
      continue;
    }
    for ( int i = 0; i < quadNode.elements.size[ ShapeType::CIRCLE ] - 1; ++i ) {
      ComponentIndex collI = quadNode.elements._[ ShapeType::CIRCLE ][ i ];
      Shape shapeI = transformedShapes[ ShapeType::CIRCLE ][ collI ];
      for ( int j = i + 1; j < quadNode.elements.size[ ShapeType::CIRCLE ]; ++j ) {
        ComponentIndex collJ = quadNode.elements._[ ShapeType::CIRCLE ][ j ];
        Shape shapeJ = transformedShapes[ ShapeType::CIRCLE ][ collJ ];
        Collision collision;
        collision.a = shapeI;
        collision.b = shapeJ;
        if ( circleCircleCollide( shapeI, shapeJ, collision.normalA, collision.normalB ) ) {
          collisions[ ShapeType::CIRCLE ][ collI ].push_back( collision );
          collisions[ ShapeType::CIRCLE ][ collJ ].push_back( { collision.b, collision.a, collision.normalB, collision.normalA } );
          Debug::drawCircle( shapeI.circle, Debug::GREEN );
          Debug::drawCircle( shapeJ.circle, Debug::GREEN );
        }
      }
    }
    for ( int i = 0; i < quadNode.elements.size[ ShapeType::AARECT ] - 1; ++i ) {
      ComponentIndex collI = quadNode.elements._[ ShapeType::AARECT ][ i ];
      Shape shapeI = transformedShapes[ ShapeType::AARECT ][ collI ];
      for ( int j = i + 1; j < quadNode.elements.size[ ShapeType::AARECT ]; ++j ) {
        ComponentIndex collJ = quadNode.elements._[ ShapeType::AARECT ][ j ];
        Shape shapeJ = transformedShapes[ ShapeType::AARECT ][ collJ ];
        Collision collision;
        collision.a = shapeI;
        collision.b = shapeJ;
        if ( aaRectAARectCollide( shapeI, shapeJ, collision.normalA, collision.normalB ) ) {
          collisions[ ShapeType::AARECT ][ collI ].push_back( collision );
          collisions[ ShapeType::AARECT ][ collJ ].push_back( { collision.b, collision.a, collision.normalB, collision.normalA } );
          Debug::drawRect( shapeI.aaRect, Debug::GREEN );
          Debug::drawRect( shapeJ.aaRect, Debug::GREEN );
        }
      }
    }
    for ( int i = 0; i < quadNode.elements.size[ ShapeType::AARECT ]; ++i ) {
      ComponentIndex collI = quadNode.elements._[ ShapeType::AARECT ][ i ];
      Shape shapeI = transformedShapes[ ShapeType::AARECT ][ collI ];
      for ( int j = 0; j < quadNode.elements.size[ ShapeType::CIRCLE ]; ++j ) {
        ComponentIndex collJ = quadNode.elements._[ ShapeType::CIRCLE ][ j ];
        Shape shapeJ = transformedShapes[ ShapeType::CIRCLE ][ collJ ];
        Collision collision;
        collision.a = shapeI;
        collision.b = shapeJ;
        if ( aaRectCircleCollide( shapeI, shapeJ, collision.normalA, collision.normalB ) ) {
          collisions[ ShapeType::AARECT ][ collI ].push_back( collision );
          collisions[ ShapeType::CIRCLE ][ collJ ].push_back( { collision.b, collision.a, collision.normalB, collision.normalA } );
          Debug::drawRect( shapeI.aaRect, Debug::GREEN );
          Debug::drawCircle( shapeJ.circle, Debug::GREEN );
        }
      }
    }
  }
}

// bool ColliderManager::collide( Shape shapeA, Shape shapeB, Collision& collision ) {
//   PROFILE;
//   switch ( shapeA.type ) {
//   case ShapeType::CIRCLE:
//     switch ( shapeB.type ) {
//     case ShapeType::CIRCLE:
//       collision.a = shapeA;
//       collision.b = shapeB;
//       return circleCircleCollide( shapeA.circle, shapeB.circle, collision.normalA, collision.normalB );
//     case ShapeType::AARECT:
//       collision.a = shapeB;
//       collision.b = shapeA;
//       return aaRectCircleCollide( shapeB.aaRect, shapeA.circle, collision.normalB, collision.normalA );
//     }
//   case ShapeType::AARECT:
//     collision.a = shapeA;
//     collision.b = shapeB;    
//     switch ( shapeB.type ) {
//     case ShapeType::CIRCLE:
//       return aaRectCircleCollide( shapeA.aaRect, shapeB.circle, collision.normalA, collision.normalB );
//     case ShapeType::AARECT:
//       return aaRectAARectCollide( shapeA.aaRect, shapeB.aaRect, collision.normalA, collision.normalB );
//     }
//   }
//   return false;
// }

void ColliderManager::lookup( const std::vector< EntityHandle >& entities, std::array< LookupResult, ShapeType::LAST >* result ) {
  for ( u8 shapeType = 0; shapeType < ShapeType::LAST; ++shapeType ) {
    componentMap[ shapeType ].lookup( entities, &( *result )[ shapeType ] );
  }
}

std::array< std::vector< std::vector< Collision > >, ShapeType::LAST >& ColliderManager::getCollisions() {
  return collisions;
}
 
 // FIXME take scale into account
bool ColliderManager::circleCircleCollide( Shape a, Shape b ) {
  PROFILE;
  ASSERT( a.type == ShapeType::CIRCLE && b.type == ShapeType::CIRCLE, "" );
  float radiiSum = a.circle.radius + b.circle.radius;
  return sqrMagnitude( a.circle.center - a.circle.center ) <= radiiSum * radiiSum;
}

// FIXME take scale into account
bool ColliderManager::circleCircleCollide( Shape a, Shape b, Vec2& normalA, Vec2& normalB ) {
  PROFILE;
  ASSERT( a.type == ShapeType::CIRCLE && b.type == ShapeType::CIRCLE, "" );
  Vec2 ab = b.circle.center - a.circle.center;
  normalA = normalized( ab );
  normalB = -normalA;

  float radiiSum = a.circle.radius + b.circle.radius;
  return sqrMagnitude( ab ) <= radiiSum * radiiSum;
}

// FIXME take scale into account
bool ColliderManager::aaRectCircleCollide( Shape a, Shape b ) {
  PROFILE;
  ASSERT( a.type == ShapeType::AARECT && b.type == ShapeType::CIRCLE, "" );
  // TODO assert integrity of circle and aaRect
  // TODO refactor distance to aaRect function
  // taken from Real-Time Collision Detection - Christer Ericson, 5.2.5 Testing Sphere Against AARECT
  float sqrDistance = 0.0f;
  if ( b.circle.center.x < a.aaRect.min.x ) {
    sqrDistance += ( a.aaRect.min.x - b.circle.center.x ) * ( a.aaRect.min.x - b.circle.center.x );
  }
  if ( b.circle.center.x > a.aaRect.max.x ) {
    sqrDistance += ( b.circle.center.x - a.aaRect.max.x ) * ( b.circle.center.x - a.aaRect.max.x );
  }
  if ( b.circle.center.y < a.aaRect.min.y ) {
    sqrDistance += ( a.aaRect.min.y - b.circle.center.y ) * ( a.aaRect.min.y - b.circle.center.y );
  }
  if ( b.circle.center.y > a.aaRect.max.y ) {
    sqrDistance += ( b.circle.center.y - a.aaRect.max.y ) * ( b.circle.center.y - a.aaRect.max.y );
  }
  return sqrDistance <= b.circle.radius * b.circle.radius; 
}

// FIXME take scale into account
bool ColliderManager::aaRectCircleCollide( Shape a, Shape b, Vec2& normalA, Vec2& normalB ) {
  PROFILE;
  ASSERT( a.type == ShapeType::AARECT && b.type == ShapeType::CIRCLE, "" );
  // TODO assert integrity of circle and aaRect
  // taken from Real-Time Collision Detection - Christer Ericson, 5.2.5 Testing Sphere Against AARECT
  normalA = {};
  float x = b.circle.center.x;
  if ( x < a.aaRect.min.x ) {
    x = a.aaRect.min.x;
    normalA.x = -1.0;
  }
  if ( x > a.aaRect.max.x ) {
    x = a.aaRect.max.x;
    normalA.x = 1.0;
  }
  float y = b.circle.center.y;
  if ( y < a.aaRect.min.y ) {
    y = a.aaRect.min.y;
    normalA.y = -1.0;
  }
  if ( y > a.aaRect.max.y ) {
    y = a.aaRect.max.y;
    normalA.y = 1.0;
  }
  normalA = normalized( normalA );
  Vec2 closestPtInRect = { x, y };
  Vec2 circleToRect = closestPtInRect - b.circle.center;
  normalB = normalized( circleToRect );

  return sqrMagnitude( circleToRect ) <= b.circle.radius * b.circle.radius;
}

bool ColliderManager::aaRectAARectCollide( Shape a, Shape b ) {
  PROFILE;
  ASSERT( a.type == ShapeType::AARECT && b.type == ShapeType::AARECT, "" );
  bool xOverlap = a.aaRect.min.x <= b.aaRect.max.x && a.aaRect.max.x >= b.aaRect.min.x;
  bool yOverlap = a.aaRect.min.y <= b.aaRect.max.y && a.aaRect.max.y >= b.aaRect.min.y;
  return xOverlap && yOverlap;
}

// TODO implement
bool ColliderManager::aaRectAARectCollide( Shape a, Shape b, Vec2& normalA, Vec2& normalB ) {
  ASSERT( a.type == ShapeType::AARECT && b.type == ShapeType::AARECT, "" );
  a = b = {};
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
  std::array< LookupResult, ShapeType::LAST > colliderLookup;
  ColliderManager::lookup( entities, &colliderLookup );
  // TODO assert that all entities have at least one collider
  auto collisions = ColliderManager::getCollisions();
  // move solid bodies
  for ( u8 shapeType = 0; shapeType < ShapeType::LAST; ++shapeType ) {
    LookupResult solidBodyLookup;
    componentMap.lookup( colliderLookup[ shapeType ].entities, &solidBodyLookup );
    VALIDATE_ENTITIES_EQUAL( colliderLookup[ shapeType ].entities, solidBodyLookup.entities );
    std::vector< Vec2 > translations;
    translations.reserve( solidBodyLookup.entities.size() );
    for ( u32 compI = 0; compI < solidBodyLookup.indices.size(); ++compI ) {
      ComponentIndex compInd = solidBodyLookup.indices[ compI ];
      std::vector< Collision > collisionsI = collisions[ shapeType ][ compInd ];
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
    TransformManager::lookup( solidBodyLookup.entities, &transformLookup );
    VALIDATE_ENTITIES_EQUAL( solidBodyLookup.entities, transformLookup.entities );
    TransformManager::translate( transformLookup.indices, translations );
  }
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
