#ifndef FONTMAP_H_INCLUDED
#define FONTMAP_H_INCLUDED

#include "graphics.h"
#include "shape.h"
#include <stdint.h>

namespace gh {

  struct Frect {
    float x1;
    float y1;
    float x2;
    float y2;
  };

  struct Fontmap
  {
    virtual bool Init(Graphics* dev) = 0;
    /** enable outline
     * @param rad radius
     * @param den density
     */
    virtual void Outline(float rad, float den) = 0;
    virtual int GetWidth(wchar_t c) = 0;
    virtual int GetHeight(wchar_t c) = 0;
    virtual void ReadPixel(wchar_t c, int* width, int* height, uint8_t* map) = 0;
    virtual Frect GetSize(const wchar_t* str, float padding) = 0;
    virtual void Draw(const Matrix& mat, const wchar_t* str, float padding = 0, int width = 0) = 0;
    virtual void DrawColor(const Matrix& mat, const wchar_t* str, uint32_t color, float padding = 0, int width = 0) = 0;
    virtual void MakeShape(Shape* shape, const wchar_t* str, float padding = 0) = 0;
    
    static Fontmap* Create(const void* ttf, int length, int font_size);
    virtual ~Fontmap();
  };
}

#endif
