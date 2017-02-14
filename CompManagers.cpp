#include "CompManagers.hpp"

std::vector< TransformManager::TransformComp > TransformManager::transformComps;
ComponentMap TransformManager::componentMap;

void TransformManager::initialize() {
}

void TransformManager::shutdown() {
}

void TransformManager::set( const std::vector< EntityHandle >& entities, const std::vector< Transform >& transforms ) {
  VALIDATE_ENTITIES( entities );
  std::vector< SetComponentMapArg > mappedPairs( transforms.size() );
  for ( u32 trInd = 0; trInd < transforms.size(); ++trInd ) {
    Transform transform = transforms[ trInd ];
    EntityHandle entity = entities[ trInd ];
    transformComps.push_back( { entity, transform, transform, 0, 0, 0, 0 } );
    u32 compInd = transformComps.size() - 1;
    mappedPairs[ trInd ] = { entity, compInd };
  }
  componentMap.set( mappedPairs );
}

void TransformManager::rotate( const std::vector< EntityHandle >& entities, const std::vector< float >& rotations ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    transformComps[ componentInds[ entInd ] ].local.orientation += rotations[ entInd ];
  }
  // TODO mark transform components as updated
}

void TransformManager::rotateAround( const std::vector< EntityHandle >& entities, const std::vector< RotateAroundArg >& rotations ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    ComponentIndex componentInd = componentInds[ entInd ];
    RotateAroundArg rotateArg = rotations[ entInd ];
    Transform transform = transformComps[ componentInd ].local;
    transform.orientation += rotateArg.rotation;
    transform.position = rotateVec2( transform.position - rotateArg.point, rotateArg.rotation ) + rotateArg.point;
    transformComps[ componentInd ].local = transform;
  }
  // TODO mark transform components as updated
}

void TransformManager::translate( const std::vector< EntityHandle >& entities, const std::vector< Vec2 >& translations ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    transformComps[ componentInds[ entInd ] ].local.position += translations[ entInd ];
  }
  // TODO mark transform components as updated
}

void TransformManager::scale( const std::vector< EntityHandle >& entities, const std::vector< Vec2 >& scales ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    transformComps[ componentInds[ entInd ] ].local.scale = scales[ entInd ];
  }
  // TODO mark transform components as updated
}

void TransformManager::update( const std::vector< EntityHandle >& entities, const std::vector< Transform >& transforms ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    ComponentIndex componentInd = componentInds[ entInd ];
    Transform transformArg = transforms[ entInd ];
    Transform transform = transformComps[ componentInd ].local;
    transform.position = transformArg.position;
    transform.scale = transformArg.scale;
    transform.orientation = transformArg.orientation;
    transformComps[ componentInd ].local = transform;
  }
  // TODO mark transform components as updated
}

std::vector< Transform > TransformManager::get( const std::vector< EntityHandle >& entities ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  std::vector< Transform > result( entities.size() );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    result[ entInd ] = transformComps[ componentInds[ entInd ] ].local;
  }
  return result;
}

std::vector< EntityHandle > TransformManager::getLastUpdated() {
  // TODO actually compute which transforms have been updated since last frame
  std::vector< EntityHandle > result( transformComps.size() );
  for ( u32 entInd = 0; entInd < transformComps.size(); ++entInd ) {
    result[ entInd ] = transformComps[ entInd ].entity;
  }
  return result;
}

std::vector< CircleColliderManager::__CircleColliderComp > CircleColliderManager::circleColliderComps;
ComponentMap CircleColliderManager::componentMap;

void CircleColliderManager::initialize() {
}

void CircleColliderManager::shutdown() {
}

void CircleColliderManager::add( const std::vector< CircleColliderComp >& circleColliders ) {
  std::vector< SetComponentMapArg > mappedPairs( circleColliders.size() );
  for ( u32 collInd = 0; collInd < circleColliders.size(); ++collInd ) {
    CircleColliderComp circleColliderComp = circleColliders[ collInd ];
    EntityHandle entity = circleColliderComp.entity;
    VALIDATE_ENTITY( entity );
    float radius = circleColliderComp.radius;
    ASSERT( radius > 0.0f, "A collider of radius %f is useless", radius );
    Vec2 center = circleColliderComp.center;
    circleColliderComps.push_back( { entity, center, radius, { 0, 0 }, { 0, 0 } } );
    u32 compInd = circleColliderComps.size() - 1;
    mappedPairs[ collInd ] = { entity, compInd };
  }
  componentMap.set( mappedPairs );
}

