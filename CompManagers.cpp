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

void TransformManager::rotate( EntityHandle entity, float rotation ) {
  PROFILE;
  ComponentIndex componentInd = componentMap.lookup( entity );
  componentMap.components[ componentInd ].local.orientation += rotation;
  // TODO mark transform component as updated
}

void TransformManager::rotateAround( EntityHandle entity, Vec2 point, float rotation ) {
  PROFILE;
  ComponentIndex componentInd = componentMap.lookup( entity );
  Transform transform = componentMap.components[ componentInd ].local;
  transform.orientation += rotation;
  transform.position = rotateVec2( transform.position - point, rotation ) + point;
  componentMap.components[ componentInd ].local = transform;
  // TODO mark transform component as updated
}

void TransformManager::translate( EntityHandle entity, Vec2 translation ) {
  PROFILE;
  ComponentIndex componentInd = componentMap.lookup( entity );
  componentMap.components[ componentInd ].local.position += translation;
  // TODO mark transform component as updated
}

void TransformManager::scale( EntityHandle entity, Vec2 scale ) {
  PROFILE;
  ComponentIndex componentInd = componentMap.lookup( entity );
  componentMap.components[ componentInd ].local.scale = scale;
  // TODO mark transform component as updated
}

void TransformManager::update( EntityHandle entity, Transform transform ) {
  PROFILE;
  ComponentIndex componentInd = componentMap.lookup( entity );
  Transform local = componentMap.components[ componentInd ].local;
  local.position = transform.position;
  local.scale = transform.scale;
  local.orientation = transform.orientation;
  componentMap.components[ componentInd ].local = local;
  // TODO mark transform component as updated
}

Transform TransformManager::get( EntityHandle entity ) {
  PROFILE;
  ComponentIndex componentInd = componentMap.lookup( entity );
  return componentMap.components[ componentInd ].local;
}

std::vector< EntityHandle > TransformManager::getLastUpdated() {
  PROFILE;
  // TODO actually compute which transforms have been updated since last frame
  // component index 0 is not valid
  std::vector< EntityHandle > result( componentMap.components.size() - 1 );
  for ( u32 entInd = 1; entInd < componentMap.components.size(); ++entInd ) {
    result[ entInd - 1 ] = componentMap.components[ entInd ].entity;
  }
  return result;
}

ComponentMap< ColliderManager::ColliderComp > ColliderManager::componentMap;
std::vector< Shape > ColliderManager::transformedShapes;
std::vector< ColliderManager::QuadNode > ColliderManager::quadTree;

