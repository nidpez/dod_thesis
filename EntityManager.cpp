#include "EngineCommon.hpp"

std::vector< EntityManager::Generation > EntityManager::generations;
std::deque< u32 > EntityManager::freeIndices;
std::unordered_multimap< u32, RmvCompCallback > EntityManager::removeComponentCallbacks;

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
    // we count from 1
    index = generations.size();
    ASSERT( index < MAX_ENTITIES, "Tried to create more than %d entities", MAX_ENTITIES );
  } else {
    // we count from 1
    index = freeIndices.front() + 1;
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
