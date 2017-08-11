#pragma once

#include <cmath>

#define PI 3.14159265359

struct Vec2 {
  union {
    float x, u;
  };
  union {
    float y, v;
  };
};

const Vec2 VEC2_ZERO = {};
const Vec2 VEC2_ONE  = { 1.0f, 1.0f };

inline Vec2  operator+( Vec2 a, Vec2 b ) {
  return { a.x + b.x, a.y + b.y };
}

inline Vec2& operator+=( Vec2& a, Vec2 b ) {
  a = a + b;
  return a;
}

inline Vec2  operator-( Vec2 a, Vec2 b ) {
  return { a.x - b.x, a.y - b.y };
}

inline Vec2& operator-=( Vec2& a, Vec2 b ) {
  a = a - b;
  return a;
}

inline Vec2& operator-( Vec2& vec ) {
  vec = VEC2_ZERO - vec;
  return vec;
}

inline Vec2  operator*( Vec2 a, Vec2 b ) {
  return { a.x * b.x, a.y * b.y };
}

inline Vec2& operator*=( Vec2& a, Vec2 b ) {
  a = a * b;
  return a;
}

inline Vec2  operator*( Vec2 vec, float scale ) {
  return { vec.x * scale, vec.y * scale };
}

inline Vec2  operator*( float scale, Vec2 vec ) {
  return vec * scale;
}

inline Vec2& operator*=( Vec2& vec, float scale ) {
  vec = vec * scale;
  return vec;
}

inline Vec2  operator/( Vec2 vec, float factor ) {
  return { vec.x / factor, vec.y / factor };
}

inline Vec2  operator/( float factor, Vec2 vec ) {
  return vec / factor;
}

inline Vec2& operator/=( Vec2& vec, float factor ) {
  vec = vec / factor;
  return vec;
}

inline float dot( Vec2 a, Vec2 b ) {
  return a.x * b.x + a.y * b.y;
}

inline float sqrMagnitude( Vec2 vec ) {
  return dot( vec, vec );
}

inline float magnitude( Vec2 vec ) {
  return std::sqrt( sqrMagnitude( vec ) );
}

// FIXME I wanted this function to be just rotate()
// but a name collision with TransformManager::rotate()
// compelled me to rename it like this.
// I also don't want to wrap everything here in a namespace
// or make the functions into class methods.
// So either I do wrap these in a namespace or leave this function like this.
inline Vec2 rotateVec2( Vec2 vec, float orientation ) {
  float _cos = cos( orientation );
  float _sin = sin( orientation );
  return { vec.x * _cos - vec.y * _sin, vec.y * _cos + vec.x * _sin };
}
