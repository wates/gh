

#ifdef _WIN32

#include "../main/underscore.h"
#include "graphics_d3d9.h"
#include "../sys/viewport.h"
#include "fertex.h"
#include <sstream>
#include <vector>
#include <map>

#include <stdio.h>
#include <d3dx9.h>

#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"d3dx9.lib")

namespace gh {

  Graphics* CreateGraphicsD3D()
  {
    return new d3d9::GraphicsD3D();
  }

  namespace d3d9 {

    using namespace gh;

    static inline D3DVERTEXELEMENT9 VertexElement(WORD stream, int offset, BYTE type, BYTE method, BYTE usage, BYTE index)
    {
      D3DVERTEXELEMENT9 elem = { stream,(WORD)offset,type,method,usage,index };
      return elem;
    }

    static unsigned int EqualizeType(LockType lt)
    {
      unsigned int d3dlock = 0;
      if (LOCK_READ == lt)
        d3dlock = D3DLOCK_READONLY;
      else if (LOCK_WRITE == lt)
        d3dlock = D3DLOCK_DISCARD;
      return d3dlock;
    }

    static D3DPOOL EqualizeType(MemoryPool type)
    {
      switch (type)
      {
      case POOL_DEFAULT:
        return D3DPOOL_DEFAULT;
      case POOL_MANAGED:
        return D3DPOOL_MANAGED;
      case POOL_SYSTEM:
        return D3DPOOL_SYSTEMMEM;
      default:
        return D3DPOOL_FORCE_DWORD;
      }
    }

    //static D3DSAMPLERSTATETYPE EqualizeType(SamplerType type)
    //{
    //    switch(type)
    //    {
    //    case SAMPLER_ADDRESSU:
    //        return D3DSAMP_ADDRESSU;
    //    case SAMPLER_ADDRESSV:
    //        return D3DSAMP_ADDRESSV;
    //    case SAMPLER_MAGFILTER:
    //        return D3DSAMP_MAGFILTER;
    //    case SAMPLER_MINFILTER:
    //        return D3DSAMP_MINFILTER;
    //    case SAMPLER_MIPFILTER:
    //        return D3DSAMP_MIPFILTER;
    //    default:
    //        return D3DSAMP_FORCE_DWORD;
    //    }
    //}

    //DWORD EqualizeType(SampleState state)
    //{
    // switch(state)
    // {
    // case ADDRESS_WRAP:
    //  return D3DTADDRESS_WRAP;
    // case ADDRESS_MIRROR:
    //  return D3DTADDRESS_MIRROR;
    // case ADDRESS_CLAMP:
    //  return D3DTADDRESS_CLAMP;
    // case FILTER_NONE:
    //  return D3DTEXF_NONE;
    // case FILTER_POINT:
    //  return D3DTEXF_POINT;
    // case FILTER_LINEAR:
    //  return D3DTEXF_LINEAR;
    // default:
    //  wtl::asrt();
    //  return -1;
    // }
    //}

    static D3DPRIMITIVETYPE EqualizeType(PrimitiveType pt)
    {
      switch (pt)
      {
      case PRIMITIVE_TRIANGLELIST:
        return D3DPT_TRIANGLELIST;
      case PRIMITIVE_TRIANGLEFAN:
        return D3DPT_TRIANGLEFAN;
      case PRIMITIVE_TRIANGLESTRIP:
        return D3DPT_TRIANGLESTRIP;
      case PRIMITIVE_LINELIST:
        return D3DPT_LINELIST;
      case PRIMITIVE_LINESTRIP:
        return D3DPT_LINESTRIP;
      default:
        return D3DPT_FORCE_DWORD;
      }
    }

    static D3DFORMAT EqualizeType(TextureFormat tf)
    {
      switch (tf)
      {
      case TEXTURE_ABGR32F:
        return D3DFMT_A32B32G32R32F;
      case TEXTURE_ARGB8:
        return D3DFMT_A8R8G8B8;
      case TEXTURE_ABGR8:
        return D3DFMT_A8B8G8R8;
      case TEXTURE_XRGB8:
        return D3DFMT_X8R8G8B8;
      case TEXTURE_ARGB4:
        return D3DFMT_A4R4G4B4;
      case TEXTURE_A8:
        return D3DFMT_A8;
      case TEXTURE_R16F:
        return D3DFMT_R16F;
      case TEXTURE_R32F:
        return D3DFMT_R32F;
      case TEXTURE_G16R16:
        return D3DFMT_G16R16;
      case TEXTURE_D16:
        return D3DFMT_D16;
      case TEXTURE_D32:
        return D3DFMT_D32;
      case TEXTURE_D24S8:
        return D3DFMT_D24S8;
      default:
        return D3DFMT_UNKNOWN;
      }
    }

