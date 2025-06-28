#ifndef MAX_H
#define MAX_H

#include "BoltID.h"
#include "box2d.h"
#include "fwd.hpp"
#include "id.h"
#include "types.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>
#include <glm.hpp>


namespace max{

    
    template<typename vectype>
    struct vec2;

    //! for util no other implementation
    template<typename vectype>
    struct vec3;
    template<typename vectype>
    struct vec4;
    //!
    
    using vec2f = vec2<float>;
    using vec2i = vec2<int>;
    using vec2d = vec2<double>;
    using vec2s = vec2<short>;
    using vec2l = vec2<long>;

    float degtorad(const float& deg);
    float radtodeg(const float& deg);

    template<typename vectype>
    inline auto magnitude(const vec2<vectype>& vec);
    template<typename vectype>
    inline constexpr auto magnitude_sqr(const vec2<vectype>& vec);
    
    template<typename vectype>
    inline  vec2<vectype> normalized(const vec2<vectype>& vec);
    template<typename vectype>
    inline  void normalize(vec2<vectype>& vec);

    template<typename vectype>
    inline constexpr vectype dot(const vec2<vectype>& a, const vec2<vectype>& b);

    template<typename vectype>
    inline constexpr vectype distance(const vec2<vectype>& a, const vec2<vectype>& b);

    template<typename vectype>
    inline vectype angle(const vec2<vectype>& vec);
    template<typename vectype>
    inline vectype angledeg(const vec2<vectype>& vec);


    template<typename vectype>
    inline void rotate(vec2<vectype>& vec,const float& radians);
    template<typename vectype>
    inline void rotate_deg(vec2<vectype>& vec,const float& deg);


    template<typename vectype>
    inline vec2<vectype> rotated(const vec2<vectype>& vec,const float& radians);
    template<typename vectype>
    inline vec2<vectype> rotated_deg(const vec2<vectype>& vec,const float& deg);
    template<typename vectype>
    inline void rotate_around(vec2<vectype>& point, const vec2<vectype>& center, float angle_rad);


    template<typename vectype>
    inline constexpr vec2<vectype> zero();

    template<typename vectype>
    inline constexpr bool is_zero(const vec2<vectype>& a);

    template<typename vectype>
    inline constexpr void lerp(vec2<vectype>& ref,const vec2<vectype>& start,const vec2<vectype>& end,float step);
    template<typename vectype>
    inline constexpr vec2<vectype> lerped(const vec2<vectype>& start,const vec2<vectype>& end,float step);

    template<typename vectype>
    inline constexpr vec2<vectype> clamped(const vec2<vectype>& vec,const vec2<vectype>& min,const vec2<vectype>& max);
    template<typename vectype>
    inline constexpr void clamp(vec2<vectype>& ref,const vec2<vectype>& min,const vec2<vectype>& max);

    template<typename vectype>
    inline constexpr vec2<vectype> absoluted(const vec2<vectype>& a);
    template<typename vectype>
    inline constexpr void absolute(vec2<vectype>& ref);

    template<typename vectype>
    inline constexpr vec2<vectype> floor(const vec2<vectype>& a);
    template<typename vectype>
    inline constexpr vec2<vectype> ceil(const vec2<vectype>& a);
    template<typename vectype>
    inline constexpr vec2<vectype> round(const vec2<vectype>& a);

    template<typename vectype>
    inline constexpr void floored(vec2<vectype>& a);
    template<typename vectype>
    inline constexpr void ceiled(vec2<vectype>& a);
    template<typename vectype>
    inline constexpr void rounded(vec2<vectype>& a);

    template<typename vectype>
    inline constexpr void perp(vec2<vectype>& a);
    

    
};

namespace max{
  
  //!for util no other implementation
  template<typename vectype>
  struct vec4{
    vectype x,y,z,w;
  };
  template<typename vectype>
  struct vec3{
    vectype x,y,z;
  };
  //!

  template<typename vectype>
  struct vec2 {
    vectype x, y;

    constexpr vec2() = default;
    constexpr vec2(vectype x, vectype y) : x(x), y(y) {}

