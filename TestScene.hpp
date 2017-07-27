#pragma once

class TestScene {
  static constexpr const u32 NUM_ENTITIES = 100;
  static constexpr const Rect TEST_AREA = { { -70, -40 }, { 70, 40 } };
  static std::vector< EntityHandle > entities;
public:
  static void initialize();
  static void update();
  static void shutdown();
};

std::vector< EntityHandle > TestScene::entities;

// TODO put randf somewhere appropriate
float randf( float min, float max ) {
  static const float F_RAND_MAX = static_cast< float >( RAND_MAX );
  return min + ( std::rand() / F_RAND_MAX ) * ( max - min );
}

void TestScene::initialize() {
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
    CircleColliderManager::addAndFitToSpriteSize( entities[ ent ] );
  }
}

void TestScene::update() {
  
}

void TestScene::shutdown() {
  
}
