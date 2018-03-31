#pragma once

#include <cmath>

#define PI 3.14159265359

class Vec2 {
  float x, y;
public:
  Vec2( float x, float y ) : x( x ), y( y ) {}
  Vec2() {
    *this = ZERO;
  }
  
  inline void setX( float x ) { this->x = x; }
  inline void setY( float y ) { this->y = y; }
  inline void setU( float u ) { x = u; }
  inline void setV( float v ) { y = v; }

  inline float getX() { return x; }
  inline float getY() { return y; }
  inline float getU() { return x; }
  inline float getV() { return y; }
  
  inline bool operator==( Vec2 b ) const {
    return x == b.x && y == b.y;
  }

  inline Vec2 operator+( Vec2 b ) const {
    return { x + b.x, y + b.y };
  }

  inline Vec2& operator+=( Vec2 b ) {
    *this = *this + b;
    return *this;
  }

  inline Vec2 operator-( Vec2 b ) const {
    return { x - b.x, y - b.y };
  }

  inline Vec2& operator-=( Vec2 b ) {
    *this = *this - b;
    return *this;
  }

  inline Vec2 operator*( Vec2 b ) const {
    return { x * b.x, y * b.y };
  }

  inline Vec2& operator*=( Vec2 b ) {
    *this = *this * b;
    return *this;
  }

  inline Vec2 operator*( float scale ) const {
    return { x * scale, y * scale };
  }

  inline Vec2& operator*=( float scale ) {
    *this = *this * scale;
    return *this;
  }

  inline Vec2 operator/( float factor ) const {
    return { x / factor, y / factor };
  }

  inline Vec2& operator/=( float factor ) {
    *this = *this / factor;
    return *this;
  }

  inline float dot( Vec2 b ) const {
    return x * b.x + y * b.y;
  }

  inline float sqrMagnitude() const {
    return dot( *this );
  }

  inline float magnitude() const {
    return std::sqrt( sqrMagnitude() );
  }

  inline Vec2 rotate( float orientation ) const {
    float _cos = cos( orientation );
    float _sin = sin( orientation );
    return { x * _cos - y * _sin, y * _cos + x * _sin };
  }

  inline Vec2 normalized() const {
    return *this / magnitude();
  }
  
  static const Vec2 ZERO, ONE;
};

inline Vec2 operator-( Vec2 vec ) {
  return Vec2::ZERO - vec;
}

inline Vec2 operator*( float scale, Vec2 vec ) {
  return vec * scale;
}

inline Vec2  operator/( float factor, Vec2 vec ) {
  return vec / factor;
}
