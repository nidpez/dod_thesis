#pragma once

//////////////////////////////////////////////////////////////////////////////

#include "Math.hpp"

//////////////////////////////////////////////////////////////////////////////

#include <cstdint>

typedef int64_t 	s64;
typedef int32_t 	s32;
typedef int16_t 	s16;
typedef int8_t		s8;
typedef uint64_t	u64;
typedef uint32_t 	u32;
typedef uint16_t 	u16;
typedef uint8_t 	u8;

/////////////////////////////// Renderer common //////////////////////////////

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <SOIL2.h>

const float PIXELS_PER_UNIT = 4.0f;

struct Circle {
  Vec2 center;
  float radius;
};

struct Color {
  float r, g, b, a;
};

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

//////////////////////////////////////////////////////////////////////////////

#include <unordered_map>
#include <vector>

#include "Debug.hpp"
#include "Asset.hpp"

//////////////////////////// Entity Handle system ////////////////////////////
//////////////////////////// & Component Manager  ////////////////////////////

// index 0 is invalid so an EntityHandle can be set to 0 by default 
struct EntityHandle; 

typedef void ( *RmvCompCallback )( EntityHandle entity );

typedef u32 ComponentIndex;

//////////////////////////////////////////////////////////////////////////////

#include "EntityManager.hpp"
#include "CompManagers.hpp"
