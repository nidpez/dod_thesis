#pragma once

//////////////////////////////////////////////////////////////////////////////

#ifdef DOD
#include "Math.hpp"
#elif defined OOP
#include "MathOOP.hpp"
#endif

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

#define UNUSED( x ) ( void )( x ) 

const float PIXELS_PER_UNIT = 4.0f;

enum ShapeType { CIRCLE, AARECT };

#ifdef DOD

struct Circle {
  Vec2 center;
  float radius;
};

// Axis aligned bounding box 
struct Rect {
  Vec2 min;
  Vec2 max;
};

struct Shape {
  union {
    Circle circle;
    Rect aaRect;
  };
  ShapeType type;
};

#elif defined OOP

class Shape {
public:
  virtual ShapeType getType() const = 0;
  virtual ~Shape() = default;
};

class Circle : public Shape {
protected:
  Vec2 center;
  float radius;
public:
  Circle( Vec2 center, float radius ) : center( center ), radius( radius ) {}
  void setCenter( Vec2 center ) { this->center = center; }
  void setRadius( float radius ) { this->radius = radius; }
  Vec2 getCenter() const { return center; }
  float getRadius() const { return radius; }
  ShapeType getType() const { return ShapeType::CIRCLE; }
};

class Rect : public Shape {
protected:
  Vec2 min;
  Vec2 max;
public:
  Rect() : min(), max() {}
  Rect( Vec2 min, Vec2 max ) : min( min ), max( max ) {}
  void setMin( Vec2 min ) { this->min = min; }
  void setMax( Vec2 max ) { this->max = max; }
  Vec2 getMin() const { return min; }
  Vec2 getMax() const { return max; }
  ShapeType getType() const { return ShapeType::AARECT; }
};

#endif

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
#include <deque>
#include <stack>
#include <list>
#include <utility>

#include "Debug.hpp"
#include "Asset.hpp"

//////////////////////////// Entity Handle system ////////////////////////////
//////////////////////////// & Component Manager  ////////////////////////////

// index 0 is invalid so an EntityHandle can be set to 0 by default 
#ifdef DOD
struct EntityHandle; 

typedef void ( *RmvCompCallback )( EntityHandle entity );

typedef u32 ComponentIndex;
#endif

//////////////////////////////////////////////////////////////////////////////

#ifdef DOD
#include "EntityManager.hpp"
#include "CompManagers.hpp"
#elif defined OOP
#include "CompManagersOOP.hpp"
#include "EntityOOP.hpp"
#endif
