#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL2.h>

#include <deque>
#include <unordered_map>
#include <vector>

typedef int64_t 	s64;
typedef int32_t 	s32;
typedef int16_t 	s16;
typedef int8_t		s8;
typedef uint64_t	u64;
typedef uint32_t 	u32;
typedef uint16_t 	u16;
typedef uint8_t 	u8;

//////////////////////////// Entity Handle system ////////////////////////////
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
public:
  static void initialize();
  static void shutdown();
  static std::vector< EntityHandle > create( u32 amount );
  static void destroy( const std::vector< EntityHandle >& entities );
  static bool isAlive( EntityHandle entity );
};

//////////////////////////// Component Manager common ////////////////////////////

typedef u32 ComponentIndex;

struct SetComponentMapArg {
  EntityHandle entity;
  ComponentIndex compInd;
};

struct ComponentMap {
  std::unordered_map< u32, ComponentIndex > map;
  void set( const std::vector< SetComponentMapArg >& mappedPairs );
  std::vector< EntityHandle > have( const std::vector< EntityHandle >& entities );
  std::vector< ComponentIndex > lookup( const std::vector< EntityHandle >& entities );
};

//////////////////////////// Renderer common ////////////////////////////

// assumes a single possible shader per system
struct RenderInfo {
  // shader's uniforms' locations
  // TODO use 2x2 matrix for projection transform
  // for now, left, right, bottom, top
  s32 projUnifLoc[ 4 ];
  u32 vboIds[ 2 ]; // expect to mostly use one
  u32 vaoId;
  u32 shaderProgramId;
};