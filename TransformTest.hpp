#pragma once

#include "EngineCommon.hpp"

class TransformTest {
  static double lastChangedParams;
  static double now;
  static u8 blockSize;
  static TextureHandle textureHandle;
  static std::vector< std::vector< EntityHandle > > entityHandles;
  static std::vector< std::vector< Transform > > transforms;
  static std::vector< float > rotationSpeeds;
  static std::vector< float > rotations;
  static std::vector< Vec2 > translationSpeeds;
  static std::vector< Vec2 > translations;
  static std::vector< Vec2 > rotationsAround;
  static std::vector< Vec2 > scaleSpeeds;
  static std::vector< Vec2 > scales;
  static std::vector< Vec2 > initialPositions1;
  static std::vector< Vec2 > initialPositions2;
  static std::vector< Vec2 > positions1;
  static std::vector< Vec2 > positions2;
  static std::vector< Vec2 > translationSpeeds2;
  static std::vector< float > rotationSpeeds2;
  static std::vector< Vec2 > scaleSpeeds2;
  static std::vector< Vec2 > scales2;
public:
  static void initialize( float aspect );
  static void update( double deltaT );
  static void shutdown();
};
  
double TransformTest::lastChangedParams;
double TransformTest::now;
u8 TransformTest::blockSize;
TextureHandle TransformTest::textureHandle;
std::vector< std::vector< EntityHandle > > TransformTest::entityHandles;
std::vector< std::vector< Transform > > TransformTest::transforms;
std::vector< float > TransformTest::rotationSpeeds;
std::vector< float > TransformTest::rotations;
std::vector< Vec2 > TransformTest::translationSpeeds;
std::vector< Vec2 > TransformTest::translations;
std::vector< Vec2 > TransformTest::rotationsAround;
std::vector< Vec2 > TransformTest::scaleSpeeds;
std::vector< Vec2 > TransformTest::scales;
std::vector< Vec2 > TransformTest::initialPositions1;
std::vector< Vec2 > TransformTest::initialPositions2;
std::vector< Vec2 > TransformTest::positions1;
std::vector< Vec2 > TransformTest::positions2;
std::vector< Vec2 > TransformTest::translationSpeeds2;
std::vector< float > TransformTest::rotationSpeeds2;
std::vector< Vec2 > TransformTest::scaleSpeeds2;
std::vector< Vec2 > TransformTest::scales2;

void TransformTest::initialize( float aspect ) {  
  // load textures
  textureHandle = AssetManager::loadTexture( "astronaut.png" );
  
  AnimationFrame runFrames[] = {
    { { 0.0f, 0.0f },		{ 1.0f / 5.0f, 1.0f } },
    { { 1.0f / 5.0f, 0.0f },	{ 2.0f / 5.0f, 1.0f } },
    { { 2.0f / 5.0f, 0.0f },	{ 3.0f / 5.0f, 1.0f } },
    { { 3.0f / 5.0f, 0.0f },	{ 4.0f / 5.0f, 1.0f } }
  };

  // create entities to test transform component
  u8 blockWidth = 7;
  u8 blockHeight = 10;
  blockSize = blockWidth * blockHeight;
  entityHandles = std::vector< std::vector< EntityHandle > >( 6 );  
  for ( u16 v = 0; v < 6; ++v ) {
    entityHandles[ v ] = std::vector< EntityHandle >();
    for ( u16 i = 0; i < blockSize; ++i ) {
      entityHandles[ v ].push_back( EntityManager::create() );
      // Logger::write( "ent %d\n", entityHandles[ v ][ entityHandles[ v ].size()-1 ] );
    }
  }

  // same sprite
  for ( u16 v = 0; v < 6; ++v ) {
    for ( u16 i = 0; i < blockSize; ++i ) {
      SpriteManager::set( entityHandles[ v ][ i ], textureHandle,
                          static_cast< Rect >( runFrames[ 0 ] ) );
    }
  }

  // all positions
  const u8 halfViewportHeight = 50; // FIXME viewport height = 100 as defined nowhere
  const u8 halfViewportWidth = halfViewportHeight * aspect;
  const float cellWidth = halfViewportWidth / 10.0f;
  const float cellHeight = halfViewportHeight / 10.0f;
  transforms = std::vector< std::vector< Transform > >( 6 );
  for ( u32 v = 0; v < 6; ++v ) {
    transforms[ v ] = std::vector< Transform >();
    transforms[ v ].reserve( blockSize );
    for ( u32 i = 0; i < blockWidth; ++i ) {
      for ( u32 j = 0; j < blockHeight; ++j ) {
        float x = static_cast< float >( ( v % 3 ) * blockWidth + i + 0.5f ) * cellWidth - halfViewportWidth;
        float y = static_cast< float >( ( ( v >= 3 ) ? blockHeight : 0 ) + j + 0.5f ) * cellHeight - halfViewportHeight;
        EntityHandle entity = entityHandles[ v ][ blockHeight * i + j ];
        Transform transform = { { x, y }, { 1.0f, 1.0f }, 0.0f };
        TransformManager::set( entity, transform );
        transforms[ v ].push_back( transform );
      }
    }
  }

  std::default_random_engine generator;
  auto frand = std::uniform_real_distribution< float >( -1.0f, 1.0f );
  
  rotationSpeeds = std::vector< float >( blockSize );
  rotations = std::vector< float >( blockSize );
  for ( u16 i = 0; i < blockSize; ++i ) {
    rotationSpeeds[ i ] = frand( generator );
  }
  
  translationSpeeds = std::vector< Vec2 >( blockSize );
  translations = std::vector< Vec2 >( blockSize );
  for ( u16 i = 0; i < blockSize; ++i ) {
    translationSpeeds[ i ] = { frand( generator ), frand( generator ) };
  }

  rotationsAround = std::vector< Vec2 >( blockSize );
  for ( u16 i = 0; i < blockSize; ++i ) {
    Vec2 offset = { frand( generator ), frand( generator ) };
    rotationsAround[ i ] = { transforms[ 2 ][ i ].position + offset * 5.0f };
  }

  scaleSpeeds = std::vector< Vec2 >( blockSize );
  scales = std::vector< Vec2 >( blockSize, { 1.0f, 1.0f } );
  for ( u16 i = 0; i < blockSize; ++i ) {
    scaleSpeeds[ i ] = { frand( generator ), frand( generator ) };
  }

  initialPositions1 = std::vector< Vec2 >( blockSize );
  initialPositions2 = std::vector< Vec2 >( blockSize );
  positions1 = std::vector< Vec2 >( blockSize );
  positions2 = std::vector< Vec2 >( blockSize );
  translationSpeeds2 = std::vector< Vec2 >( blockSize );
  rotationSpeeds2 = std::vector< float >( blockSize );
  scaleSpeeds2 = std::vector< Vec2 >( blockSize );
  scales2 = std::vector< Vec2 >( blockSize, { 1.0f, 1.0f } );
  for ( u16 i = 0; i < blockSize; ++i ) {
    initialPositions1[ i ] = transforms[ 4 ][ i ].position;
    initialPositions2[ i ] = transforms[ 5 ][ i ].position;
    positions1[ i ] = initialPositions1[ i ];
    positions2[ i ] = initialPositions2[ i ];
    translationSpeeds2[ i ] = { frand( generator ), frand( generator ) };
    scaleSpeeds2[ i ] = { frand( generator ), frand( generator ) };
    rotationSpeeds2[ i ] = frand( generator );
  }
  
  lastChangedParams = glfwGetTime();
}

