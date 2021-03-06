#pragma once

#include <deque>

// based on http://bitsquid.blogspot.com.co/2014/08/building-data-oriented-entity-system.html and http://gamesfromwithin.com/managing-data-relationships

const u32 HANDLE_INDEX_BITS = 21;
const u32 HANDLE_GENERATION_BITS = 32 - HANDLE_INDEX_BITS;
// With 21 index bits 2 million entities are possible at a time.
const u32 MAX_ENTITIES = 1 << HANDLE_INDEX_BITS;

// index 0 is invalid so an EntityHandle can be set to 0 by default 
struct EntityHandle {
  u32 index : HANDLE_INDEX_BITS;
  u32 generation : HANDLE_GENERATION_BITS;
  operator u32() const;
}; 

class EntityManager {
  struct Generation { // can't just use u32 since they overflow at different values
    u32 generation : HANDLE_GENERATION_BITS;
  };
  const static u32 MIN_FREE_INDICES = 1024;
  static std::vector< Generation > generations;
  static std::deque< u32 > freeIndices;
  // ComponentMap is responsible, with it's set() method, to provide EntityManager
  // with functions to call in order to delete all the components from an entity when
  // it is to be deleted itself.
  template< typename T > friend struct ComponentMap;
  static std::unordered_multimap< u32, RmvCompCallback > removeComponentCallbacks;
public:
  static void initialize();
  static void shutdown();
  static EntityHandle create();
  static void destroy( EntityHandle entity );
  static bool isAlive( EntityHandle entity );
};
  
#ifdef NDEBUG

#define VALIDATE_ENTITY( entity ) ( ( void )0 )

#define VALIDATE_ENTITIES( entities ) ( ( void )0 )

#define VALIDATE_ENTITIES_EQUAL( entitiesA, entitiesB ) ( ( void )0 )

#else

#define VALIDATE_ENTITY( entity ) {                                     \
    ASSERT( EntityManager::isAlive( ( entity ) ), "Invalid entity id %d", ( entity ) ); \
  }

#define VALIDATE_ENTITIES( entities ) {                               \
    for ( u32 entInd = 0; entInd < ( entities ).size(); ++entInd ) {	\
      VALIDATE_ENTITY( ( entities )[ entInd ] );                      \
    }                                                                 \
  }

#define VALIDATE_ENTITIES_EQUAL( entitiesA, entitiesB ) {               \
    ASSERT( ( entitiesA ).size() == ( entitiesB ).size(), "Entity lists must have the same size" ); \
    for ( u32 entInd = 0; entInd < ( entitiesA ).size(); ++entInd ) {   \
      ASSERT( ( entitiesA )[ entInd ] == ( entitiesB )[ entInd ], "Entity id %d != entity id %d", ( entitiesA )[ entInd ], ( entitiesB )[ entInd ] ); \
    }                                                                   \
  }

#endif

struct LookupResult {
  std::vector< EntityHandle > entities;
  std::vector< ComponentIndex > indices;
};

template< typename T >
struct ComponentMap {
  std::vector< T > components;
  ComponentIndex map[ MAX_ENTITIES ];

  ComponentMap();
  void set( EntityHandle entity, T component, RmvCompCallback rmvComp );
  void remove( EntityHandle entity );
  void lookup( const std::vector< EntityHandle >& entities, LookupResult* result );
};

template< typename T >
ComponentMap< T >::ComponentMap() {
  components.push_back( {} );
}

template< typename T >
void ComponentMap< T >::set( EntityHandle entity, T component, RmvCompCallback rmvComp ) {
  VALIDATE_ENTITY( entity );
  components.push_back( component );
  u32 compInd = components.size() - 1;
  map[ entity ] = compInd;
  // tell the EntityManager how to remove this component
  // for when it needs to destroy the entity
  EntityManager::removeComponentCallbacks.insert( { entity, rmvComp } );
}

template< typename T >
void ComponentMap< T >::remove( EntityHandle entity ) {
  VALIDATE_ENTITY( entity );
  ComponentIndex compInd = map[ entity ];
  ASSERT( compInd > 0, "Entity %d has no given component", entity );
  // replace comp-to-remove with last one, thus removing it,
  // and erase last element
  components[ compInd ] = components[ components.size() - 1 ];
  components.erase( components.end() - 1 );
  // ...update comp-not-to-remove in map
  map[ components[ compInd ].entity ] = compInd;
  // and remove comp-to-remove in map
  map[ entity ] = 0;
}

template< typename T >
void ComponentMap< T >::lookup( const std::vector< EntityHandle >& entities, LookupResult* result ) {
  VALIDATE_ENTITIES( entities );
  ASSERT( result->entities.size() == 0 && result->indices.size() == 0, "" );
  u32 maxSize = entities.size();
  result->entities.reserve( maxSize );
  result->indices.reserve( maxSize );
  for ( u32 entInd = 0; entInd < maxSize; ++entInd ) {
    EntityHandle entity = entities[ entInd ];
    ComponentIndex compInd = map[ entity ];
    if ( compInd > 0 ) {
      result->entities.push_back( entity );
      result->indices.push_back( compInd );
    }
  }
}
