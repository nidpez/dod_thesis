#include "EngineCommon.hpp"

#include "Debug.hpp"

//////////////////////////// Entity Handle system ////////////////////////////

std::vector< EntityManager::Generation > EntityManager::generations;
std::deque< u32 > EntityManager::freeIndices;
std::unordered_multimap< u32, removeComponentCallback > EntityManager::removeComponentCallbacks;

EntityHandle::operator u32() const {
  return this->generation << HANDLE_INDEX_BITS | this->index;
}

void EntityManager::initialize() {
}

void EntityManager::shutdown() {
}

EntityHandle EntityManager::create() {
  u32 index;
  if ( freeIndices.size() < MIN_FREE_INDICES ) {
    generations.push_back( { 0 } );
    index = generations.size();    
    ASSERT( index < MAX_ENTITIES, "Tried to create more than %d entities", MAX_ENTITIES ); 
  } else {
    index = freeIndices.front();
    freeIndices.pop_front();
  }
  return { index, generations[ index - 1 ].generation };
}

bool EntityManager::isAlive( EntityHandle entity ) {
  return entity.index > 0 && generations[ entity.index - 1 ].generation == entity.generation;
}

void EntityManager::destroy( EntityHandle entity ) {
  VALIDATE_ENTITY( entity );
  // remove all of this entity's components
  auto range = removeComponentCallbacks.equal_range( entity );
  for ( auto iter = range.first; iter != range.second; ++iter ) {
    ( iter->second )( entity );
  }
  // remove the entity
  u32 index = entity.index - 1; // we count from 1
  ++generations[ index ].generation;
  freeIndices.push_back( index );
}

//////////////////////////// Component Manager common ////////////////////////////

template< typename T >
void ComponentMap< T >::set( EntityHandle entity, T component ) {
  VALIDATE_ENTITY( entity );
  components.push_back( component );
  u32 compInd = components.size() - 1;
  bool inserted = map.insert( { entity, compInd } ).second;  
  ASSERT( inserted, "Could not map entity %d to component index %d", entity, compInd );
  // tell the EntityManager how to remove this component
  // for when it needs to destroy the entity
  EntityManager::removeComponentCallbacks.insert( { entity, this->remove } );
}

template< typename T >
void ComponentMap< T >::remove( EntityHandle entity ) {
  VALIDATE_ENTITY( entity );
  auto iterator = map.find( entity );
  ASSERT( iterator != map.end(), "Entity %d has no given component", entity );
  components.erase( iterator->second );
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

template struct ComponentMap< TransformManager::TransformComp >;
