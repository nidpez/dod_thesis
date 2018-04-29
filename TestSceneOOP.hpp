#pragma once

class TestScene {
  static constexpr const u32 NUM_ENTITIES = 1500;
  static const Rect TEST_AREA;
  static Entity enclosure[ 4 ];
  static Entity entities[ NUM_ENTITIES ];
public:
  static void initialize();
  static void update( double deltaT );
  static void shutdown();
};

const Rect TestScene::TEST_AREA( Vec2( -420, -240 ), Vec2( 420, 240 ) );
Entity TestScene::enclosure[ 4 ];
Entity TestScene::entities[ NUM_ENTITIES ];

// TODO put randf somewhere appropriate
float randf( float min, float max ) {
  static const float F_RAND_MAX = static_cast< float >( RAND_MAX );
  return min + ( std::rand() / F_RAND_MAX ) * ( max - min );
}

void TestScene::initialize() {
  // create enclosure
  const float WALL_THICKNESS = 5.0f;
  // top wall
  Vec2 position( 0.0f, TEST_AREA.getMax().getY() );
  enclosure[ 0 ].setTransform( Transform( position, Vec2::ONE, {} ) );
  Rect bounds( Vec2( TEST_AREA.getMin().getX(), 0.0f ),
               Vec2( TEST_AREA.getMax().getX(), WALL_THICKNESS ) );
  enclosure[ 0 ].setCollider( Collider( bounds ) );
  // bottom wall
  enclosure[ 1 ].setTransform( Transform( Vec2( position.getX(), TEST_AREA.getMin().getY() - WALL_THICKNESS ), Vec2::ONE, {} ) );
  enclosure[ 1 ].setCollider( Collider( bounds ) );
  // right wall
  position = Vec2( TEST_AREA.getMax().getX(), 0.0f );
  enclosure[ 2 ].setTransform( Transform( position, Vec2::ONE, {} ) );
  bounds = Rect( Vec2( 0.0f, TEST_AREA.getMin().getY() ),
                 Vec2( WALL_THICKNESS, TEST_AREA.getMax().getY() ) );
  enclosure[ 2 ].setCollider( Collider( bounds ) );
  // left wall
  enclosure[ 3 ].setTransform( Transform( Vec2( TEST_AREA.getMin().getX() - WALL_THICKNESS, position.getY() ), Vec2::ONE, {} ) );
  enclosure[ 3 ].setCollider( Collider( bounds ) );

  // create actors
  {
    PROFILE_BLOCK( "Create Actors" );
    AssetIndex textureHandle = AssetManager::loadTexture( "astronaut.png" );
    for ( u32 ent = 0; ent < NUM_ENTITIES; ++ent ) {
      float r1 = randf( TEST_AREA.getMin().getX(), TEST_AREA.getMax().getX() );
      float r2 = randf( TEST_AREA.getMin().getY(), TEST_AREA.getMax().getY() );
      Vec2 position( r1, r2 );
      float r3 = randf( 0.0f, 2 * PI );
      float orientation = r3;
      entities[ ent ].setTransform( Transform( position, Vec2::ONE, orientation ) );
    
      entities[ ent ].setSprite( Sprite( textureHandle, Rect( Vec2( 0.0f, 0.0f ), Vec2( 1.0f / 5.0f, 1.0f ) ) ) );
    
      Collider collider( Circle( Vec2::ZERO, 0 ) );
      entities[ ent ].setCollider( collider );
      entities[ ent ].getCollider().fitCircleToSprite();
    
      float r4 = randf( -1.0f, 1.0f );
      float r5 = randf( -1.0f, 1.0f );
      Vec2 direction( r4, r5 );
      direction = direction.normalized() * 5;
      entities[ ent ].setSolidBody( direction );
    }
  }
}

void TestScene::shutdown() {
  
}