    template<typename otherType>
    constexpr vec2 operator+(otherType num) const {
        using T = std::common_type_t<vectype, otherType>;
        return vec2<T>(static_cast<T>(x) + static_cast<T>(num),
                       static_cast<T>(y) + static_cast<T>(num));
    }
    template<typename otherType>
    constexpr vec2 operator-(otherType num) const {
        using T = std::common_type_t<vectype, otherType>;
        return vec2<T>(static_cast<T>(x) - static_cast<T>(num),
                       static_cast<T>(y) - static_cast<T>(num));
    }
    template<typename otherType>
    constexpr vec2 operator*(otherType num) const {
        using T = std::common_type_t<vectype, otherType>;
        return vec2<T>(static_cast<T>(x) * static_cast<T>(num),
                       static_cast<T>(y) * static_cast<T>(num));
    }
    template<typename otherType>
    constexpr vec2 operator/(otherType num) const {
        using T = std::common_type_t<vectype, otherType>;
        return vec2<T>(static_cast<T>(x) / static_cast<T>(num),
                       static_cast<T>(y) / static_cast<T>(num));
    }

    template<typename otherType>
    constexpr auto operator+(const vec2<otherType>& other) const {
        using T = std::common_type_t<vectype, otherType>;
        return vec2<T>(static_cast<T>(x) + static_cast<T>(other.x),
                       static_cast<T>(y) + static_cast<T>(other.y));
    }
    template<typename otherType>
    constexpr auto operator-(const vec2<otherType>& other) const {
        using T = std::common_type_t<vectype, otherType>;
        return vec2<T>(static_cast<T>(x) - static_cast<T>(other.x),
                       static_cast<T>(y) - static_cast<T>(other.y));
    }
    template<typename otherType>
    constexpr auto operator*(const vec2<otherType>& other) const {
        using T = std::common_type_t<vectype, otherType>;
        return vec2<T>(static_cast<T>(x) * static_cast<T>(other.x),
                       static_cast<T>(y) * static_cast<T>(other.y));
    }
    template<typename otherType>
    constexpr auto operator/(const vec2<otherType>& other) const {
        if(other.x==0){throw std::runtime_error("cannot divide by zero!");}
        if(other.y==0){throw std::runtime_error("cannot divide by zero!");}
        using T = std::common_type_t<vectype, otherType>;
        return vec2<T>(static_cast<T>(x) / static_cast<T>(other.x),
                       static_cast<T>(y) / static_cast<T>(other.y));
    }

    template<typename otherType>
    constexpr vec2<vectype>& operator+=(otherType num){
        x+=static_cast<vectype>(num);
        y+=static_cast<vectype>(num);
        return *this;
    }
    template<typename otherType>
    constexpr vec2<vectype>& operator-=(otherType num){
        x-=static_cast<vectype>(num);
        y-=static_cast<vectype>(num);
        return *this;
    }
    template<typename otherType>
    constexpr vec2<vectype>& operator*=(otherType num){
        x*=static_cast<vectype>(num);
        y*=static_cast<vectype>(num);
        return *this;
    }
    template<typename otherType>
    constexpr vec2<vectype>& operator/=(otherType num){
        if(num==0){throw std::runtime_error("cannot divide by zero!");}
        x/=static_cast<vectype>(num);
        y/=static_cast<vectype>(num);
        return *this;
    }
    
    template<typename otherType>
    constexpr vec2& operator+=(const vec2<otherType>& other) {
        x += static_cast<vectype>(other.x);
        y += static_cast<vectype>(other.y);
        return *this;
    }
    template<typename otherType>
    constexpr vec2& operator-=(const vec2<otherType>& other) {
        x -= static_cast<vectype>(other.x);
        y -= static_cast<vectype>(other.y);
        return *this;
    }
    template<typename otherType>
    constexpr vec2& operator*=(const vec2<otherType>& other) {
        x *= static_cast<vectype>(other.x);
        y *= static_cast<vectype>(other.y);
        return *this;
    }
    template<typename otherType>
    constexpr vec2& operator/=(const vec2<otherType>& other) {
        if(other.x==0){throw std::runtime_error("cannot divide by zero!");}
        if(other.y==0){throw std::runtime_error("cannot divide by zero!");}
        x /= static_cast<vectype>(other.x);
        y /= static_cast<vectype>(other.y);
        return *this;
    }

    constexpr vec2 operator-() const {
        return vec2(-x, -y);
    }

    template<typename U>
    constexpr bool operator==(const vec2<U>& other) const {
        return x == other.x && y == other.y;
    }
    template<typename U>
    constexpr bool operator!=(const vec2<U>& other) const {
        return !(*this == other);
    }