    //static TextureFormat EqualizeType(D3DFORMAT fmt)
    //{
    //    switch(fmt)
    //    {
    //    case D3DFMT_A32B32G32R32F:
    //        return TEXTURE_ABGR32F;
    //    case D3DFMT_X8B8G8R8:
    //        return TEXTURE_XBGR8;
    //    case D3DFMT_A8B8G8R8:
    //        return TEXTURE_ABGR8;
    //    case D3DFMT_A4R4G4B4:
    //        return TEXTURE_A4R4G4B4;
    //    case D3DFMT_A8:
    //        return TEXTURE_A8;
    //    case D3DFMT_R16F:
    //        return TEXTURE_R16F;
    //    case D3DFMT_R32F:
    //        return TEXTURE_R32F;
    //    case D3DFMT_G16R16:
    //        return TEXTURE_G16R16;
    //    case D3DFMT_D16:
    //        return TEXTURE_D16;
    //    case D3DFMT_D32:
    //        return TEXTURE_D32;
    //    case D3DFMT_D24S8:
    //        return TEXTURE_D24S8;
    //    default:
    //        return TEXTURE_UNKNOWN;
    //    }
    //}


    /////////////////////////////////////////
    //// Texture

    void TextureD3D::Release()
    {
      this->tex->Release();
      delete this;
    }

    const TextureDescription& TextureD3D::Desc() const
    {
      return this->desc;
    }

    bool TextureD3D::Lock(LockInfo& li, LockType lt)
    {
      D3DLOCKED_RECT lr;
      if (D3D_OK != tex->LockRect(0, &lr, 0, EqualizeType(lt)))
        return false;
      li.Bits = (unsigned char*)lr.pBits;
      li.Pitch = lr.Pitch;
      return true;
    }

    bool TextureD3D::LockRect(LockInfo& li, LockType lt, int left, int top, int right, int bottom)
    {
      RECT rc;
      rc.top = top;
      rc.left = left;
      rc.bottom = bottom;
      rc.right = right;
      D3DLOCKED_RECT lr;
      if (D3D_OK != tex->LockRect(0, &lr, &rc, EqualizeType(lt)))
        return false;
      li.Bits = (unsigned char*)lr.pBits;
      li.Pitch = lr.Pitch;
      return true;
    }

    void TextureD3D::Unlock()
    {
      tex->UnlockRect(0);
    }

    void RenderTargetTextureD3D::Release()
    {
      this->depth->Release();
      TextureD3D::Release();
    }

    //////////////////////////////
    // vertex buffer

    VertexBufferD3D::VertexBufferD3D(int format, int vertices)
      :format_(format)
      , vertices_(vertices)
      , buffer_(0)
    {
    }

    VertexBufferD3D::~VertexBufferD3D()
    {
    }

    void* VertexBufferD3D::Lock(LockType type)
    {
      void* p;
      buffer_->Lock(0, GetFertexSize(format_) * (vertices_), &p, EqualizeType(type));
      return p;
    }

    void VertexBufferD3D::Unlock()
    {
      buffer_->Unlock();
    }

    void VertexBufferD3D::Release()
    {
      if (buffer_)
        buffer_->Release();
      delete this;
    }

    void VertexBufferD3D::WriteVertex(void* src)
    {
      size_t size = GetFertexSize(format_);
      void* dst = Lock(LOCK_WRITE);
      memcpy(dst, src, size * vertices_);
      Unlock();
    }

    ///////////////////////////////
    // index buffer

    IndexBufferD3D::IndexBufferD3D(int indices, PrimitiveType pt)
      :indices_(indices)
      , type_(pt)
      , buffer_(0)
    {
    }

    IndexBufferD3D::~IndexBufferD3D()
    {
    }

    void* IndexBufferD3D::Lock(LockType type)
    {
      void* p;
      buffer_->Lock(0, sizeof(IndexType) * indices_, &p, EqualizeType(type));
      return p;
    }

    void IndexBufferD3D::Unlock()
    {
      buffer_->Unlock();
    }

    void IndexBufferD3D::Release()
    {
      if (buffer_)
        buffer_->Release();
      delete this;
    }

    void IndexBufferD3D::WriteIndex(void* src)
    {
      void* dst = Lock(LOCK_WRITE);
      memcpy(dst, src, sizeof(IndexType) * indices_);
      Unlock();
    }


    //////////////////////////////
    // device
    LPDIRECT3DDEVICE9 GraphicsD3D::RawDevice()
    {
      return device_;
    }

