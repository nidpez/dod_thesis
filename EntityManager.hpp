#pragma once

#include <deque>
#include <unordered_map>
#include <vector>


// based on http://bitsquid.blogspot.com.co/2014/08/building-data-oriented-entity-system.html and http://gamesfromwithin.com/managing-data-relationships
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
  static std::unordered_multimap< u32, removeComponentCallback > removeComponentCallbacks;
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

#endif

#define VALIDATE_ENTITY( entity )                                       \
  ASSERT( EntityManager::isAlive( ( entity ) ), "Invalid entity id %d", ( entity ) )

#define VALIDATE_ENTITIES( entities ) {                               \
    for ( u32 entInd = 0; entInd < ( entities ).size(); ++entInd ) {	\
      VALIDATE_ENTITY( ( entities )[ entInd ] );                      \
    }                                                                 \
  }

template< typename T >
struct ComponentMap {
  std::vector< T > components;
  std::unordered_map< u32, ComponentIndex > map;
  
  void set( EntityHandle entity, T component, removeComponentCallback f );
  void remove( EntityHandle entity );
  std::vector< EntityHandle > have( const std::vector< EntityHandle >& entities );
  ComponentIndex lookup( EntityHandle entity );
};

template< typename T >
void ComponentMap< T >::set( EntityHandle entity, T component, removeComponentCallback f ) {
  VALIDATE_ENTITY( entity );
  components.push_back( component );
  u32 compInd = components.size() - 1;
  bool inserted = map.insert( { entity, compInd } ).second;  
  ASSERT( inserted, "Could not map entity %d to component index %d", entity, compInd );
  // tell the EntityManager how to remove this component
  // for when it needs to destroy the entity
  EntityManager::removeComponentCallbacks.insert( { entity, f } );
}

template< typename T >
void ComponentMap< T >::remove( EntityHandle entity ) {
  VALIDATE_ENTITY( entity );
  auto iterator = map.find( entity );
  ASSERT( iterator != map.end(), "Entity %d has no given component", entity );
  components.erase( components.begin() + iterator->second );
  map.erase( iterator );
}

template< typename T >
std::vector< EntityHandle > ComponentMap< T >::have( const std::vector< EntityHandle >& entities ) {
  VALIDATE_ENTITIES( entities );  
  std::vector< EntityHandle > result;
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    if ( map.count( entities[ entInd ] ) ) {
      result.push_back( entities[ entInd ] );
    }
  }
  return result;
}

template< typename T >
ComponentIndex ComponentMap< T >::lookup( EntityHandle entity ) {
  VALIDATE_ENTITY( entity );
  auto iterator = map.find( entity );
  ASSERT( iterator != map.end(), "Entity %d has no given component", entity );
  return iterator->second;
}