void CircleColliderManager::addAndFitToSpriteSize( const std::vector< EntityHandle >& entities ) {
  std::vector< CircleColliderComp > circleColliders( entities.size() );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    circleColliders[ entInd ] = { entities[ entInd ], {}, 1.0f };
  }
  add( circleColliders );
  fitToSpriteSize( entities );
}
  
void CircleColliderManager::updateAndCollide() {
  if ( circleColliderComps.size() == 0 ) {
    return;
  }
  // update local transform cache
  std::vector< EntityHandle > updatedEntities = TransformManager::getLastUpdated();
  // TODO get world transforms here
  std::vector< Transform > updatedTransforms = TransformManager::get( updatedEntities );
  updatedEntities = componentMap.have( updatedEntities );
  std::vector< ComponentIndex > updatedCircleColliders = componentMap.lookup( updatedEntities );
  for ( u32 trInd = 0; trInd < updatedTransforms.size(); ++trInd ) {
    Transform transform = updatedTransforms[ trInd ];
    ComponentIndex circleColliderCompInd = updatedCircleColliders[ trInd ];
    circleColliderComps[ circleColliderCompInd ].position = transform.position;
    circleColliderComps[ circleColliderCompInd ].scale = transform.scale;
  }
  // TODO do frustrum culling
  std::vector< DebugCircle > circles;
  circles.reserve( circleColliderComps.size() );
  for ( u32 colInd = 0; colInd < circleColliderComps.size(); ++colInd ) {
    __CircleColliderComp circleColliderComp = circleColliderComps[ colInd ];
    DebugCircle circle;
    float scaleX = circleColliderComp.scale.x, scaleY = circleColliderComp.scale.y;
    float maxScale = ( scaleX > scaleY ) ? scaleX : scaleY;
    circle.position = circleColliderComp.position + circleColliderComp.center * maxScale;
    circle.radius = circleColliderComp.radius * maxScale;
    circle.color[ 0 ] =  0.0f;
    circle.color[ 1 ] =  1.0f;
    circle.color[ 2 ] =  0.0f;
    circle.color[ 3 ] =  1.0f;
    circles.push_back( circle );
  }
  DebugRenderer::addCircles( circles );
}

void CircleColliderManager::fitToSpriteSize( const std::vector< EntityHandle >& entities ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  std::vector< SpriteComp > spriteComps = SpriteManager::get( entities );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    SpriteComp spriteComp = spriteComps[ entInd ];
    ComponentIndex componentInd = componentInds[ entInd ];
    Vec2 size = spriteComp.size;
    float maxSize = ( size.x > size.y ) ? size.x : size.y;
    circleColliderComps[ componentInd ].radius = maxSize / 2.0f;
  }
}

std::vector< SpriteManager::__SpriteComp > SpriteManager::spriteComps;
ComponentMap SpriteManager::componentMap;
RenderInfo SpriteManager::renderInfo;
SpriteManager::Pos* SpriteManager::posBufferData;
SpriteManager::UV* SpriteManager::texCoordsBufferData;

