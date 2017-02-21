#include "CompManagers.hpp"

std::vector< TransformManager::TransformComp > TransformManager::transformComps;
ComponentMap TransformManager::componentMap;

void TransformManager::initialize() {
}

void TransformManager::shutdown() {
}

void TransformManager::set( EntityHandle entity, Transform transform ) {
  VALIDATE_ENTITY( entity );
  transformComps.push_back( { entity, transform, transform, 0, 0, 0, 0 } );
  u32 compInd = transformComps.size() - 1;
  componentMap.set( entity, compInd );
}

void TransformManager::rotate( EntityHandle entity, float rotation ) {
  ComponentIndex componentInd = componentMap.lookup( entity );
  transformComps[ componentInd ].local.orientation += rotation;
  // TODO mark transform component as updated
}

void TransformManager::rotateAround( EntityHandle entity, Vec2 point, float rotation ) {
  ComponentIndex componentInd = componentMap.lookup( entity );
  Transform transform = transformComps[ componentInd ].local;
  transform.orientation += rotation;
  transform.position = rotateVec2( transform.position - point, rotation ) + point;
  transformComps[ componentInd ].local = transform;
  // TODO mark transform component as updated
}

void TransformManager::translate( EntityHandle entity, Vec2 translation ) {
  ComponentIndex componentInd = componentMap.lookup( entity );
  transformComps[ componentInd ].local.position += translation;
  // TODO mark transform component as updated
}

void TransformManager::scale( EntityHandle entity, Vec2 scale ) {
  ComponentIndex componentInd = componentMap.lookup( entity );
  transformComps[ componentInd ].local.scale = scale;
  // TODO mark transform component as updated
}

void TransformManager::update( EntityHandle entity, Transform transform ) {
  ComponentIndex componentInd = componentMap.lookup( entity );
  Transform local = transformComps[ componentInd ].local;
  local.position = transform.position;
  local.scale = transform.scale;
  local.orientation = transform.orientation;
  transformComps[ componentInd ].local = local;
  // TODO mark transform component as updated
}

Transform TransformManager::get( EntityHandle entity ) {
  ComponentIndex componentInd = componentMap.lookup( entity );
  return transformComps[ componentInd ].local;
}

std::vector< EntityHandle > TransformManager::getLastUpdated() {
  // TODO actually compute which transforms have been updated since last frame
  std::vector< EntityHandle > result( transformComps.size() );
  for ( u32 entInd = 0; entInd < transformComps.size(); ++entInd ) {
    result[ entInd ] = transformComps[ entInd ].entity;
  }
  return result;
}

std::vector< CircleColliderManager::CircleColliderComp > CircleColliderManager::circleColliderComps;
ComponentMap CircleColliderManager::componentMap;

void CircleColliderManager::initialize() {
}

void CircleColliderManager::shutdown() {
}

void CircleColliderManager::add( EntityHandle entity, Circle circleCollider ) {
  VALIDATE_ENTITY( entity );
  float radius = circleCollider.radius;
  ASSERT( radius > 0.0f, "A collider of radius %f is useless", radius );
  Vec2 center = circleCollider.center;
  circleColliderComps.push_back( { entity, center, radius, { 0, 0 }, { 0, 0 } } );
  u32 compInd = circleColliderComps.size() - 1;
  componentMap.set( entity, compInd );
}

void CircleColliderManager::addAndFitToSpriteSize( EntityHandle entity ) {
  Circle circleCollider = { {}, 1.0f };
  add( entity, circleCollider );
  fitToSpriteSize( entity );
}
  
void CircleColliderManager::updateAndCollide() {
  if ( circleColliderComps.size() == 0 ) {
    return;
  }
  // update local transform cache
  std::vector< EntityHandle > updatedEntities = TransformManager::getLastUpdated();
  updatedEntities = componentMap.have( updatedEntities );
  for ( u32 trInd = 0; trInd < updatedEntities.size(); ++trInd ) {
    // FIXME get world transforms here
    Transform transform = TransformManager::get( updatedEntities[ trInd ] );
    ComponentIndex circleColliderCompInd = componentMap.lookup( updatedEntities[ trInd ] );
    circleColliderComps[ circleColliderCompInd ].position = transform.position;
    circleColliderComps[ circleColliderCompInd ].scale = transform.scale;
  }
  // TODO do frustrum culling
  for ( u32 colInd = 0; colInd < circleColliderComps.size(); ++colInd ) {
    CircleColliderComp circleColliderComp = circleColliderComps[ colInd ];
    float scaleX = circleColliderComp.scale.x, scaleY = circleColliderComp.scale.y;
    float maxScale = ( scaleX > scaleY ) ? scaleX : scaleY;
    Vec2 position = circleColliderComp.position + circleColliderComp.circle.center * maxScale;
    float radius = circleColliderComp.circle.radius * maxScale;
    Circle circle = { position, radius };
    Color color = { 0.0f, 1.0f, 0.0f, 1.0f };
    DebugRenderer::addCircle( circle, color );
  }
}

