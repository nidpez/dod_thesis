#pragma once

#include <cmath>

struct Vec2 {
  union {
    float x, u;
  };
  union {
    float y, v;
  };
  static const Vec2 zero; 
  static const Vec2 one;
};

Vec2 operator+( Vec2 a, Vec2 b );

Vec2& operator+=( Vec2& a, Vec2 b );

Vec2 operator-( Vec2 a, Vec2 b );

Vec2& operator-=( Vec2& a, Vec2 b );

Vec2& operator-( Vec2& vec );

Vec2 operator*( Vec2 a, Vec2 b );

Vec2& operator*=( Vec2& a, Vec2 b );

Vec2 operator*( Vec2 vec, float scale );

Vec2 operator*( float scale, Vec2 vec );

Vec2& operator*=( Vec2& vec, float scale );

float dot( Vec2 a, Vec2 b );

Vec2 rotateVec2( Vec2 vec, float orientation );


const Vec2 Vec2::zero = {};
const Vec2 Vec2::one = { 1.0f, 1.0f };

Vec2 operator+( Vec2 a, Vec2 b ) {
  return { a.x + b.x, a.y + b.y };
}

Vec2& operator+=( Vec2& a, Vec2 b ) {
  a = a + b;
  return a;
}

Vec2 operator-( Vec2 a, Vec2 b ) {
  return { a.x - b.x, a.y - b.y };
}

Vec2& operator-=( Vec2& a, Vec2 b ) {
  a = a - b;
  return a;
}

Vec2& operator-( Vec2& vec ) {
  vec = Vec2::zero - vec;
  return vec;
}

Vec2 operator*( Vec2 a, Vec2 b ){
  return { a.x * b.x, a.y * b.y };
}

Vec2& operator*=( Vec2& a, Vec2 b ) {
  a = a * b;
  return a;
}

Vec2 operator*( Vec2 vec, float scale ) {
  return { vec.x * scale, vec.y * scale };
}

Vec2 operator*( float scale, Vec2 vec ) {
  return { vec.x * scale, vec.y * scale };
}

Vec2& operator*=( Vec2& vec, float scale ) {
  vec = vec * scale;
  return vec;
}

float dot( Vec2 a, Vec2 b ) {
  return a.x * b.x + a.y * b.y;
}

Vec2 rotateVec2( Vec2 vec, float orientation ){
  float _cos = cos( orientation );
  float _sin = sin( orientation );
  return { vec.x * _cos - vec.y * _sin, vec.y * _cos + vec.x * _sin };
}
