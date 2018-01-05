#pragma once

class TestScene {
  static constexpr const u32 NUM_ENTITIES = 100;
  static constexpr const Rect TEST_AREA = { { -70, -40 }, { 70, 40 } };
  static std::vector< EntityHandle > entities;
  static std::vector< Vec2 > directions;
public:
  static void initialize();
  static void update( double deltaT );
  static void shutdown();
};

std::vector< EntityHandle > TestScene::entities;
std::vector< Vec2 > TestScene::directions;

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
  Transform transform = { { WALL_THICKNESS, TEST_AREA.max.y }, VEC2_ONE, {} };
  TransformManager::set( enclosure[ 0 ], transform );
  Rect bounds = { { TEST_AREA.min.x - WALL_THICKNESS * 2, 0.0f },
                  { TEST_AREA.max.x, WALL_THICKNESS } };
  ColliderManager::addAxisAlignedRect( enclosure[ 0 ], bounds );
  // bottom wall
  transform.position.y = TEST_AREA.min.y;
  TransformManager::set( enclosure[ 2 ], transform );
  bounds.max.y = -bounds.max.y;
  ColliderManager::addAxisAlignedRect( enclosure[ 2 ], bounds );
  // right wall
  transform = { { TEST_AREA.max.x, WALL_THICKNESS }, VEC2_ONE, {} };
  TransformManager::set( enclosure[ 1 ], transform );
  bounds = { { 0.0f, TEST_AREA.min.y - WALL_THICKNESS * 2 },
             { WALL_THICKNESS, TEST_AREA.max.y } };
  ColliderManager::addAxisAlignedRect( enclosure[ 1 ], bounds );
  // left wall
  transform.position.x = TEST_AREA.min.x;
  TransformManager::set( enclosure[ 3 ], transform );
  bounds.max.x = -bounds.max.x;
  ColliderManager::addAxisAlignedRect( enclosure[ 3 ], bounds );

  // create actors
  entities.reserve( NUM_ENTITIES );
  for ( u32 ent = 0; ent < NUM_ENTITIES; ++ent ) {
    entities.push_back( EntityManager::create() );
  }
  for ( u32 ent = 0; ent < NUM_ENTITIES; ++ent ) {
    Transform transform;
    transform.position = { randf( TEST_AREA.min.x, TEST_AREA.max.x ),
                           randf( TEST_AREA.min.y, TEST_AREA.max.y ) };
    transform.scale = VEC2_ONE;
    transform.orientation = randf( 0.0f, 2 * PI );
    TransformManager::set( entities[ ent ], transform );
  }
  AssetIndex textureHandle = AssetManager::loadTexture( "astronaut.png" );
  for ( u32 ent = 0; ent < NUM_ENTITIES; ++ent ) {
    SpriteManager::set( entities[ ent ], textureHandle,
                        { { 0.0f, 0.0f }, { 1.0f / 5.0f, 1.0f } } );
  }
  for ( u32 ent = 0; ent < NUM_ENTITIES; ++ent ) {
    ColliderManager::fitCircleToSprite( entities[ ent ] );
  }
  directions.reserve( NUM_ENTITIES );
  for ( u32 ent = 0; ent < NUM_ENTITIES; ++ent ) {
    Vec2 direction = { randf( -1.0f, 1.0f ), randf( -1.0f, 1.0f ) };
    directions.push_back( normalized( direction ) );
  }
}

void TestScene::update( double deltaT ) {
  for ( u32 ent = 0; ent < NUM_ENTITIES; ++ent ) {
    Vec2 translation = directions[ ent ] * deltaT * 10.0f;
    TransformManager::translate( entities[ ent ], translation );
  }
}

void TestScene::shutdown() {
  
}
