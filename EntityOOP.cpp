#include "EngineCommon.hpp"

std::vector< Entity* > Entity::allEntities;

Entity::Entity() {
  allEntities.push_back( this );
}
std::vector< Entity* > Entity::getAllEntities() {
  return allEntities;
}