    friend std::ostream& operator<<(std::ostream& os, const vec2& v) {
        return os << "(" << v.x << ", " << v.y << ")";
    }
   
    static inline std::string tostring(const vec2& v){
      return std::to_string(v.x)+","+std::to_string(v.y);
    }
  };
};
namespace max {
  template<typename vectype>
  inline constexpr auto magnitude_sqr(const vec2<vectype>& vec){
      using promoted = typename std::conditional<
      std::is_integral<vectype>::value,
      double, 
      vectype
      >::type;
      return static_cast<promoted>(vec.x * vec.x + vec.y * vec.y);
  }
  template<typename vectype>
  inline auto magnitude(const vec2<vectype>& vec){
      using promoted = typename std::conditional<
      std::is_integral<vectype>::value,
      double, 
      vectype
      >::type;
      return static_cast<promoted>(
          std::sqrt(static_cast<promoted>(vec.x * vec.x + vec.y * vec.y))
      );
  } 

  template<typename vectype>
  inline vec2<vectype> normalized(const vec2<vectype>& vec) {
      double mag = magnitude(vec);
      if (mag == 0.0) return {0.0, 0.0};
      return { (vectype)(vec.x / mag), (vectype)(vec.y / mag) };
  }
  template<typename vectype>
  inline void normalize(vec2<vectype>& vec){
    double mag = magnitude(vec);
    if(mag==0.0){
        vec.x = 0;
        vec.y = 0;
        return;
    }
    vec/=mag;
  }

  template<typename vectype>
  inline constexpr vectype dot(const vec2<vectype>& a, const vec2<vectype>& b){
    auto mul = a*b;
    return mul.x+mul.y;
  }

  template<typename vectype>
  inline constexpr vectype distance(const vec2<vectype>& a, const vec2<vectype>& b){
      auto _v = b-a;
      return max::magnitude(_v);
  }

  template<typename vectype>
  inline vectype angle(const vec2<vectype>& vec){
    return std::atan2(vec.y,vec.x);
  }
  template<typename vectype>
  inline vectype angledeg(const vec2<vectype>& vec){
    return radtodeg(angle(vec));
  }

  template<typename vectype>
  inline void rotate(vec2<vectype>& vec,const float& radians){ 
    auto bycos = vec*std::cos(radians);
    auto bysin = vec*std::sin(radians);
    vec.x = bycos.x - bysin.y;
    vec.y = bysin.x + bycos.y;
  }
  template<typename vectype>
  inline void rotate_around(vec2<vectype>& point, const vec2<vectype>& center, float angle_rad) {
      float s = std::sin(angle_rad);
      float c = std::cos(angle_rad);

      vec2<vectype> p = point - center;
      float xnew = p.x * c - p.y * s;
      float ynew = p.x * s + p.y * c;
      point = vec2<vectype>{xnew,ynew}+center;
  }
  template<typename vectype>
  inline void rotate_deg(vec2<vectype>& vec,const float& deg){ 
    rotate(vec,degtorad(deg));
  }

  template<typename vectype>
  inline vec2<vectype> rotated(const vec2<vectype>& vec,const float& radians){ 
    auto bycos = vec*std::cos(radians);
    auto bysin = vec*std::sin(radians);
    vec2<vectype> newvec;
    newvec.x = bycos.x - bysin.y;
    newvec.y = bysin.x + bycos.y;
    return newvec;
  }
  template<typename vectype>
  inline vec2<vectype> rotated_deg(const vec2<vectype>& vec,const float& deg){ 
    return rotated(vec,degtorad(deg));
  }

  template<typename vectype>
  inline constexpr vec2<vectype> zero(){
      return {0,0};
  }
  template<typename vectype>
  inline constexpr bool is_zero(const vec2<vectype>& a){
    return a.x==0&&a.y==0;
  }

  template<typename vectype>
  inline constexpr void lerp(vec2<vectype>& ref,const vec2<vectype>& start,const vec2<vectype>& end,float step){
    ref = start+(end-start)*step;
  }

  template<typename vectype>
  inline constexpr vec2<vectype> lerped(const vec2<vectype>& start,const vec2<vectype>& end,float step){
    return start+(end-start)*step;
  }