void CircleColliderManager::fitToSpriteSize( EntityHandle entity ) {
  ComponentIndex componentInd = componentMap.lookup( entity );
  Vec2 size = SpriteManager::get( entity ).size;
  float maxSize = ( size.x > size.y ) ? size.x : size.y;
  circleColliderComps[ componentInd ].circle.radius = maxSize / 2.0f;
}

std::vector< SpriteManager::SpriteComp > SpriteManager::spriteComps;
ComponentMap SpriteManager::componentMap;
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
  s32 error = createShaderProgram( &renderInfo.shaderProgramId,
                                   "shaders/SpriteUnlit.vert", "shaders/SpriteUnlit.frag" );
  if ( error ) {
    // TODO maybe hardcode a default shader here
  }  
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

void SpriteManager::set( EntityHandle entity, TextureHandle textureId, Rect texCoords ) {
  VALIDATE_ENTITY( entity );
  ASSERT( AssetManager::isTextureAlive( textureId ), "Invalid texture id %d", textureId ); 
  SpriteComp spriteComp = {};
  spriteComp.entity = entity;
  spriteComp.sprite.textureId = textureId;
  spriteComp.sprite.texCoords = texCoords;
  TextureAsset texture = AssetManager::getTexture( textureId );
  float width = texture.width * ( texCoords.max.u - texCoords.min.u ) / PIXELS_PER_UNIT;
  float height = texture.height * ( texCoords.max.v - texCoords.min.v ) / PIXELS_PER_UNIT;
  spriteComp.sprite.size = { width, height };
  spriteComps.push_back( spriteComp );
  u32 compInd = spriteComps.size() - 1;
  componentMap.set( entity, compInd );
}

Sprite SpriteManager::get( EntityHandle entity ) {
  ComponentIndex componentInd = componentMap.lookup( entity );
  return static_cast< Sprite >( spriteComps[ componentInd ] );
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
  if ( spriteComps.size() == 0 ) {
    return;
  }
  // update local transform cache
  std::vector< EntityHandle > updatedEntities = TransformManager::getLastUpdated();
  updatedEntities = componentMap.have( updatedEntities );
  for ( u32 trInd = 0; trInd < updatedEntities.size(); ++trInd ) {
    // TODO get world transforms here
    Transform transform = TransformManager::get( updatedEntities[ trInd ] );
    ComponentIndex spriteCompInd = componentMap.lookup( updatedEntities[ trInd ] );
    spriteComps[ spriteCompInd ].transform.position = transform.position;
    spriteComps[ spriteCompInd ].transform.scale = transform.scale;
    spriteComps[ spriteCompInd ].transform.orientation = transform.orientation;
  }
  // build vertex buffer and render for sprites with same texture
  glUseProgram( renderInfo.shaderProgramId );
  glBindVertexArray( renderInfo.vaoId );
  // TODO don't render every sprite every time
  u32 spritesToRenderCount = spriteComps.size();
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
    SpriteComp spriteComp = spriteComps[ spriteInd ];
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
    SpriteComp spriteComp = spriteComps[ spriteInd ];
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
  u32 currentTexId = spriteComps[ 0 ].sprite.textureId;  
  ASSERT( AssetManager::isTextureAlive( currentTexId ), "Invalid texture id %d", currentTexId );  
  u32 currentTexGlId = AssetManager::getTexture( currentTexId ).glId;
  glBindTexture( GL_TEXTURE_2D, currentTexGlId );
  // mark where a sub-buffer with sprites sharing a texture ends and a new one begins
  u32 currentSubBufferStart = 0;
  for ( u32 spriteInd = 1; spriteInd < spritesToRenderCount; ++spriteInd ) {
    if ( spriteComps[ spriteInd ].sprite.textureId != currentTexId ) {
      // send current vertex sub-buffer and render it
      u32 spriteCountInSubBuffer = spriteInd - currentSubBufferStart;
      glDrawArrays( GL_TRIANGLES, vertsPerSprite * currentSubBufferStart, vertsPerSprite * spriteCountInSubBuffer );
      // and start a new one
      currentTexId = spriteComps[ spriteInd ].sprite.textureId;      
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
