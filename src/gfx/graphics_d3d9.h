
#ifndef WTS_GFX_GRAPHICS_D3D9_H_
#define WTS_GFX_GRAPHICS_D3D9_H_

#ifdef _WIN32

#include "graphics.h"
#include "color.h"
#include "../main/md5.h"

#include "shader_hlsl.h"

#include <d3d9.h>
#include <map>

//#include <d3dx9.h>

namespace gh {
  namespace d3d9 {

    static const int MAX_SHADER_CHAIN = 32;

    struct CompiledShader
    {
      float vetex_register[128 * 4];
      float pixel_register[128 * 4];
      LPDIRECT3DVERTEXDECLARATION9 vd;
      LPDIRECT3DVERTEXSHADER9 vs;
      LPDIRECT3DPIXELSHADER9 ps;

      int vertex_register_offset[MAX_SHADER_CHAIN];
      int pixel_register_offset[MAX_SHADER_CHAIN];
    };

    struct MD5sum
    {
      unsigned char sum[16];
      bool operator>(const MD5sum& s)const
      {
        return memcmp(sum, s.sum, 16) > 0;
      }
      bool operator<(const MD5sum& s)const
      {
        return memcmp(sum, s.sum, 16) < 0;
      }
    };

    struct TextureD3D
      :public Texture
    {
      TextureDescription desc;
      LPDIRECT3DTEXTURE9 tex;
      void Release();
      const TextureDescription& Desc()const;
      bool Lock(LockInfo& li, LockType lt);
      bool LockRect(LockInfo& li, LockType lt, int left, int top, int right, int bottom);
      void Unlock();
    };
    struct RenderTargetTextureD3D
      :public TextureD3D
    {
      LPDIRECT3DTEXTURE9 depth;
      void Release();
    };


    class GraphicsD3D
      :public Graphics
    {
    public:
      LPDIRECT3DDEVICE9 RawDevice();

      inline GraphicsD3D() {};

    private:

      bool SetupShader(int format);

    public:
      virtual bool InitializeFromViewport(Viewport* vp);
      bool Initialize(int width, int height);
      bool Release();
      //
      bool clear();
      bool Flip(Viewport* vp);
      bool BeginScene();
      bool EndScene();
      //env
      bool SetBgColor(Color color);
      int Width()const;
      int Height()const;
      bool GetRenderTarget(Texture** tex);
      bool SetRenderTarget(Texture* tex);

      bool SetRenderState(RenderState state, unsigned int value);
      bool SetAlphaBlendMode(AlphaBlendMode mode);

      bool SetTexture(Texture* tex, int stage = 0);

      //status
      bool GetRenderTargetData(Texture* tex);
      bool SetShader(shader::Shader* p, int slot);

      //create
      bool CreateRenderTarget(int width, int height, TextureFormat color, TextureFormat depth, Texture** tex);
      bool CreateVertexBuffer(VertexBuffer** vb, int format, int vertices);
      bool CreateIndexBuffer(IndexBuffer** ib, int indices, PrimitiveType pt = PRIMITIVE_TRIANGLELIST);
      bool CreateTexture(Texture** tex, int width, int height, TextureFormat format, MemoryPool pool = POOL_MANAGED);

      //shader
      bool CreateShaderFractal(shader::Fractal** out);
      bool CreateShaderDecalTexture(shader::DecalTexture** out);
      shader::NormalMap* CreateShaderNormalMap();
      shader::TangentMap* CreateShaderTangentMap();
      void CreateShaderPointicle(shader::Pointicle** out);
      shader::CenterSpecular* CreateShaderCenterSpecular();
      shader::EdgeSpecular* CreateShaderEdgeSpecular();
      shader::BlowNormal* CreateShaderBlowNormal();
      shader::SetAngleRGBA* CreateShaderSetAngleRGBA();
      shader::Shader* CreateShader(shader::ShaderType st);
      bool DeleteShader(shader::Shader* shader);

      //draw
      bool DrawPrimitive(const VertexBuffer* vb);
      bool DrawIndexedPrimitive(const VertexBuffer* vb, const IndexBuffer* ib, int offset, int triangles);
      bool DrawPrimitiveUP(PrimitiveType type, unsigned int primitivecount, const void* vtx, unsigned int vertexformat);
      bool DrawIndexedPrimitiveUP(PrimitiveType type, unsigned int primitivecount, const unsigned short* idx, const void* vtx, unsigned int vertexcount, unsigned int vertexformat);
    private:
      Color bg_color_;

      LPDIRECT3D9 d3d_;
      LPDIRECT3DDEVICE9 device_;

      int width_;
      int height_;

      class shader::Shader* shader_slot_[MAX_SHADER_CHAIN];
      std::map<MD5sum, CompiledShader> compiled_shader_;
      RenderTargetTextureD3D* render_target_;
      //RenderTargetTextureContent back_buffer_;
      unsigned int render_state_[RENDERSTATE_NUM];

      AlphaBlendMode alphablend_mode_;

      const VertexBuffer* last_vb_;
      const IndexBuffer* last_ib_;

      IDirect3DSurface9* default_surface_;

    };

    class VertexBufferD3D
      :public VertexBuffer
    {
    public:
      VertexBufferD3D(int format, int vertices);
      ~VertexBufferD3D();
    private:
      void* Lock(LockType type);
      void Unlock();
      void Release();
      void WriteVertex(void* src);
      inline int Format()const { return format_; };
      inline int Vertices()const { return vertices_; };

      LPDIRECT3DVERTEXBUFFER9 buffer_;
      int format_;
      int vertices_;

      friend class GraphicsD3D;
    };

    class IndexBufferD3D
      :public IndexBuffer
    {
    public:
      IndexBufferD3D(int indices, PrimitiveType pt);
      ~IndexBufferD3D();
    private:
      void* Lock(LockType type);
      void Unlock();
      void Release();
      void WriteIndex(void* src);
      inline int Indices()const { return indices_; };
      inline PrimitiveType Type()const { return type_; };

      LPDIRECT3DINDEXBUFFER9 buffer_;
      int indices_;
      PrimitiveType type_;

      friend class GraphicsD3D;

    };

  }//d3d9
}//gh

#endif

#endif
