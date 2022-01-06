
#ifndef GH_GRAPHICS_H_
#define GH_GRAPHICS_H_

#include "color.h"
#include "shader.h"

namespace gh {
  struct Viewport;

  typedef unsigned short IndexType;

  enum PrimitiveType
  {
    PRIMITIVE_TRIANGLELIST,
    PRIMITIVE_TRIANGLESTRIP,
    PRIMITIVE_TRIANGLEFAN,
    PRIMITIVE_LINELIST,
    PRIMITIVE_LINESTRIP
  };

  enum LockType
  {
    LOCK_READ,
    LOCK_WRITE,
    LOCK_READWRITE
  };

  enum TextureFormat
  {
    TEXTURE_UNKNOWN,
    TEXTURE_NONE,
    TEXTURE_ABGR32F,
    TEXTURE_XBGR8,
    TEXTURE_ABGR8,
    TEXTURE_ARGB4,
    TEXTURE_A8,
    TEXTURE_R16F,
    TEXTURE_R32F,
    TEXTURE_G16R16,
    TEXTURE_D16,
    TEXTURE_D32,
    TEXTURE_D24S8,
    TEXTURE_ARGB8,
    TEXTURE_XRGB8,
    TEXTURE_FORMAT_END,
  };

  static const bool kHasAlpha[] = {
    false,//TEXTURE_UNKNOWN,
    false,//TEXTURE_NONE,
    true,//TEXTURE_ABGR32F,
    false,//TEXTURE_XBGR8,
    true,//TEXTURE_ABGR8,
    true,//TEXTURE_ARGB4,
    true,//TEXTURE_A8,
    false,//TEXTURE_R16F,
    false,//TEXTURE_R32F,
    false,//TEXTURE_G16R16,
    false,//TEXTURE_D16,
    false,//TEXTURE_D32,
    false,//TEXTURE_D24S8,
    true,//TEXTURE_ARGB8,
    false,//TEXTURE_XRGB8,
  };

  struct VertexBuffer
  {
    virtual void Release() = 0;
    virtual void WriteVertex(void* src) = 0;
    virtual int Format()const = 0;
    virtual int Vertices()const = 0;

    inline virtual ~VertexBuffer() {};
  };

  struct IndexBuffer
  {
    virtual void Release() = 0;
    virtual void WriteIndex(void* src) = 0;
    virtual int Indices()const = 0;
    virtual PrimitiveType Type()const = 0;

    inline virtual ~IndexBuffer() {};
  };


  struct LockInfo
  {
    unsigned char* Bits;
    int Pitch;
  };

  struct TextureDescription
  {
    unsigned int width;
    unsigned int height;
    TextureFormat format;
  };

  struct Texture
  {
    virtual void Release() = 0;
    virtual const TextureDescription& Desc()const = 0;
    virtual bool Lock(LockInfo& li, LockType lt) = 0;
    virtual bool LockRect(LockInfo& li, LockType lt, int left, int top, int right, int bottom) = 0;
    virtual void Unlock() = 0;

    inline virtual bool isInverse()const { return false; };//only OpenGL Rendertarget

    inline virtual ~Texture() {}
  };

  enum RenderState
  {
    RENDER_SHADEMODE,
    RENDER_CULLMODE,
    RENDER_Z,
    RENDER_ZFUNC,
    RENDER_ZWRITE,
    RENDER_COLORWRITE,
    RENDERSTATE_NUM
  };

  enum RenderValue
  {
    VALUE_DISABLE = 0,
    VALUE_ENABLE
  };

  enum ShadeMode
  {
    SHADE_GOURAUD,
    SHADE_FLAT
  };

  enum CompareFunction
  {
    COMPARE_ALWAYS,
    COMPARE_EQUAL,
    COMPARE_GREATER,
    COMPARE_GREATEREQUAL,
    COMPARE_LESS,
    COMPARE_LESSEQUAL,
    COMPARE_NEVER,
    COMPARE_NOTEQUAL
  };

  enum AlphaBlendMode
  {
    ALPHABLEND_NONE,
    ALPHABLEND_MODULATE,
    ALPHABLEND_ADD
  };

  enum SamplerType
  {
    SAMPLER_ADDRESSU,
    SAMPLER_ADDRESSV,
    SAMPLER_MAGFILTER,
    SAMPLER_MINFILTER,
    SAMPLER_MIPFILTER
  };

  enum TextureAdressing
  {
    ADDRESS_WRAP,
    ADDRESS_MIRROR,
    ADDRESS_CLAMP
  };

  enum TextureFiltering
  {
    FILTER_NONE,
    FILTER_POINT,
    FILTER_LINEAR
  };

  enum CullMode
  {
    CULL_CW,
    CULL_CCW,
    CULL_NONE
  };

  enum MemoryPool
  {
    POOL_DEFAULT,
    POOL_MANAGED,
    POOL_SYSTEM
  };

  struct Graphics
  {
    virtual bool InitializeFromViewport(Viewport* vp) = 0;
    virtual bool Initialize(int width, int height) = 0;
    virtual bool Release() = 0;

    virtual bool clear() = 0;
    virtual bool Flip(Viewport* vp) = 0;
    virtual bool BeginScene() = 0;
    virtual bool EndScene() = 0;

    virtual bool SetBgColor(Color color) = 0;
    virtual int Width()const = 0;
    virtual int Height()const = 0;

    virtual bool GetRenderTarget(Texture** tex) = 0;
    virtual bool SetRenderTarget(Texture* tex) = 0;

    virtual bool SetRenderState(RenderState state, unsigned int value) = 0;
    virtual bool SetAlphaBlendMode(AlphaBlendMode mode) = 0;

    virtual bool GetRenderTargetData(Texture* tex) = 0;
    virtual bool SetShader(shader::Shader* p, int slot) = 0;

    virtual bool CreateRenderTarget(int width, int height, TextureFormat color, TextureFormat depth, Texture** tex) = 0;
    virtual bool CreateVertexBuffer(VertexBuffer** vb, int format, int vertices) = 0;
    virtual bool CreateIndexBuffer(IndexBuffer** ib, int indices, PrimitiveType pt = PRIMITIVE_TRIANGLELIST) = 0;
    virtual bool CreateTexture(Texture** tex, int width, int height, TextureFormat format, MemoryPool pool = POOL_MANAGED) = 0;

    virtual shader::Shader* CreateShader(shader::ShaderType st) = 0;
    virtual bool DeleteShader(shader::Shader* shader) = 0;

    template<shader::ShaderType T>typename shader::TypeInterface<T>::type* CreateShader()
    {
      return static_cast<typename shader::TypeInterface<T>::type*>(this->CreateShader(T));
    }

    virtual bool DrawPrimitive(const VertexBuffer* vb, unsigned int primitivecount = 0) = 0;
    virtual bool DrawIndexedPrimitive(const VertexBuffer* vb, const IndexBuffer* ib, int offset, int triangles) = 0;
    virtual bool DrawPrimitiveUP(PrimitiveType type, unsigned int primitivecount, const void* vtx, unsigned int vertexformat) = 0;
    virtual bool DrawIndexedPrimitiveUP(PrimitiveType type, unsigned int primitivecount, const unsigned short* idx, const void* vtx, unsigned int vertexcount, unsigned int vertexformat) = 0;
  protected:
    inline Graphics() {}
    inline virtual ~Graphics() {}
  };

  Graphics* CreateGraphicsD3D();
  Graphics* CreateGraphicsEGL();
  Graphics* CreateGraphicsMock();
}

#endif