void TransformTest::update( double deltaT ) {
  for ( u16 i = 0; i < blockSize; ++i ) {
    float rotation = rotationSpeeds[ i ] * deltaT * 10.0f;
    TransformManager::rotate( entityHandles[ 0 ][ i ], rotation );
  }
  
  now = glfwGetTime();
  bool timeToChangeParams = false;
  if ( now - lastChangedParams >= 1.0f ) {
    timeToChangeParams = true;
    lastChangedParams = now;
  }
  
  for ( u16 i = 0; i < blockSize; ++i ) {
    if ( timeToChangeParams ) {
      translationSpeeds[ i ] = -translationSpeeds[ i ];
    }
    Vec2 translation = translationSpeeds[ i ] * deltaT * 10.0f;
    TransformManager::translate( entityHandles[ 1 ][ i ], translation );
  }
  
  for ( u16 i = 0; i < blockSize; ++i ) {
    float rotation = rotationSpeeds[ i ] * deltaT * 10.0f;
    TransformManager::rotateAround( entityHandles[ 2 ][ i ], rotationsAround[ i ], rotation );
  }
  
  for ( u16 i = 0; i < blockSize; ++i ) {
    if ( timeToChangeParams ) {
      scaleSpeeds[ i ] = -scaleSpeeds[ i ];
    }
    scales[ i ] += scaleSpeeds[ i ] * deltaT;
    TransformManager::scale( entityHandles[ 3 ][ i ], scales[ i ] );
  }
  
  for ( u16 i = 0; i < blockSize; ++i ) {
    positions1[ i ] += translationSpeeds2[ i ] * deltaT * 5.0f;
    Vec2 pos = positions1[ i ];
    pos.x = std::sin( pos.x );
    pos.y = std::sin( pos.y );
    transforms[ 4 ][ i ].position = initialPositions1[ i ] + pos * 5.0f;
    transforms[ 4 ][ i ].orientation += rotationSpeeds[ i ] * deltaT * 10.0f;
    TransformManager::update( entityHandles[ 4 ][ i ], transforms[ 4 ][ i ] );
  }
  
  for ( u16 i = 0; i < blockSize; ++i ) {
    positions2[ i ] += translationSpeeds2[ i ] * deltaT * 5.0f;
    Vec2 pos = positions1[ i ];
    pos.x = std::sin( pos.x );
    pos.y = std::sin( pos.y );
    transforms[ 5 ][ i ].position = initialPositions2[ i ] + pos * 5.0f;
    scales2[ i ] += scaleSpeeds2[ i ] * deltaT * 3.0f;
    Vec2 scale = scales2[ i ];
    scale.x = std::sin( scales2[ i ].x );
    scale.y = std::sin( scales2[ i ].y );
    transforms[ 5 ][ i ].scale = scale * 1.5f;
    TransformManager::update( entityHandles[ 5 ][ i ], transforms[ 5 ][ i ] );
  }
}

void TransformTest::shutdown() {
  AssetManager::destroyTexture( textureHandle );
}