  template<typename vectype>
  inline constexpr vec2<vectype> clamped(const vec2<vectype>& vec,const vec2<vectype>& min,const vec2<vectype>& max){
    return {
        std::clamp(vec.x,min.x,max.x),
        std::clamp(vec.y,min.y,max.y)
    };  
  }
  template<typename vectype>
  inline constexpr void clamp(vec2<vectype>& ref,const vec2<vectype>& min,const vec2<vectype>& max){
    ref.x = std::clamp(ref.x,min.x,max.x);
    ref.y = std::clamp(ref.y,min.y,max.y);
  }

  template<typename vectype>
  inline constexpr vec2<vectype> absoluted(const vec2<vectype>& a){
    return {
        std::abs(a.x),
        std::abs(a.y)
    };
  }
  
  template<typename vectype>
  inline constexpr void absolute(vec2<vectype>& ref){
    ref.x = std::abs(ref.x);
    ref.y = std::abs(ref.y);
  }

  template<typename vectype>
  inline constexpr vec2<vectype> floor(const vec2<vectype>& a){
    return{
        std::floor(a.x),
        std::floor(a.y),
    };
  }
  template<typename vectype>
  inline constexpr vec2<vectype> ceil(const vec2<vectype>& a){
    return{
        std::ceil(a.x),
        std::ceil(a.y),
    };
  }
  template<typename vectype>
  inline constexpr vec2<vectype> round(const vec2<vectype>& a){
    return{
        std::round(a.x),
        std::round(a.y),
    };
  }

  template<typename vectype>
  inline constexpr void floored(vec2<vectype>& a){
     a.x = std::floor(a.x);
     a.y = std::floor(a.y);
  }
  template<typename vectype>
  inline constexpr void ceiled(vec2<vectype>& a){
     a.x = std::ceil(a.x);
     a.y = std::ceil(a.y);
  }
  template<typename vectype>
  inline constexpr void rounded(vec2<vectype>& a){
     a.x = std::round(a.x);
     a.y = std::round(a.y);
  }
  template<typename vectype>
  inline constexpr void perp(vec2<vectype>& a){
    auto _x = a.x;
    auto _y = a.y;
    a.x = -_y;
    a.y = _x;
  }
  inline glm::vec2 toglm(max::vec2<float> _vec){
    return {_vec.x,_vec.y};
  }
  inline max::vec2<float> fromglm(glm::vec2 _vec){
    return {_vec.x,_vec.y};
  }

  namespace physics {
    static inline b2BodyId create_dynamic_box(b2WorldId worldid,max::vec2<float> half_scale,max::vec2<float> pos = {0.0f,0.0f},float pixels_per_meter = 32.0f,float friction = 0.3f,float density = 1.0f){
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = (b2Vec2){pos.x/pixels_per_meter, pos.y/pixels_per_meter};
        auto bodyId= b2CreateBody(worldid, &bodyDef);
        b2Polygon dynamicBox = b2MakeBox(half_scale.x/pixels_per_meter, half_scale.y/pixels_per_meter);
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = density;
        shapeDef.material.friction = friction;
        b2CreatePolygonShape(bodyId, &shapeDef, &dynamicBox);
        return bodyId;
    }
    static inline b2BodyId create_static_box(b2WorldId worldid,max::vec2<float> half_scale,max::vec2<float> pos = {0.0f,0.0f},float pixels_per_meter = 32.0f){
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
        bodyDef.position = (b2Vec2){pos.x/pixels_per_meter, pos.y/pixels_per_meter};
        auto bodyId= b2CreateBody(worldid, &bodyDef);
        b2Polygon dynamicBox = b2MakeBox(half_scale.x/pixels_per_meter, half_scale.y/pixels_per_meter);
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.material.friction = 0.3f;
        b2CreatePolygonShape(bodyId, &shapeDef, &dynamicBox);
        return bodyId;
    }
  
  }
  namespace math{
    static constexpr inline bool inside_region(float x, float y, float left, float top, float right, float bottom) {
      return x >= left && x <= right && y <= top && y >= bottom;
    }
    static inline bool auto_inside_region(float x, float y, max::vec2<float> position,max::vec2<float> size) {
      return max::math::inside_region(x,y, position.x-(size/2).x, position.y+(size/2).y, position.x+(size/2).x, position.y-(size/2).y);;
    }
  };
}


#endif // MAX_H