    bool GraphicsD3D::InitializeFromViewport(Viewport* vp)
    {
      d3d_ = Direct3DCreate9(D3D_SDK_VERSION);
      if (!d3d_)
        return false;
      RECT client_rect;
      GetClientRect((HWND)vp->GetWindowHandle(), &client_rect);
      D3DPRESENT_PARAMETERS pp;
      ZeroMemory(&pp, sizeof(pp));
      pp.EnableAutoDepthStencil = TRUE;
      pp.AutoDepthStencilFormat = D3DFMT_D24S8;
      {
        pp.BackBufferWidth = client_rect.right - client_rect.left;
        pp.BackBufferHeight = client_rect.bottom - client_rect.top;
        pp.Windowed = TRUE;
        pp.hDeviceWindow = GetDesktopWindow();
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.BackBufferFormat = D3DFMT_UNKNOWN;
        pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
        //pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
      }
      if (FAILED(d3d_->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, (HWND)vp->GetWindowHandle(),
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &device_)))
        return false;

      width_ = pp.BackBufferWidth;
      height_ = pp.BackBufferHeight;

      device_->SetRenderState(D3DRS_LIGHTING, FALSE);

      device_->SetRenderState(D3DRS_ALPHAREF, 0);
      device_->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_NOTEQUAL);
      this->SetRenderState(RENDER_CULLMODE, CULL_CCW);

      last_vb_ = NULL;
      last_ib_ = NULL;

      device_->GetRenderTarget(0, &default_surface_);

