#pragma once

class TestScene {
  static constexpr const u32 NUM_ENTITIES = 1500;
  static constexpr const Rect TEST_AREA = { { -420, -240 }, { 420, 240 } };
  static std::vector< EntityHandle > entities;
public:
  static void initialize();
  static void update( double deltaT );
  static void shutdown();
};

std::vector< EntityHandle > TestScene::entities;

// TODO put randf somewhere appropriate
float randf( float min, float max ) {
  static const float F_RAND_MAX = static_cast< float >( RAND_MAX );
  return min + ( std::rand() / F_RAND_MAX ) * ( max - min );
}

void TestScene::initialize() {
  // create enclosure
  EntityHandle enclosure[ 4 ];
  for ( u32 wall = 0; wall < 4; ++wall ) {
    enclosure[ wall ] = EntityManager::create();
  }
  const float WALL_THICKNESS = 5.0f;
  // top wall
  Transform transform = { { 0.0f, TEST_AREA.max.y }, VEC2_ONE, {} };
  TransformManager::set( enclosure[ 0 ], transform );
  Rect bounds = { { TEST_AREA.min.x, 0.0f },
                  { TEST_AREA.max.x, WALL_THICKNESS } };
  ColliderManager::addAxisAlignedRect( enclosure[ 0 ], bounds );
  // bottom wall
  transform.position.y = TEST_AREA.min.y - WALL_THICKNESS;
  TransformManager::set( enclosure[ 1 ], transform );
  ColliderManager::addAxisAlignedRect( enclosure[ 1 ], bounds );
  // right wall
  transform = { { TEST_AREA.max.x, 0.0f }, VEC2_ONE, {} };
  TransformManager::set( enclosure[ 2 ], transform );
  bounds = { { 0.0f, TEST_AREA.min.y },
             { WALL_THICKNESS, TEST_AREA.max.y } };
  ColliderManager::addAxisAlignedRect( enclosure[ 2 ], bounds );
  // left wall
  transform.position.x = TEST_AREA.min.x - WALL_THICKNESS;
  TransformManager::set( enclosure[ 3 ], transform );
  ColliderManager::addAxisAlignedRect( enclosure[ 3 ], bounds );

  // create actors
  {
    PROFILE_BLOCK( "Create Actors" );
    AssetIndex textureHandle = AssetManager::loadTexture( "astronaut.png" );
    entities.reserve( NUM_ENTITIES );
    for ( u32 ent = 0; ent < NUM_ENTITIES; ++ent ) {
      EntityHandle entity = EntityManager::create();
      entities.push_back( entity );
      
      Transform transform;
      float r1 = randf( TEST_AREA.min.x, TEST_AREA.max.x );
      float r2 = randf( TEST_AREA.min.y, TEST_AREA.max.y );
      transform.position = { r1, r2 };
      transform.scale = VEC2_ONE;
      float r3 = randf( 0.0f, 2 * PI );
      transform.orientation = r3;
      TransformManager::set( entity, transform );
      
      SpriteManager::set( entity, textureHandle,
                          { { 0.0f, 0.0f }, { 1.0f / 5.0f, 1.0f } } );
      
      ColliderManager::fitCircleToSprite( entity );

      float r4 = randf( -1.0f, 1.0f );
      float r5 = randf( -1.0f, 1.0f );
      Vec2 direction = { r4, r5 };
      direction = normalized( direction );
      SolidBodyManager::set( entity, { direction * 5 } );
    }
  }
}

void TestScene::shutdown() {
  
}
