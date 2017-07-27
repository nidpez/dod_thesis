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
  std::vector< EntityHandle > result( componentMap.components.size() );
  for ( u32 entInd = 0; entInd < componentMap.components.size(); ++entInd ) {
    result[ entInd ] = componentMap.components[ entInd ].entity;
  }
  return result;
}

ComponentMap< CircleColliderManager::CircleColliderComp > CircleColliderManager::componentMap;

void CircleColliderManager::initialize() {
}

void CircleColliderManager::shutdown() {
}

void CircleColliderManager::add( EntityHandle entity, Circle circleCollider ) {
  float radius = circleCollider.radius;
  ASSERT( radius > 0.0f, "A collider of radius %f is useless", radius );
  Vec2 center = circleCollider.center;
  componentMap.set( entity, { entity, center, radius, { 0, 0 }, { 0, 0 } }, &CircleColliderManager::remove );
}

void CircleColliderManager::remove( EntityHandle entity ) {
  componentMap.remove( entity );
}

void CircleColliderManager::addAndFitToSpriteSize( EntityHandle entity ) {
  Circle circleCollider = { {}, 1.0f };
  add( entity, circleCollider );
  fitToSpriteSize( entity );
}
  
void CircleColliderManager::updateAndCollide() {
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
    ComponentIndex circleColliderCompInd = componentMap.lookup( updatedEntities[ trInd ] );
    componentMap.components[ circleColliderCompInd ].position = transform.position;
    componentMap.components[ circleColliderCompInd ].scale = transform.scale;
  }
  std::vector< Circle > transformedCircles;
  transformedCircles.reserve( componentMap.components.size() );
  for ( u32 colInd = 0; colInd < componentMap.components.size(); ++colInd ) {
    CircleColliderComp circleColliderComp = componentMap.components[ colInd ];
    float scaleX = circleColliderComp.scale.x, scaleY = circleColliderComp.scale.y;
    float maxScale = ( scaleX > scaleY ) ? scaleX : scaleY;
    Vec2 position = circleColliderComp.position + circleColliderComp.circle.center * maxScale;
    float radius = circleColliderComp.circle.radius * maxScale;
    transformedCircles.push_back( { position, radius } );
  }
  // n^2 collision detection
  // FIXME if we put the collision logic here or pass component instances
  // to the collide function we can save on a few array access calls
  Color color = { 0.0f, 1.0f, 0.0f, 1.0f };
  for ( u32 collI = 0; collI < componentMap.components.size() - 1; ++collI ) {
    Circle circleI = transformedCircles[ collI ];
    for ( u32 collJ = collI + 1; collJ < componentMap.components.size(); ++collJ ) {
      Circle circleJ = transformedCircles[ collJ ];
      if ( circleCircleCollide( circleI, circleJ ) ) {
        Debug::drawCircle( transformedCircles[ collI ], color );
        Debug::drawCircle( transformedCircles[ collJ ], color );
        // TODO create reactions for dynamic components
        // Debug::write( "Entities %d & %d collided\n", componentMap.components[ collI ].entity, componentMap.components[ collJ ].entity );
        // TODO fire callbacks for programatic components
      }
    }
  }
  // TODO do frustrum culling
  // for ( u32 colInd = 0; colInd < componentMap.components.size(); ++colInd ) {
  //   Debug::drawCircle( transformedCircles[ colInd ], color );
  // }
}

// FIXME take scale into account
bool CircleColliderManager::circleCircleCollide( Circle circleA, Circle circleB ) {
  PROFILE;
  float distance = magnitude( circleA.center - circleB.center );
  if ( distance <= circleA.radius + circleB.radius ) {
    return true;
  }
  return false;
}       

void CircleColliderManager::fitToSpriteSize( EntityHandle entity ) {
  ComponentIndex componentInd = componentMap.lookup( entity );
  Vec2 size = SpriteManager::get( entity ).size;
  float maxSize = ( size.x > size.y ) ? size.x : size.y;
  componentMap.components[ componentInd ].circle.radius = maxSize / 2.0f;
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