SpriteManager::__SpriteComp::operator SpriteComp() const {
  return { this->entity, this->textureId, this->texCoords, this->size };
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

void SpriteManager::set( const std::vector< SetSpriteArg >& sprites ) {
  std::vector< SetComponentMapArg > mappedPairs( sprites.size() );
  for ( u32 sprInd = 0; sprInd < sprites.size(); ++sprInd ) {
    SetSpriteArg setSpriteArg = sprites[ sprInd ];
    EntityHandle entity = setSpriteArg.entity;
    VALIDATE_ENTITY( entity );
    TextureHandle textureId = setSpriteArg.textureId;
    Rect texCoords = setSpriteArg.texCoords; 
    ASSERT( AssetManager::isTextureAlive( textureId ), "Invalid texture id %d", textureId ); 
    __SpriteComp spriteComp = {};
    spriteComp.entity = entity;
    spriteComp.textureId = textureId;
    spriteComp.texCoords = texCoords;
    TextureAsset texture = AssetManager::getTexture( textureId );
    float width = texture.width * ( texCoords.max.u - texCoords.min.u ) / PIXELS_PER_UNIT;
    float height = texture.height * ( texCoords.max.v - texCoords.min.v ) / PIXELS_PER_UNIT;
    spriteComp.size = { width, height };
    spriteComps.push_back( spriteComp );
    u32 compInd = spriteComps.size() - 1;
    mappedPairs[ sprInd ] = { entity, compInd };
  }
  componentMap.set( mappedPairs );
}

std::vector< SpriteComp > SpriteManager::get( const std::vector< EntityHandle >& entities ) {
  std::vector< ComponentIndex > componentInds = componentMap.lookup( entities );
  std::vector< SpriteComp > resultSpriteComps;
  resultSpriteComps.reserve( entities.size() );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    SpriteComp spriteComp = static_cast< SpriteComp >( spriteComps[ componentInds[ entInd ] ] );
    resultSpriteComps.push_back( spriteComp );
  }
  return resultSpriteComps;
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
  // TODO get world transforms here
  std::vector< Transform > updatedTransforms = TransformManager::get( updatedEntities );
  updatedEntities = componentMap.have( updatedEntities );
  std::vector< ComponentIndex > updatedSprites = componentMap.lookup( updatedEntities );
  for ( u32 trInd = 0; trInd < updatedTransforms.size(); ++trInd ) {
    Transform transform = updatedTransforms[ trInd ];
    ComponentIndex spriteCompInd = updatedSprites[ trInd ];
    spriteComps[ spriteCompInd ].position = transform.position;
    spriteComps[ spriteCompInd ].scale = transform.scale;
    spriteComps[ spriteCompInd ].orientation = transform.orientation;
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
    __SpriteComp spriteComp = spriteComps[ spriteInd ];
    for ( u32 vertInd = 0; vertInd < vertsPerSprite; ++vertInd ) {
      Vec2 vert = baseGeometry[ vertInd ] * spriteComp.size * spriteComp.scale;
      vert = rotateVec2( vert, spriteComp.orientation );
      vert += spriteComp.position;
      posBufferData[ spriteInd * vertsPerSprite + vertInd ].pos = vert;
    }
  } 
  glBindBuffer( GL_ARRAY_BUFFER, renderInfo.vboIds[ 0 ] );
  glBufferData( GL_ARRAY_BUFFER, spritesToRenderCount * vertsPerSprite * sizeof( Pos ), posBufferData, GL_STATIC_DRAW );
  // TODO measure how expensive these allocations and deallocations are!
  delete[] posBufferData;
  // build the texture coordinates buffer
  for ( u32 spriteInd = 0; spriteInd < spritesToRenderCount; ++spriteInd ) {
    __SpriteComp spriteComp = spriteComps[ spriteInd ];
    Vec2 texCoords[] = {
      spriteComp.texCoords.min,
      spriteComp.texCoords.max,
      { spriteComp.texCoords.min.u, spriteComp.texCoords.max.v },
      spriteComp.texCoords.min,
      { spriteComp.texCoords.max.u, spriteComp.texCoords.min.v },
      spriteComp.texCoords.max
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
  u32 currentTexId = spriteComps[ 0 ].textureId;  
  ASSERT( AssetManager::isTextureAlive( currentTexId ), "Invalid texture id %d", currentTexId );  
  u32 currentTexGlId = AssetManager::getTexture( currentTexId ).glId;
  glBindTexture( GL_TEXTURE_2D, currentTexGlId );
  // mark where a sub-buffer with sprites sharing a texture ends and a new one begins
  u32 currentSubBufferStart = 0;
  for ( u32 spriteInd = 1; spriteInd < spritesToRenderCount; ++spriteInd ) {
    if ( spriteComps[ spriteInd ].textureId != currentTexId ) {
      // send current vertex sub-buffer and render it
      u32 spriteCountInSubBuffer = spriteInd - currentSubBufferStart;
      glDrawArrays( GL_TRIANGLES, vertsPerSprite * currentSubBufferStart, vertsPerSprite * spriteCountInSubBuffer );
      // and start a new one
      currentTexId = spriteComps[ spriteInd ].textureId;      
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
