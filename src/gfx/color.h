#ifndef COLOR_H_INCLUDED
#define COLOR_H_INCLUDED

#include "geometory.h"

namespace gh {
  struct Color
  {
    float r, g, b, a;

    inline Color() {}
    inline Color(const float r, const float g, const float b, const float a = 1)
      :r(r), g(g), b(b), a(a) {}
    inline Color(unsigned int abgr8)
      : r((abgr8 & 0xff) / 255.0f)
      , g(((abgr8 >> 8) & 0xff) / 255.0f)
      , b(((abgr8 >> 16) & 0xff) / 255.0f)
      , a((abgr8 >> 24) / 255.0f) {}
    inline Color(const Vector4& f) {
      r = f.x;
      g = f.y;
      b = f.z;
      a = f.w;
    }
    inline Color fromRGBf(const Vector3& f) {
      r = f.x;
      g = f.y;
      b = f.z;
      a = 1.0;
      return *this;
    }
    inline Color fromRGBAf(const Vector4& f) {
      r = f.x;
      g = f.y;
      b = f.z;
      a = f.w;
      return *this;
    }
    inline unsigned int ARGB8()const
    {
      int ir = (int)(r * 255);
      if (ir < 0)ir = 0;
      else if (ir > 255)ir = 255;
      int ig = (int)(g * 255);
      if (ig < 0)ig = 0;
      else if (ig > 255)ig = 255;
      int ib = (int)(b * 255);
      if (ib < 0)ib = 0;
      else if (ib > 255)ib = 255;
      int ia = (int)(a * 255);
      if (ia < 0)ia = 0;
      else if (ia > 255)ia = 255;
      return (ia << 24) | (ir << 16) | (ig << 8) | ib;
    }
    inline unsigned int ABGR8()const
    {
      int ir = (int)(r * 255);
      if (ir < 0)ir = 0;
      else if (ir > 255)ir = 255;
      int ig = (int)(g * 255);
      if (ig < 0)ig = 0;
      else if (ig > 255)ig = 255;
      int ib = (int)(b * 255);
      if (ib < 0)ib = 0;
      else if (ib > 255)ib = 255;
      int ia = (int)(a * 255);
      if (ia < 0)ia = 0;
      else if (ia > 255)ia = 255;
      return (ia << 24) | (ib << 16) | (ig << 8) | ir;
    }
    inline const float* ToArray()const
    {
      return (const float*)this;
    }
    inline operator const Vector4& ()const
    {
      return *reinterpret_cast<const Vector4*>(this);
    }
    void Normalize()
    {
      float sq = sqrtf(r * r + g * g + b * b);
      r /= sq;
      g /= sq;
      b /= sq;
    }
    void Mul(float f) {
      r *= f;
      g *= f;
      b *= f;
      a *= f;
    }
    inline bool HasColor()const {
      return 0 != a;
    }
  };

  struct HSV {
    float h;
    float s;
    float v;

    inline HSV() {
    }
    inline HSV(float h, float s, float v)
      :h(h), s(s), v(v)
    {
    }

    Color toRGB()
    {
      if (0 == s)
      {
        return Color(v, v, v);
      }
      int i = (int)(h * 6);
      float f = h * 6 - i;
      float p = v * (1 - s);
      float q = v * (1 - f * s);
      float t = v * (1 - (1 - f) * s);
      switch (i)
      {
      case 0:
        return Color(v, t, p);
      case 1:
        return Color(q, v, p);
      case 2:
        return Color(p, v, t);
      case 3:
        return Color(p, q, v);
      case 4:
        return Color(t, p, v);
      default:
        return Color(v, p, q);
      }
    }
  };

  inline Color Lerp(Color a, Color b, float f)
  {
    float i = 1 - f;
    Color c;
    c.r = a.r * i + b.r * f;
    c.g = a.g * i + b.g * f;
    c.b = a.b * i + b.b * f;
    c.a = a.a * i + b.a * f;
    return c;
  }

  inline float Lerp(float a, float b, float f)
  {
    return a * (1 - f) + b * f;
  }

}

#endif