      return true;
    }

    bool GraphicsD3D::Initialize(int width, int height)
    {
      d3d_ = Direct3DCreate9(D3D_SDK_VERSION);
      if (!d3d_)
        return false;
      D3DPRESENT_PARAMETERS pp;
      ZeroMemory(&pp, sizeof(pp));
      pp.EnableAutoDepthStencil = TRUE;
      pp.AutoDepthStencilFormat = D3DFMT_D24S8;
      //if(ip->full_screen)
      //{
      //	D3DDISPLAYMODE dm;
      //	d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT,&dm);
      //	pp.Windowed=FALSE;
      //	if(ip->backbuffer_height||ip->backbuffer_width)
      //	{
      //		pp.BackBufferHeight=ip->backbuffer_height;
      //		pp.BackBufferWidth=ip->backbuffer_width;
      //	}
      //	else
      //	{
      //		pp.BackBufferWidth=dm.Width;;
      //		pp.BackBufferHeight=dm.Height;
      //	}
      //	pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
      //	pp.BackBufferFormat=dm.Format;
      //	pp.PresentationInterval=D3DPRESENT_INTERVAL_DEFAULT;
      //	pp.FullScreen_RefreshRateInHz=dm.RefreshRate;
      //}
      //else
      {
        pp.BackBufferWidth = width;
        pp.BackBufferHeight = height;
        pp.Windowed = TRUE;
        pp.hDeviceWindow = GetDesktopWindow();
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.BackBufferFormat = D3DFMT_UNKNOWN;
        pp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;//D3DPRESENT_INTERVAL_IMMEDIATE;
        //pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
        //pp.MultiSampleType=D3DMULTISAMPLE_8_SAMPLES;
        //pp.MultiSampleQuality=0;
      }
      if (FAILED(d3d_->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetDesktopWindow(),
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &device_)))
        return false;

      //dev->GetDepthStencilSurface(&back_buffer.depth);
      //      dev->GetRenderTarget(0,&back_buffer.tex);

      width_ = pp.BackBufferWidth;
      height_ = pp.BackBufferHeight;

      device_->SetRenderState(D3DRS_LIGHTING, FALSE);

      device_->SetRenderState(D3DRS_ALPHAREF, 0);
      device_->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_NOTEQUAL);

      device_->GetRenderTarget(0, &default_surface_);

      last_vb_ = NULL;
      last_ib_ = NULL;

      return true;
    }
    bool GraphicsD3D::Release()
    {
      return true;
    }

    int GraphicsD3D::Width()const
    {
      return width_;
    }

    int GraphicsD3D::Height()const
    {
      return height_;
    }

    //
    bool GraphicsD3D::clear()
    {
      uint32_t flag = D3DCLEAR_ZBUFFER;
      if (bg_color_.a)
        flag |= D3DCLEAR_TARGET;
      return D3D_OK == device_->Clear(0, NULL, flag, bg_color_.ARGB8(), 1.0f, 0x00);
    }
    bool GraphicsD3D::Flip(Viewport* vp)
    {
      return D3D_OK == device_->Present(0, 0, (HWND)vp->GetWindowHandle(), 0);
    }
    bool GraphicsD3D::BeginScene()
    {
      return D3D_OK == device_->BeginScene();
    }
    bool GraphicsD3D::EndScene()
    {
      return D3D_OK == device_->EndScene();
    }
    //env
    bool GraphicsD3D::SetBgColor(Color color)
    {
      bg_color_ = color;
      return true;
    }

    bool GraphicsD3D::GetRenderTarget(Texture** tex)
    {
      return true;
    }
    bool GraphicsD3D::SetRenderTarget(Texture* tex)
    {
      if (tex) {
        RenderTargetTextureD3D* tc = static_cast<RenderTargetTextureD3D*>(tex);

        IDirect3DSurface9* color;
        tc->tex->GetSurfaceLevel(0, &color);
        device_->SetRenderTarget(0, color);
        this->render_target_ = tc;
      }
      else {
        device_->SetRenderTarget(0, default_surface_);
      }
      return true;
    }

    static DWORD RenderFuncToD3DFunc(unsigned int value)
    {
      switch (value)
      {
      case COMPARE_ALWAYS:
        return D3DCMP_ALWAYS;
      case COMPARE_EQUAL:
        return D3DCMP_EQUAL;
      case COMPARE_GREATER:
        return D3DCMP_GREATER;
      case COMPARE_GREATEREQUAL:
        return D3DCMP_GREATEREQUAL;
      case COMPARE_LESS:
        return D3DCMP_LESS;
      case COMPARE_LESSEQUAL:
        return D3DCMP_LESSEQUAL;
      case COMPARE_NEVER:
        return D3DCMP_NEVER;
      case COMPARE_NOTEQUAL:
        return D3DCMP_NOTEQUAL;
      default:
        return 0;
      }
    }

    bool GraphicsD3D::SetRenderState(RenderState state, unsigned int value)
    {
      render_state_[state] = value;
      switch (state)
      {
      case RENDER_SHADEMODE:
        device_->SetRenderState(D3DRS_SHADEMODE, value == SHADE_GOURAUD ? D3DSHADE_GOURAUD : D3DSHADE_FLAT);
        break;
      case RENDER_CULLMODE:
      {
        switch (value)
        {
        case CULL_CW:
          device_->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
          break;
        case CULL_CCW:
          device_->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
          break;
        case CULL_NONE:
          device_->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
          break;
        }
      }
      break;
      case RENDER_Z:
        device_->SetRenderState(D3DRS_ZENABLE, value);
        break;
      case RENDER_ZFUNC:
        device_->SetRenderState(D3DRS_ZFUNC, RenderFuncToD3DFunc(value));
        break;
      case RENDER_ZWRITE:
        device_->SetRenderState(D3DRS_ZWRITEENABLE, value);
        break;
      case RENDER_COLORWRITE:
        device_->SetRenderState(D3DRS_COLORWRITEENABLE, value ? 0xf : 0);
        break;
        //default:
      }

      return true;
    }
    bool GraphicsD3D::SetAlphaBlendMode(AlphaBlendMode mode)
    {
      if (alphablend_mode_ == mode)
        return true;
      if (mode == ALPHABLEND_MODULATE)
      {
        device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        device_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        device_->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
      }
      else if (mode == ALPHABLEND_ADD)
      {
        device_->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        device_->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
        //device_->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
        device_->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        device_->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
      }
      else if (mode == ALPHABLEND_NONE)
      {
        device_->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        device_->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
      }
      alphablend_mode_ = mode;
      return true;
    }

    bool GraphicsD3D::SetTexture(Texture* tex, int stage)
    {
      if (!tex)
        return D3D_OK == device_->SetTexture(stage, 0);
      TextureD3D* tc = (TextureD3D*)tex;
      return D3D_OK == device_->SetTexture(stage, tc->tex);
    }

    //status
    bool GraphicsD3D::GetRenderTargetData(Texture* tex)
    {
      TextureD3D* tc = (TextureD3D*)tex;
      LPDIRECT3DSURFACE9 from, to;
      render_target_->tex->GetSurfaceLevel(0, &from);
      tc->tex->GetSurfaceLevel(0, &to);
      device_->GetRenderTargetData(from, to);
      from->Release();
      to->Release();
      return true;
    }

    bool GraphicsD3D::SetShader(shader::Shader* p, int slot)
    {
      this->shader_slot_[slot] = p;
      return true;
    }

    //create
    bool GraphicsD3D::CreateRenderTarget(int width, int height, TextureFormat color, TextureFormat depth, Texture** tex)
    {
      *tex = 0;
      RenderTargetTextureD3D* tc = new RenderTargetTextureD3D;
      if (D3D_OK != device_->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, EqualizeType(color), D3DPOOL_DEFAULT, &(tc->tex), 0))
        return false;
      if (depth == TEXTURE_NONE)
      {
        tc->depth = 0;
        *tex = tc;
        return true;
      }
      if (D3D_OK != device_->CreateTexture(width, height, 1, D3DUSAGE_DEPTHSTENCIL, EqualizeType(depth), D3DPOOL_DEFAULT, &(tc->depth), 0))
      {
        tc->tex->Release();
        return false;
      }
      *tex = tc;
      return true;
    }
    bool GraphicsD3D::CreateVertexBuffer(VertexBuffer** vb, int format, int vertices)
    {
      VertexBufferD3D* vbb;
      vbb = new VertexBufferD3D(format, vertices);
      if (D3D_OK == device_->CreateVertexBuffer(GetFertexSize(format) * vertices, 0, 0, D3DPOOL_MANAGED, &vbb->buffer_, 0))
      {
        *vb = vbb;
        return true;
      }
      else
      {
        return false;
      }
    }
    bool GraphicsD3D::CreateIndexBuffer(IndexBuffer** ib, int indices, PrimitiveType pt)
    {
      IndexBufferD3D* ibb;
      ibb = new IndexBufferD3D(indices, pt);
      if (D3D_OK == device_->CreateIndexBuffer(indices * sizeof(IndexType), 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ibb->buffer_, 0))
      {
        *ib = ibb;
        return true;
      }
      else
      {
        return false;
      }
    }
    bool GraphicsD3D::CreateTexture(Texture** tex, int width, int height, TextureFormat format, MemoryPool pool)
    {
      TextureD3D* tc = new TextureD3D;
      D3DFORMAT fmt = EqualizeType(format);
      if (D3DFMT_UNKNOWN == fmt)
        return false;
      if (D3D_OK != device_->CreateTexture(width, height, 1, 0, fmt, EqualizeType(pool), &(tc->tex), 0))
        return false;
      D3DSURFACE_DESC sd;
      tc->tex->GetLevelDesc(0, &sd);
      tc->desc.width = sd.Width;
      tc->desc.height = sd.Height;
      tc->desc.format = format;
      *tex = tc;
      return true;
    }

    bool GraphicsD3D::SetupShader(int format)
    {
      std::vector<char> t;
      CompiledShader* cs = 0;

      std::vector<unsigned char> config;
      _.push(config, (unsigned char*)&format, 4);

      for (int i = 0; i < MAX_SHADER_CHAIN; i++)
      {
        if (shader_slot_[i] == shader::ShaderEnd())
        {
          break;
        }
        if (shader_slot_[i] == shader::ShaderIgnore())
          continue;
        unsigned char type = shader_slot_[i]->GetClassID();
        config.push_back(type);
      }
      if (compiled_shader_.end() != compiled_shader_.find(config)) {
        cs = &compiled_shader_[config];
      }

      bool need_compile = cs == NULL;

      bool need_build = false;
      for (int i = 0; i < MAX_SHADER_CHAIN; i++)
      {
        if (shader_slot_[i] == shader::ShaderEnd())
          break;
        if (shader_slot_[i] == shader::ShaderIgnore())
          continue;
        if (hlsl::ShaderGenerator* shg = reinterpret_cast<hlsl::ShaderGenerator*>(shader_slot_[i]->GetShaderGenerator()))
        {
          if (false == shg->compiled_) {
            need_build = true;
            break;
          }
        }
      }


      if (need_compile || need_build)
      {
        cs = &compiled_shader_[config];

        int pipetype = 0;
        std::string vertex_constant;
        int vertex_constant_slots = 0;
        std::string vertex_program;
        std::string pixel_constant;
        int pixel_constant_slots = 0;
        int vertex_sampler_slots = 0;
        int pixel_sampler_slots = 0;
        std::string pixel_program;

        for (int i = 0; i < MAX_SHADER_CHAIN; i++)
        {
          if (shader_slot_[i] == shader::ShaderIgnore())
            continue;
          if (shader_slot_[i] == shader::ShaderEnd())
          {
            if (false == need_compile) {
              break;
            }
            /////////////////////////
            // building vertex decl.

            int e = 0;// using elements.
            int offset = 0; // of vertex format.
            D3DVERTEXELEMENT9 elem[50];// elements list.
            if (format & FTX_POSITION)
            {
              elem[e++] = VertexElement(0, offset, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0);
              offset += GetFertexSize(FTX_POSITION);
            }
            if (format & FTX_NORMAL)
            {
              elem[e++] = VertexElement(0, offset, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0);
              offset += GetFertexSize(FTX_NORMAL);
            }
            if (format & FTX_TANGENT)
            {
              elem[e++] = VertexElement(0, offset, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT, 0);
              offset += GetFertexSize(FTX_TANGENT);
            }
            BYTE colorindex = 0;
            if (format & FTX_DIFFUSE)
            {
              elem[e++] = VertexElement(0, offset, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, colorindex++);
              offset += GetFertexSize(FTX_DIFFUSE);
            }
            if (format & FTX_SPECULAR)
            {
              elem[e++] = VertexElement(0, offset, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, colorindex++);
              offset += GetFertexSize(FTX_SPECULAR);
            }
            if ((format & FTX_WEIGHTMASK) == FTX_WEIGHT1)
            {
              elem[e++] = VertexElement(0, offset, D3DDECLTYPE_FLOAT1, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0);
              offset += GetFertexSize(FTX_WEIGHT1);
            }
            else if ((format & FTX_WEIGHTMASK) == FTX_WEIGHT2)
            {
              elem[e++] = VertexElement(0, offset, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0);
              elem[e++] = VertexElement(0, offset + 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0);
              offset += GetFertexSize(FTX_WEIGHT2);
            }
            else if ((format & FTX_WEIGHTMASK) == FTX_WEIGHT3)
            {
              elem[e++] = VertexElement(0, offset, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0);
              elem[e++] = VertexElement(0, offset + 4, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0);
              offset += GetFertexSize(FTX_WEIGHT3);
            }
            else if ((format & FTX_WEIGHTMASK) == FTX_WEIGHT4)
            {
              elem[e++] = VertexElement(0, offset, D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0);
              elem[e++] = VertexElement(0, offset + 4, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT, 0);
              offset += GetFertexSize(FTX_WEIGHT4);
            }
            if ((format & FTX_TEXTUREMASK) == FTX_TEX1)
            {
              elem[e++] = VertexElement(0, offset, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0);
              offset += GetFertexSize(FTX_TEX1);
            }
            else if ((format & FTX_TEXTUREMASK) == FTX_TEX2)
            {
              elem[e++] = VertexElement(0, offset, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0);
              elem[e++] = VertexElement(0, offset + 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1);
              offset += GetFertexSize(FTX_TEX2);
            }
            else if ((format & FTX_TEXTUREMASK) == FTX_TEX3)
            {
              elem[e++] = VertexElement(0, offset, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0);
              elem[e++] = VertexElement(0, offset + 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1);
              elem[e++] = VertexElement(0, offset + 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2);
              offset += GetFertexSize(FTX_TEX3);
            }
            else if ((format & FTX_TEXTUREMASK) == FTX_TEX4)
            {
              elem[e++] = VertexElement(0, offset, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0);
              elem[e++] = VertexElement(0, offset + 8, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1);
              elem[e++] = VertexElement(0, offset + 16, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2);
              elem[e++] = VertexElement(0, offset + 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3);
              offset += GetFertexSize(FTX_TEX4);
            }
            elem[e++] = D3DDECL_END();

            device_->CreateVertexDeclaration(elem, &cs->vd);

            // end of vertex decl.

            if (format & FTX_DIFFUSE)
              pipetype |= FTX_DIFFUSE;

            ///////////////////////

            std::stringstream pipe;// "vs" to "ps" pipe format.
            std::string semantics_ps_in;// "vs" to "ps" pipe format.
            std::stringstream pipe_vs_program;
            std::stringstream pipe_ps_program;

            int color_slot = 0;

            pipe << "struct VS_OUTPUT{\n";
            pipe << "float4 position:POSITION;\n";
            pipe_vs_program << "output.position=position;\n";
            if (FTX_NORMAL & pipetype)
            {
              pipe << "float3 normal:NORMAL;\n";
              pipe_vs_program << "output.normal=normal;\n";
              pipe_ps_program << "float3 normal=input.normal;\n";
            }
            if (FTX_VIEW_DIRECTION & pipetype)
            {
              pipe << "float3 view_direction:COLOR" << color_slot++ << ";\n";
              pipe_vs_program << "output.view_direction=view_direction;\n";
              pipe_ps_program << "float3 view_direction=input.view_direction;\n";
            }
            if (FTX_LIGHT_DIRECTION & pipetype)
            {
              pipe << "float3 light_direction:COLOR" << color_slot++ << ";\n";
              pipe_vs_program << "output.light_direction=light_direction;\n";
              pipe_ps_program << "float3 light_direction=input.light_direction;\n";
            }
            //if(FTX_TANGENT&pipetype)
            //{
            //    pipe<<"float3 tangent:TANGENT;\n";
            //    pipe<<"float3 binormal:BINORMAL;\n";
            //    pipe_vs_program<<"output.tangent=tangent;\n";
            //    pipe_vs_program<<"output.binormal=binormal;\n";
            //    pipe_ps_program<<"float3 tangent=input.tangent;\n";
            //    pipe_ps_program<<"float3 binormal=input.binormal;\n";
            //}
            if (FTX_DIFFUSE & pipetype)
            {
              pipe << "float4 diffuse:COLOR" << color_slot++ << ";\n";
              pipe_vs_program << "output.diffuse=diffuse;\n";
              pipe_ps_program << "float4 diffuse=input.diffuse;\n";
            }
            else
            {
              pipe_ps_program << "float4 diffuse=float4(1,1,1,1);\n";
            }
            if (FTX_SPECULAR & pipetype)
            {
              pipe << "float3 specular:COLOR" << color_slot++ << ";\n";
              pipe_vs_program << "output.specular=specular;\n";
              pipe_ps_program << "float3 specular=input.specular;\n";
            }
            else
            {
              if (FTX_VIEW_DIRECTION & pipetype)
              {
                pipe_ps_program << "float3 specular=float3(0,0,0);\n";
              }
            }
            for (int i = 0; i < ((FTX_TEXTUREMASK & pipetype) >> 8); i++)
            {
              pipe << "float2 uv" << i << ":TEXCOORD" << i << ";\n";
              pipe_vs_program << "output.uv" << i << "=uv" << i << ";\n";
              pipe_ps_program << "float2 uv" << i << "=input.uv" << i << ";\n";
            }

            semantics_ps_in = pipe.str();

            if (FTX_VIEWPORT_POSITION & pipetype) {
              _(semantics_ps_in) << "float2 vpos:VPOS;\n";
              pipe_ps_program << "float2 vpos=input.vpos;\n";
            }
            _(semantics_ps_in) << "};\n";
            pipe << "};\n";

            /////////////////////////
            // building vertex shader
            std::stringstream vs;
            vs << vertex_constant;
            //vs<<"float4 register_slot[128]:register(c0);\n";;
            vs << pipe.str();
            vs << "VS_OUTPUT Main(\n";
            vs << "in float4 position:POSITION\n";
            if (FTX_NORMAL & format)
              vs << ",in float3 normal:NORMAL\n";
            if (FTX_TANGENT & format)
              vs << ",in float3 tangent:TANGENT\n";
            if (FTX_DIFFUSE & format)
              vs << ",in float4 diffuse:COLOR0\n";
            if (FTX_SPECULAR & format)
              vs << ",in float3 specular:COLOR1\n";
            if (FTX_WEIGHT1 == (FTX_WEIGHTMASK & format))
              vs << ",in float blendindex:BLENDINDICES\n";
            if (FTX_WEIGHT2 == (FTX_WEIGHTMASK & format))
            {
              vs << ",in float2 blendindex:BLENDINDICES\n";
              vs << ",in float2 blendweight:BLENDWEIGHT\n";
            }
            for (int i = 0; i < ((FTX_TEXTUREMASK & format) >> 8); i++)
              vs << ",in float2 uv0:TEXCOORD" << i++ << "\n";
            vs << "){\n";

            // main.
            vs << "VS_OUTPUT output;\n";
            if ((FTX_DIFFUSE & pipetype) && !(FTX_DIFFUSE & format))
              vs << "float4 diffuse=float4(1,1,1,1);\n";
            if ((FTX_SPECULAR & pipetype) && !(FTX_SPECULAR & format))
              vs << "float3 specular=float3(0,0,0);\n";
            vs << vertex_program;
            vs << pipe_vs_program.str();
            vs << "return output;\n";
            vs << "}";
            // compile
            LPD3DXBUFFER buf, msg;
            if (D3D_OK != D3DXCompileShader(vs.str().data(), (UINT)vs.str().size(), NULL, NULL, "Main", D3DXGetVertexShaderProfile(device_),
              /*D3DXSHADER_DEBUG*/0, &buf, &msg, NULL))
            {
              const char* error = (const char*)msg->GetBufferPointer();
              std::string prog = vs.str();
              prog += "// ";
              prog += error;
              //WriteFile("vs_error.txt", prog.data());
              DebugBreak();
            }
            else
            {
#ifdef _DEBUG
              remove("vs_error.txt");
              LPD3DXBUFFER dasm;
              D3DXDisassembleShader((DWORD*)(buf->GetBufferPointer()), false, 0, &dasm);

              std::string fn;
              fn += "vs_";
              fn += _.toHex(config);
              fn += ".txt";
              std::vector<char> data;
              _.push(data, vs.str().data(), vs.str().size() - 1);
              _.push(data, (char*)dasm->GetBufferPointer(), dasm->GetBufferSize() - 1);
              //WriteFile(fn.data(), data);
#endif
            }
            device_->CreateVertexShader((DWORD*)(buf->GetBufferPointer()), &cs->vs);
            buf->Release();

            ///////////////////////////
            // building pixel shader
            std::stringstream ps;
            ps << pixel_constant;
            ps << semantics_ps_in;
            ps << "float4 Main(VS_OUTPUT input):COLOR0\n";
            ps << "{\n";
            ps << pipe_ps_program.str();
            ps << "float4 color;\n";
            ps << "color=diffuse;\n";
            ps << pixel_program;
            if (FTX_SPECULAR & pipetype ||
              FTX_VIEW_DIRECTION & pipetype)
            {
              //ps<<"color.xyz=color.xyz+specular.xyz;\n";
            }
            ps << "return color;\n";
            ps << "}\n";

            if (D3D_OK != D3DXCompileShader(ps.str().data(), (UINT)ps.str().size(), NULL, NULL, "Main", D3DXGetPixelShaderProfile(device_),
              /*D3DXSHADER_DEBUG*/0, &buf, &msg, NULL))
            {
              std::string error = (char*)msg->GetBufferPointer();
              std::string prog = ps.str();
              prog += "// ";
              prog += error;
              //WriteFile("ps_error.txt", prog.data());
              DebugBreak();
            }
            else
            {
#ifdef _DEBUG
              remove("ps_error.txt");
              LPD3DXBUFFER dasm;
              D3DXDisassembleShader((DWORD*)(buf->GetBufferPointer()), false, 0, &dasm);

              std::string fn;
              fn += "ps_";
              fn += _.toHex(config);
              fn += ".txt";
              std::vector<char> data;
              _.push(data, ps.str().data(), ps.str().size() - 1);
              _.push(data, (char*)dasm->GetBufferPointer(), dasm->GetBufferSize() - 1);
              //WriteFile(fn.data(), data);
#endif
            }
            device_->CreatePixelShader((DWORD*)(buf->GetBufferPointer()), &cs->ps);
            buf->Release();
            break;
          }
          if (hlsl::ShaderGenerator* shg = reinterpret_cast<hlsl::ShaderGenerator*>(shader_slot_[i]->GetShaderGenerator()))
          {
            if (shg->compiled_) {
              //warning. double compiling
#ifdef _DEBUG
                        //printf("Warning! double compile type=%d\n",(int)(shader_slot_[i]->GetClassID()));
#endif
            }
            cs->vertex_register_offset[i] = vertex_constant_slots;
            cs->pixel_register_offset[i] = pixel_constant_slots;
            shg->Vertex(format, vertex_constant, vertex_constant_slots, vertex_sampler_slots, vertex_program);
            shg->Pixel(format, pixel_constant, pixel_constant_slots, pixel_sampler_slots, pixel_program);
            pipetype |= shg->PipeType(format);
            shg->compiled_ = true;
          }
        }
      }

      for (int i = 0; i < MAX_SHADER_CHAIN; i++)
      {
        if (shader_slot_[i] == shader::ShaderIgnore())
          continue;
        if (shader_slot_[i] == shader::ShaderEnd())
        {
          device_->SetVertexDeclaration(cs->vd);
          device_->SetVertexShader(cs->vs);
          device_->SetPixelShader(cs->ps);
          break;
        }
        if (hlsl::ShaderGenerator* shg = reinterpret_cast<hlsl::ShaderGenerator*>(shader_slot_[i]->GetShaderGenerator()))
        {
          hlsl::ShaderConstant sc = {
              this,
              cs->vertex_register_offset[i],
              cs->pixel_register_offset[i]
          };
          shg->SetupConstant(sc);
        }
      }
      return true;
    }
    //draw
    bool GraphicsD3D::DrawPrimitive(const VertexBuffer* vb, unsigned int primitivecount)
    {
      VertexBufferD3D* vbb = (VertexBufferD3D*)vb;
      if (D3D_OK != device_->SetStreamSource(0, vbb->buffer_, 0, GetFertexSize(vb->Format())))
        return false;

      if (!SetupShader(vb->Format()))
        return false;

      if (D3D_OK != device_->DrawPrimitive(D3DPT_TRIANGLELIST, 0, primitivecount ? primitivecount : vb->Vertices() / 3))
        return false;
      return true;
    }
    bool GraphicsD3D::DrawIndexedPrimitive(const VertexBuffer* vb, const IndexBuffer* ib, int offset, int triangles)
    {
      if (last_ib_ != ib)
      {
        IndexBufferD3D* ibb = (IndexBufferD3D*)ib;
        if (D3D_OK != device_->SetIndices(ibb->buffer_))
          return false;
        last_ib_ = ib;
      }

      if (last_vb_ != vb)
      {
        VertexBufferD3D* vbb = (VertexBufferD3D*)vb;
        if (D3D_OK != device_->SetStreamSource(0, vbb->buffer_, 0, GetFertexSize(vb->Format())))
          return false;
        last_vb_ = vb;
      }

      if (!SetupShader(vb->Format()))
        return false;

      if (D3D_OK != device_->DrawIndexedPrimitive(EqualizeType(ib->Type()), 0, 0, vb->Vertices(), offset, triangles))
        return false;
      return true;
    }
    bool GraphicsD3D::DrawPrimitiveUP(PrimitiveType type, unsigned int primitivecount, const void* vtx, unsigned int vertexformat)
    {
      last_vb_ = NULL;
      SetupShader(vertexformat);
      auto ret = device_->DrawPrimitiveUP(EqualizeType(type), primitivecount, vtx, GetFertexSize(vertexformat));
      return true;
    }
    bool GraphicsD3D::DrawIndexedPrimitiveUP(PrimitiveType type, unsigned int primitivecount, const unsigned short* idx, const void* vtx, unsigned int vertexcount, unsigned int vertexformat)
    {
      last_vb_ = NULL;
      last_ib_ = NULL;
      SetupShader(vertexformat);
      device_->DrawIndexedPrimitiveUP(EqualizeType(type), 0, vertexcount, primitivecount, idx, D3DFMT_INDEX16, vtx, GetFertexSize(vertexformat));
      return true;
    }

  }
}

#endif
