#include "EngineCommon.hpp"

#include "Debug.hpp"

//////////////////////////// Entity Handle system ////////////////////////////

std::vector< EntityManager::Generation > EntityManager::generations;
std::deque< u32 > EntityManager::freeIndices;

EntityHandle::operator u32() const {
  return this->generation << HANDLE_INDEX_BITS | this->index;
}

void EntityManager::initialize() {
}

void EntityManager::shutdown() {
}

std::vector< EntityHandle > EntityManager::create( u32 amount ){
  std::vector< EntityHandle > newEntities( amount );
  for ( u32 entInd = 0; entInd < amount; ++entInd ) {
    u32 index;
    if ( freeIndices.size() < MIN_FREE_INDICES ) {
      generations.push_back( { 0 } );
      index = generations.size();    
      ASSERT( index < MAX_ENTITIES, "Tried to create more than %d entities", MAX_ENTITIES ); 
    } else {
      index = freeIndices.front();
      freeIndices.pop_front();
    }
    EntityHandle newEntity = { index, generations[ index - 1 ].generation };
    newEntities[ entInd ] = newEntity;
  }
  return newEntities;
}

bool EntityManager::isAlive( EntityHandle entity ) {
  return entity.index > 0 && generations[ entity.index - 1 ].generation == entity.generation;
}

//////////////////////////// Component Manager common ////////////////////////////

void ComponentMap::set( const std::vector< SetComponentMapArg >& mappedPairs ) {
  for ( u32 pairInd = 0; pairInd < mappedPairs.size(); ++pairInd ) {
    EntityHandle entity = mappedPairs[ pairInd ].entity;
    ComponentIndex compInd = mappedPairs[ pairInd ].compInd;
    bool inserted = map.insert( { entity, compInd } ).second;  
    ASSERT( inserted, "Could not map entity %d to component index %d", entity, compInd );
  }
}

std::vector< EntityHandle > ComponentMap::have( const std::vector< EntityHandle >& entities ) {
  VALIDATE_ENTITIES( entities );  
  std::vector< EntityHandle > result;
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    if ( map.count( entities[ entInd ] ) ) {
      result.push_back( entities[ entInd ] );
    }
  }
  return result;
}

std::vector< ComponentIndex > ComponentMap::lookup( const std::vector< EntityHandle >& entities ) {
  VALIDATE_ENTITIES( entities );
  std::vector< ComponentIndex > result( entities.size() );
  for ( u32 entInd = 0; entInd < entities.size(); ++entInd ) {
    auto iterator = map.find( entities[ entInd ] );
    ASSERT( iterator != map.end(), "Entity %d has no given component", entities[ entInd ] );
    result[ entInd ] = iterator->second;
  }
  return result;
}