void ColliderManager::buildQuadTree(Rect boundary) {
  PROFILE;
  quadTree = std::vector< QuadNode >();
  quadTree.push_back( {} );
  QuadNode rootNode = {};
  rootNode.boundary.aaRect = boundary;
  quadTree.push_back( rootNode );
  for ( u32 colliderInd = 1; colliderInd < componentMap.components.size(); ++colliderInd ) {
    insertIntoQuadTree( colliderInd );
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
  bool inserted = false;
  while ( !nextNodeInds.empty() ) {
    u32 nodeInd = nextNodeInds.front();
    nextNodeInds.pop_front();
    // try to add collider to this node
    if ( quadTree[ nodeInd ].isLeaf ) {
      // ... if there is still space
      if ( quadTree[ nodeInd ].elements.lastInd < QuadBucket::CAPACITY - 1 ) {
        quadTree[ nodeInd ].elements._[ ++quadTree[ nodeInd ].elements.lastInd ] = colliderInd;
        inserted = true;
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
  ColliderComp comp { {}, {}, {}, entity };
  comp._.circle = circleCollider;
  comp._.type = ShapeType::CIRCLE;
  componentMap.set( entity, comp, &ColliderManager::remove );
}

void ColliderManager::addAxisAlignedRect( EntityHandle entity, Rect aaRectCollider ) {
  ColliderComp comp { {}, {}, {}, entity };
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
  updatedEntities = componentMap.have( updatedEntities );
  for ( u32 trInd = 0; trInd < updatedEntities.size(); ++trInd ) {
    // FIXME get world transforms here
    Transform transform = TransformManager::get( updatedEntities[ trInd ] );
    ComponentIndex colliderCompInd = componentMap.lookup( updatedEntities[ trInd ] );
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
  Rect boundary = { { -70, -40 }, { 70, 40 } };
  buildQuadTree( boundary );
  // debug render space partitions 
  for ( u32 nodeInd = 1; nodeInd < quadTree.size(); ++nodeInd ) {
    QuadNode quadNode = quadTree[ nodeInd ];
    if ( !quadNode.isLeaf ) {
      continue;
    }
    Debug::drawRect( quadNode.boundary.aaRect, { 0, 0, 1, 1 } );
    for ( int i = 0; i <= quadNode.elements.lastInd; ++i ) {
      ComponentIndex ci = quadNode.elements._[ i ];
      switch ( transformedShapes[ ci ].type ) {
      case ShapeType::CIRCLE:
        Debug::drawCircle( transformedShapes[ ci ].circle, { 0, 0, 1, 1 } );
        break;
      case ShapeType::AARECT:
        Debug::drawRect( transformedShapes[ ci ].aaRect, { 0, 0, 1, 1 } );
        break;
      }
    }
  }
  // detect collisions
  Color color = { 0.0f, 1.0f, 0.0f, 1.0f };
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
        if ( collide( shapeI, shapeJ ) ) {  
          switch ( transformedShapes[ collI ].type ) {
          case ShapeType::CIRCLE:
            Debug::drawCircle( transformedShapes[ collI ].circle, color );
            break;
          case ShapeType::AARECT:
            Debug::drawRect( transformedShapes[ collI ].aaRect, color );
            break;
          }
          switch ( transformedShapes[ collJ ].type ) {
          case ShapeType::CIRCLE:
            Debug::drawCircle( transformedShapes[ collJ ].circle, color );
            break;
          case ShapeType::AARECT:
            Debug::drawRect( transformedShapes[ collJ ].aaRect, color );
            break;
          }
          // TODO create reactions for dynamic components
          // TODO fire callbacks for programatic components
        }
      }
    }
  }
}

bool ColliderManager::collide( Shape shapeA, Shape shapeB ) {
  PROFILE;
  switch ( shapeA.type ) {
  case ShapeType::CIRCLE:
    switch ( shapeB.type ) {
    case ShapeType::CIRCLE:
      return circleCircleCollide( shapeA.circle, shapeB.circle );
    case ShapeType::AARECT:
      return circleAARectCollide( shapeA.circle, shapeB.aaRect );
    }
  case ShapeType::AARECT:
    switch ( shapeB.type ) {
    case ShapeType::CIRCLE:
      return circleAARectCollide( shapeB.circle, shapeA.aaRect );
    case ShapeType::AARECT:
      return aaRectAARectCollide( shapeA.aaRect, shapeB.aaRect );
    }
  }
  return false;
}

// FIXME take scale into account
bool ColliderManager::circleCircleCollide( Circle circleA, Circle circleB ) {
  PROFILE;
  // TODO use squared magnitude and compare to (ra + rb)^2
  float distance = magnitude( circleA.center - circleB.center );
  if ( distance <= circleA.radius + circleB.radius ) {
    return true;
  }
  return false;
}

// FIXME take scale into account
bool ColliderManager::circleAARectCollide( Circle circle, Rect aaRect ) {
  PROFILE;
  // TODO assert integrity of circle and aaRect
  // TODO refactor distance to aaRect function
  // taken from Real-Time Collision Detection - Christer Ericson, 5.2.5 Testing Sphere Against AARECT
  float sqDistance = 0.0f;
  if ( circle.center.x < aaRect.min.x ) {
    sqDistance += ( aaRect.min.x - circle.center.x ) * ( aaRect.min.x - circle.center.x );
  }
  if ( circle.center.x > aaRect.max.x ) {
    sqDistance += ( circle.center.x - aaRect.max.x ) * ( circle.center.x - aaRect.max.x );
  }
  if ( circle.center.y < aaRect.min.y ) {
    sqDistance += ( aaRect.min.y - circle.center.y ) * ( aaRect.min.y - circle.center.y );
  }
  if ( circle.center.y > aaRect.max.y ) {
    sqDistance += ( circle.center.y - aaRect.max.y ) * ( circle.center.y - aaRect.max.y );
  }
  return sqDistance <= circle.radius * circle.radius; 
}

bool ColliderManager::aaRectAARectCollide( Rect aaRectA, Rect aaRectB ) {
  aaRectA = aaRectB = {};
  return true;
}

void ColliderManager::fitCircleToSprite( EntityHandle entity ) {
  Vec2 size = SpriteManager::get( entity ).size;
  float maxSize = ( size.x > size.y ) ? size.x : size.y;
  Circle circleCollider = { {}, maxSize / 2.0f };
  addCircle( entity, circleCollider );
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

Sprite SpriteManager::get( EntityHandle entity ) {
  PROFILE;
  ComponentIndex componentInd = componentMap.lookup( entity );
  return static_cast< Sprite >( componentMap.components[ componentInd ] );
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
  updatedEntities = componentMap.have( updatedEntities );
  for ( u32 trInd = 0; trInd < updatedEntities.size(); ++trInd ) {
    // TODO get world transforms here
    Transform transform = TransformManager::get( updatedEntities[ trInd ] );
    ComponentIndex spriteCompInd = componentMap.lookup( updatedEntities[ trInd ] );
    componentMap.components[ spriteCompInd ].transform.position = transform.position;
    componentMap.components[ spriteCompInd ].transform.scale = transform.scale;
    componentMap.components[ spriteCompInd ].transform.orientation = transform.orientation;
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
