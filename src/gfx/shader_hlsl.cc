
#ifdef _WIN32

#include "shader_hlsl.h"
#include "graphics_d3d9.h"
#include "fertex.h"
#include "../main/underscore.h"
#include <assert.h>

namespace gh {

  static float* MatrixTranspose(const Matrix& mat)
  {
    static float matTemp[16];
    matTemp[0] = mat._11;
    matTemp[1] = mat._21;
    matTemp[2] = mat._31;
    matTemp[3] = mat._41;
    matTemp[4] = mat._12;
    matTemp[5] = mat._22;
    matTemp[6] = mat._32;
    matTemp[7] = mat._42;
    matTemp[8] = mat._13;
    matTemp[9] = mat._23;
    matTemp[10] = mat._33;
    matTemp[11] = mat._43;
    matTemp[12] = mat._14;
    matTemp[13] = mat._24;
    matTemp[14] = mat._34;
    matTemp[15] = mat._44;
    return matTemp;
  }

  shader::Shader* d3d9::GraphicsD3D::CreateShader(shader::ShaderType st)
  {
    using namespace shader;
    using namespace hlsl;
    switch (st)
    {
    case ClassID_WorldTransform:
      return new WorldTransformBody();
    case ClassID_BlendTransform:
      return new BlendTransformBody();
    case ClassID_HugeBlendTransform:
      return new HugeBlendTransformBody();
    case ClassID_Camera:
      return new CameraBody();
    case ClassID_DirectionalLight:
      return new DirectionalLightBody();
    case ClassID_AmbientLight:
      return new AmbientLightBody();
    case ClassID_SpecularLight:
      return new SpecularLightBody();
    case ClassID_DiffuseLight:
      return new DiffuseLightBody();
    case ClassID_Fractal:
      return new FractalBody();
    case ClassID_DecalTexture:
      return new DecalTextureBody();
    case ClassID_NormalMap:
      return new NormalMapBody();
    case ClassID_TangentMap:
      return new TangentMapBody();
    case ClassID_ColoredNormalMap:
      return new ColoredNormalMapBody();
    case ClassID_AlphaBlendTexture:
      return new AlphaBlendTextureImpl();
    case ClassID_RGBModAlpha:
      return new RGBModAlphaBody();
    case ClassID_SetRGBA:
      return new SetRGBABody();
    case ClassID_Particle:
      return new ParticleBody();
    case ClassID_Pointicle:
      return new PointicleBody();
    case ClassID_CenterSpecular:
      return new CenterSpecularBody();
    case ClassID_EdgeSpecular:
      return new EdgeSpecularBody();
    case ClassID_BlowNormal:
      return new BlowNormalBody();
    case ClassID_SetAngleRGBA:
      return new SetAngleRGBABody();
    case ClassID_LoopJet:
      return new LoopJetHLSL();
    case ClassID_Billboard:
      return new BillboardHLSL();
    case ClassID_ScreenSpaceDiffuse:
      return new ScreenSpaceDiffuseBody();
    case ClassID_GaussianFilter:
      return new GaussianFilterBody();
    }
    assert(false);
    return NULL;
  }

  bool d3d9::GraphicsD3D::DeleteShader(shader::Shader* p)
  {
    delete p;
    return true;
  }

  namespace hlsl {
    ////////////////////////////////////////////////
    // WorldTransform

    void WorldTransformBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      _(constant) << "float4x4 mat_world:register(c" << used_constant << ");\n";
      used_constant += 4;
      _(main) << "position=mul(position,mat_world);\n";
      if (format & FTX_NORMAL)
      {
        _(main) << "normal=mul(normal,mat_world);\n";
        _(main) << "normal=normalize(normal);\n";
      }
      if (format & FTX_TANGENT)
      {
        _(main) << "tangent=mul(tangent,mat_world);\n";
        _(main) << "tangent=normalize(tangent);\n";
      }
    }

    int WorldTransformBody::PipeType(int format)
    {
      return FTX_POSITION;
    }

    void WorldTransformBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
    }

    void WorldTransformBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      dev->SetVertexShaderConstantF(sc.vertex_register_offset, transform_.m, 4);
    }

    ////////////////////////////////////////////////
    // BlendTransform

    BlendTransformBody::BlendTransformBody()
      :fixed_(0)
      , max_using_(0)
    {
    }

    void BlendTransformBody::Transform(const Matrix& mat, int slot)
    {
      if (slot > max_matrix_buffer)
      {
        //fatal error
        //assert(false);
      }
      else
      {
        memcpy(transform_ + slot * 16, MatrixTranspose(mat), sizeof(float) * 16);
        if (slot >= max_using_)
          max_using_ = slot + 1;
        fixed_ = 0;
      }
    }

    void BlendTransformBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      _(constant) << "float4x4 mat_blend[" << max_matrix_buffer << "]:register(c" << used_constant << ");\n";
      used_constant += 4 * max_matrix_buffer;
      if (FTX_WEIGHT1 == (format & FTX_WEIGHTMASK))
      {
        _(main) << "position=mul(position,mat_blend[blendindex]);\n";
        if (format & FTX_NORMAL)
        {
          _(main) << "normal=mul(normal,mat_blend[blendindex]);\n"
            "normal=normalize(normal);\n";
        }
      }
      else if (FTX_WEIGHT2 == (format & FTX_WEIGHTMASK))
      {
        _(main) << "float4 blendposition0=mul(position,mat_blend[blendindex[0]])*blendweight[0];\n";
        _(main) << "float4 blendposition1=mul(position,mat_blend[blendindex[1]])*blendweight[1];\n";
        _(main) << "position=blendposition0+blendposition1;\n";
        _(main) << "float3 blendnormal0=mul(normal,mat_blend[blendindex[0]])*blendweight[0];\n";
        _(main) << "float3 blendnormal1=mul(normal,mat_blend[blendindex[1]])*blendweight[1];\n";
        _(main) << "normal=blendnormal0+blendnormal1;\n";
        _(main) << "normal=normalize(normal);\n";
      }
      //main<<"position=mul(position,mat_Blend);\n";
      //if(format&FTX_NORMAL)
      //{
      //    main<<"normal=mul(normal,mat_Blend);\n";
      //    main<<"normal=normalize(normal);\n";
      //}
    }

    int BlendTransformBody::PipeType(int format)
    {
      return FTX_POSITION;
    }

    void BlendTransformBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
    }

    void BlendTransformBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      if (0 == fixed_)
        fixed_ = max_using_;
      dev->SetVertexShaderConstantF(sc.vertex_register_offset, transform_, 4 * fixed_);
    }

    ////////////////////////////////////////////////
    // HugeBlendTransform

    HugeBlendTransformBody::HugeBlendTransformBody()
      :fixed_(0)
      , max_using_(0)
      , blends_(0)
    {
    }

    void HugeBlendTransformBody::Transform(const Matrix& mat, int slot)
    {
      if (slot > max_matrix_buffer)
      {
        //fatal error
      }
      else
      {
        memcpy(transform_ + slot * 16, MatrixTranspose(mat), sizeof(float) * 16);
        if (slot + 1 > max_using_)
          max_using_ = slot + 1;
        fixed_ = 0;
      }
    }

    void HugeBlendTransformBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      sampler_slot_ = used_sampler++;
      _(constant) << "texture tex_blend_weight;\n"
        "sampler samp_blend_weight:register(s" << sampler_slot_ << ")=sampler_state\n"
        "{\n"
        "texture = <tex_blend_weight>;\n"
        "mipfilter=none;\n"
        "magfilter=point;\n"
        "minfilter=point;\n"
        "};\n"
        ;
      if (FTX_WEIGHT1 == (format & FTX_WEIGHTMASK))
      {
        _(main) << "\n"
          "float blend_wtexel=" << 1.0f / buffer_width << ";\n"
          "float blend_htexel=" << 1.0f / max_matrix_buffer << ";\n"
          "float blend_whalftexel=blend_wtexel/2;\n"
          "float blend_hhalftexel=blend_htexel/2;\n"
          "float blend_row0=blend_htexel*blendindex+blend_hhalftexel;\n"
          "float4 blend_row00=tex2Dlod(samp_blend_weight,float4(blend_whalftexel,blend_row0,0,0));\n"
          "float4 blend_row01=tex2Dlod(samp_blend_weight,float4(blend_wtexel+blend_whalftexel,blend_row0,0,0));\n"
          "float4 blend_row02=tex2Dlod(samp_blend_weight,float4(blend_wtexel*2+blend_whalftexel,blend_row0,0,0));\n"
          "float3 blendposition0;\n"
          "blendposition0.x=dot(blend_row00.xyzw,position);\n"
          "blendposition0.y=dot(blend_row01.xyzw,position);\n"
          "blendposition0.z=dot(blend_row02.xyzw,position);\n"
          "position.xyz=blendposition0;\n"
          ;
        if (format & FTX_NORMAL)
        {
          _(main) << "float3 blendnormal0;\n"
            "blendnormal0.x=dot(blend_row00.xyz,normal);\n"
            "blendnormal0.y=dot(blend_row01.xyz,normal);\n"
            "blendnormal0.z=dot(blend_row02.xyz,normal);\n"
            "normal.xyz=blendnormal0;\n"
            "normal=normalize(normal);\n"
            ;
        }
        if (format & FTX_TANGENT)
        {
          _(main) << "float3 blendtangent0;\n"
            "blendtangent0.x=dot(blend_row00.xyz,tangent);\n"
            "blendtangent0.y=dot(blend_row01.xyz,tangent);\n"
            "blendtangent0.z=dot(blend_row02.xyz,tangent);\n"
            "tangent.xyz=blendtangent0;\n"
            "tangent=normalize(tangent);\n"
            ;
        }
      }
      else if (FTX_WEIGHT2 == (format & FTX_WEIGHTMASK))
      {
        _(main) << "\n"
          "float blend_wtexel=" << 1.0f / buffer_width << ";\n"
          "float blend_htexel=" << 1.0f / max_matrix_buffer << ";\n"
          "float blend_whalftexel=blend_wtexel/2;\n"
          "float blend_hhalftexel=blend_htexel/2;\n"
          "float blend_row0=blend_htexel*blendindex[0]+blend_hhalftexel;\n"
          "float blend_row1=blend_htexel*blendindex[1]+blend_hhalftexel;\n"
#if 1
          "float4 blend_row00=tex2Dlod(samp_blend_weight,float4(blend_whalftexel,blend_row0,0,0));\n"
          "float4 blend_row01=tex2Dlod(samp_blend_weight,float4(blend_wtexel+blend_whalftexel,blend_row0,0,0));\n"
          "float4 blend_row02=tex2Dlod(samp_blend_weight,float4(blend_wtexel*2+blend_whalftexel,blend_row0,0,0));\n"
          "float4 blend_row10=tex2Dlod(samp_blend_weight,float4(blend_whalftexel,blend_row1,0,0));\n"
          "float4 blend_row11=tex2Dlod(samp_blend_weight,float4(blend_wtexel+blend_whalftexel,blend_row1,0,0));\n"
          "float4 blend_row12=tex2Dlod(samp_blend_weight,float4(blend_wtexel*2+blend_whalftexel,blend_row1,0,0));\n"
#else
          "float4 blend_row00=tex2D(samp_blend_weight,float2(blend_whalftexel,blend_row0));\n"
          "float4 blend_row01=tex2D(samp_blend_weight,float2(blend_wtexel+blend_whalftexel,blend_row0));\n"
          "float4 blend_row02=tex2D(samp_blend_weight,float2(blend_wtexel*2+blend_whalftexel,blend_row0));\n"
          "float4 blend_row10=tex2D(samp_blend_weight,float2(blend_whalftexel,blend_row1));\n"
          "float4 blend_row11=tex2D(samp_blend_weight,float2(blend_wtexel+blend_whalftexel,blend_row1));\n"
          "float4 blend_row12=tex2D(samp_blend_weight,float2(blend_wtexel*2+blend_whalftexel,blend_row1));\n"
#endif
          "float3 blendposition0,blendposition1;\n"
          "blendposition0.x=dot(blend_row00.xyzw,position)*blendweight[0];\n"
          "blendposition0.y=dot(blend_row01.xyzw,position)*blendweight[0];\n"
          "blendposition0.z=dot(blend_row02.xyzw,position)*blendweight[0];\n"
          "blendposition1.x=dot(blend_row10.xyzw,position)*blendweight[1];\n"
          "blendposition1.y=dot(blend_row11.xyzw,position)*blendweight[1];\n"
          "blendposition1.z=dot(blend_row12.xyzw,position)*blendweight[1];\n"
          "position.xyz=blendposition0+blendposition1;\n"
          ;
        if (format & FTX_NORMAL)
        {
          _(main) << "float3 blendnormal0,blendnormal1;\n"
            "blendnormal0.x=dot(blend_row00.xyz,normal)*blendweight[0];\n"
            "blendnormal0.y=dot(blend_row01.xyz,normal)*blendweight[0];\n"
            "blendnormal0.z=dot(blend_row02.xyz,normal)*blendweight[0];\n"
            "blendnormal1.x=dot(blend_row10.xyz,normal)*blendweight[1];\n"
            "blendnormal1.y=dot(blend_row11.xyz,normal)*blendweight[1];\n"
            "blendnormal1.z=dot(blend_row12.xyz,normal)*blendweight[1];\n"
            "normal.xyz=blendnormal0+blendnormal1;\n"
            "normal=normalize(normal);\n"
            ;
        }
      }
      //main<<"position=mul(position,mat_Blend);\n";
      //if(format&FTX_NORMAL)
      //{
      //    main<<"normal=mul(normal,mat_Blend);\n";
      //    main<<"normal=normalize(normal);\n";
      //}
    }

    int HugeBlendTransformBody::PipeType(int format)
    {
      return FTX_POSITION;
    }

    void HugeBlendTransformBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
    }

    void HugeBlendTransformBody::SetupConstant(ShaderConstant& sc)
    {
      if (0 == fixed_)
        fixed_ = max_using_;
      if (!blends_)
      {
        sc.dd->CreateTexture(&blends_, buffer_width, max_matrix_buffer, TEXTURE_ABGR32F);
        LockInfo li;
        blends_->Lock(li, LOCK_WRITE);
        for (int y = 0; y < max_matrix_buffer; y++)
        {
          ((Matrix*)(li.Bits + li.Pitch * y))->Zero();
        }
        blends_->Unlock();
      }
      if (1 <= max_using_) {
        LockInfo li;
        blends_->LockRect(li, LOCK_WRITE, 0, 0, buffer_width, max_using_);
        for (int y = 0; y < max_using_; y++)
        {
          memcpy((li.Bits + li.Pitch * y), transform_ + y * 16, sizeof(float) * 16);
        }
        blends_->Unlock();
        sc.dd->SetTexture(blends_, D3DVERTEXTEXTURESAMPLER0 + sampler_slot_);
        LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
        dev->SetSamplerState(D3DVERTEXTEXTURESAMPLER0 + sampler_slot_, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        dev->SetSamplerState(D3DVERTEXTEXTURESAMPLER0 + sampler_slot_, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      }
    }

    //////////////////////////////////////////////////////
    // Camera

    CameraBody::CameraBody()
    {
      bias_.x = 0;
      bias_.y = 0;
      bias_.z = 0;
    }

    void CameraBody::ScreenBias(float x, float y, float z)
    {
      bias_.x = x;
      bias_.y = y;
      bias_.z = z;
    }

    void CameraBody::PerspectiveFov(float nearclip, float farclip, float fov, float aspect)
    {
      //D3DXMatrixPerspectiveFovLH((D3DXMATRIX*)&projection_,fov,aspect,nearclip,farclip);
      projection_.PerspectiveFovLH(nearclip, farclip, fov, aspect);
      projection_._41 += bias_.x;
      projection_._42 += bias_.y;
      projection_._43 += bias_.z;
      view_projection_ = view_ * projection_;
    }

    void CameraBody::Ortho(float nearclip, float farclip, float w, float h)
    {
      projection_.OrthoLH(nearclip, farclip, w, h);
      projection_._41 += bias_.x;
      projection_._42 += bias_.y;
      projection_._43 += bias_.z;
      view_projection_ = view_ * projection_;
    }

    void CameraBody::LookAt(const Vector3& from, const Vector3& at, const Vector3& up)
    {
      this->from_ = from;
      this->dir_ = from - at;
      this->dir_.Normalize();
      //D3DXMatrixLookAtLH((D3DXMATRIX*)&view_,(D3DXVECTOR3*)&from,(D3DXVECTOR3*)&at,(D3DXVECTOR3*)&up);
      view_.LookAtLH(from, at, up);
      view_projection_ = view_ * projection_;
    }

    void CameraBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      _(constant) << "float4x4 mat_view_projection:register(c" << (used_constant + 0) << ");\n";
      _(constant) << "float3 camera_position:register(c" << (used_constant + 4) << ");\n";
      _(constant) << "float3 camera_direction:register(c" << (used_constant + 5) << ");\n";
      used_constant += 6;

      _(main) << "position=mul(position,mat_view_projection);\n";

    }

    int CameraBody::PipeType(int format)
    {
      return FTX_POSITION;
    }

    void CameraBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
    }

    void CameraBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 0, MatrixTranspose(view_projection_), 4);
      float from[4];
      from[0] = from_.x;
      from[1] = from_.y;
      from[2] = from_.z;
      from[3] = 0;
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 4, from, 1);
      from[0] = dir_.x;
      from[1] = dir_.y;
      from[2] = dir_.z;
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 5, from, 1);
    }

    ///////////////////////////////////////////////////
    // DirectionalLight

    void DirectionalLightBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      if (format & FTX_NORMAL)
      {
        _(constant) << "float3 directionallight_direction:register(c" << (used_constant + 0) << ");\n";
        _(constant) << "float4 directionallight_color:register(c" << (used_constant + 1) << ");\n";
        used_constant += 2;

        //_(main) << "diffuse.xyz=diffuse.xyz*directionallight_color.xyz*dot(normal,directionallight_direction);\n";
      }
    }

    int DirectionalLightBody::PipeType(int format)
    {
      return FTX_DIFFUSE;
    }

    void DirectionalLightBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
    }

    void DirectionalLightBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 0, direction_.v, 1);
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 1, color_.ToArray(), 1);
    }

    //////////////////////////////////////
    // AmbientLight

    void AmbientLightBody::Ambient(const Color& color)
    {
      this->color_ = color;
    }

    void AmbientLightBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      _(constant) << "float4 ambientlight_color:register(c" << used_constant << ");\n";
      used_constant += 1;

      _(main) << "diffuse.xyz=max(ambientlight_color.xyz,diffuse.xyz);\n";
    }

    int AmbientLightBody::PipeType(int format)
    {
      return FTX_DIFFUSE;
    }

    void AmbientLightBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
    }

    void AmbientLightBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      dev->SetVertexShaderConstantF(sc.vertex_register_offset, color_.ToArray(), 1);
    }

    //////////////////////////////////////
    // SpecularLight

    void SpecularLightBody::Specular(const Color& color)
    {
      this->color_ = color;
    }

    void SpecularLightBody::Power(float power)
    {
      this->power_ = power;
    }

    void SpecularLightBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      _(constant) << "float4 specularlight_color:register(c" << used_constant << ");\n";
      used_constant += 1;

      _(main) << "specular=specular+specularlight_color.xyz*pow(max(0,dot(normal,normalize(normalize(camera_position.xyz-position.xyz)+directionallight_direction))),specularlight_color.w);\n"
        ;
      //main<<"specular.b=specular.b+max(0,-dot(normalize(position_world.xyz),normal.xyz));\n";

    }

    int SpecularLightBody::PipeType(int format)
    {
      return FTX_DIFFUSE | FTX_SPECULAR;
    }

    void SpecularLightBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      _(main) << "color.xyz=color.xyz+specular.xyz;\n"
        "color.xyz=min(color.xyz,float3(1,1,1));\n"
        ;
    }

    void SpecularLightBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      color_.a = power_;
      dev->SetVertexShaderConstantF(sc.vertex_register_offset, color_.ToArray(), 1);
    }

    //////////////////////////////////////
    // DiffuseLight

    void DiffuseLightBody::Diffuse(const Color& color)
    {
      this->diffuse_ = color;
    }

    void DiffuseLightBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      _(constant) << "float4 DiffuseLight_diffuse:register(c" << (used_constant + 0) << ");\n";
      used_constant += 1;

      _(main) << "diffuse.xyz=diffuse.xyz*directionallight_color.xyz*dot(normal,directionallight_direction);\n";
      //_(main) << "diffuse.xyz=DiffuseLight_diffuse.xyz*diffuse.xyz;\n";
      //_(main) << "diffuse.xyz=max(DiffuseLight_ambient,diffuse.xyz);\n";
    }

    int DiffuseLightBody::PipeType(int format)
    {
      
      return FTX_DIFFUSE;
    }

    void DiffuseLightBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      
    }

    void DiffuseLightBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      dev->SetVertexShaderConstantF(sc.vertex_register_offset, (float*)&diffuse_, 1);
    }

    //////////////////////////////////////
    // Fractal

    FractalBody::FractalBody()
    {
    }

    void FractalBody::ColorTable(Texture* t)
    {
      table_ = t;
    }

    void FractalBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      
      //vertex_slot_color=used_constant;
      //constant<<"float4 Fractal_color:register(c"<<used_constant<<");\n";
      //used_constant+=1;

      //main<<"diffuse=max(Fractal_color,diffuse);\n";
    }

    int FractalBody::PipeType(int format)
    {
      
      return FTX_NORMAL;
    }

    void FractalBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      

      sampler_ = used_sampler++;

      _(constant) << "texture tex" << sampler_ << ";\n";
      _(constant) << "sampler samp" << sampler_ << ":register(s" << sampler_ << ")=sampler_state\n";
      _(constant) << "{\n";
      _(constant) << "Texture = <tex" << sampler_ << ">;\n";
      _(constant) << "};\n";

      _(main) << "float loop,x=0,y=0;\n"
        "float step=1.0f/256.0f;\n"
        "float2 uv;\n"
        "uv.x=1.0f;\n"
        "uv.y=normal.z;\n"
        "for(loop=0;loop<1;loop+=step)\n{\n"
        "float x2=x*x;\n"
        "float y2=y*y;\n"
        "float zx=x2-y2+normal.x;\n"
        "float zy=2*x*y+normal.y;\n"
        "x=zx;\n"
        "y=zy;\n"
        "if(x2+y2>=4){\n"
        "uv.x=loop;\n"
        "break;}\n"
        "};\n"
        "color=tex2D(samp" << sampler_ << ",uv);\n";
    }

    void FractalBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      sc.dd->SetTexture(table_, sampler_);
      dev->SetSamplerState(sampler_, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      dev->SetSamplerState(sampler_, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    }

    //////////////////////////////////////
    // DecalTexture

    DecalTextureBody::DecalTextureBody()
      :uv_(0)
    {
    }

    void DecalTextureBody::SetTexture(Texture* t)
    {
      decal_ = t;
    }

    void DecalTextureBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      
    }

    int DecalTextureBody::PipeType(int format)
    {
      
      return FTX_TEX1;
    }

    void DecalTextureBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      

      sampler_ = used_sampler++;

      _(constant) << "texture tex" << sampler_ << ";\n";
      _(constant) << "sampler samp" << sampler_ << ":register(s" << sampler_ << ")=sampler_state\n";
      _(constant) << "{\n";
      _(constant) << "Texture = <tex" << sampler_ << ">;\n";
      _(constant) << "};\n";

      _(main) << "color=color*tex2D(samp" << sampler_ << ",uv" << uv_ << ");\n";
    }

    void DecalTextureBody::SetupConstant(ShaderConstant& sc)
    {
      sc.dd->SetTexture(decal_, sampler_);
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      //dev->SetSamplerState(sampler_, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      //dev->SetSamplerState(sampler_, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    }

    //////////////////////////////////////
    // NormalMap

    NormalMapBody::NormalMapBody()
      :uv_(0)
    {
    }

    void NormalMapBody::SetTexture(Texture* t)
    {
      decal_ = t;
    }

    void NormalMapBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      _(main) <<
        "float3 n = normalize(normal);\n"
        "float3 t = normalize(tangent);\n"
        "float3 b = normalize(cross(n, t));\n"

        "float3 cam_dir=camera_position.xyz-position.xyz;\n"
        "float3 view_direction;\n"
        "view_direction.x = dot(t, cam_dir);\n"
        "view_direction.y = dot(b, cam_dir);\n"
        "view_direction.z = dot(n, cam_dir);\n"
        "view_direction = normalize(view_direction);\n"

        "float3 light_direction;\n"
        "light_direction.x = dot(t, directionallight_direction);\n"
        "light_direction.y = dot(b, directionallight_direction);\n"
        "light_direction.z = dot(n, directionallight_direction);\n"
        "light_direction = normalize(light_direction);\n"
        ;
    }

    int NormalMapBody::PipeType(int format)
    {
      
      return FTX_TEX1 | FTX_VIEW_DIRECTION | FTX_LIGHT_DIRECTION;
    }

    void NormalMapBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      

      sampler_ = used_sampler++;

      _(constant) << "texture tex" << sampler_ << ";\n";
      _(constant) << "sampler samp" << sampler_ << ":register(s" << sampler_ << ")=sampler_state\n";
      _(constant) << "{\n";
      _(constant) << "Texture = <tex" << sampler_ << ">;\n";
      _(constant) << "};\n";

      _(main) <<
        "float3 normal_map = (tex2D(samp" << sampler_ << ",uv" << uv_ << ")).xyz * 2.0 - 1.0;\n"
        "float3 half = normalize(light_direction + view_direction);\n"
        "float normapmap_specular = pow(max(dot(normal_map, half), 0.0), 10.0);\n"
        "color.x+=normapmap_specular;\n"
        "color.y+=normapmap_specular;\n"
        "color.z+=normapmap_specular;\n"
        ;
    }

    void NormalMapBody::SetupConstant(ShaderConstant& sc)
    {
      sc.dd->SetTexture(decal_, sampler_);
    }

    //////////////////////////////////////
    // TangentMap

    TangentMapBody::TangentMapBody()
      :uv_(0)
    {
    }

    void TangentMapBody::SetTexture(Texture* t)
    {
      decal_ = t;
    }

    void TangentMapBody::Specular(const Color& color)
    {
      this->color_ = color;
    }

    void TangentMapBody::Power(float power)
    {
      this->power_ = power;
    }

    void TangentMapBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      _(main) <<
        "float3 n = normalize(normal);\n"
        "float3 t = normalize(tangent);\n"
        "float3 b = normalize(cross(n, t));\n"

        "float3 cam_dir=camera_position.xyz-position.xyz;\n"
        "float3 view_direction;\n"
        "view_direction.x = dot(t, cam_dir);\n"
        "view_direction.y = dot(b, cam_dir);\n"
        "view_direction.z = dot(n, cam_dir);\n"
        "view_direction = normalize(view_direction);\n"
        ;
    }

    int TangentMapBody::PipeType(int format)
    {
      
      return FTX_TEX1 | FTX_VIEW_DIRECTION;
    }

    void TangentMapBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      _(constant) << "float4 tangent_map_color:register(c" << used_constant++ << ");\n";

      sampler_ = used_sampler++;

      _(constant) << "texture tex" << sampler_ << ";\n";
      _(constant) << "sampler samp" << sampler_ << ":register(s" << sampler_ << ")=sampler_state\n";
      _(constant) << "{\n";
      _(constant) << "Texture = <tex" << sampler_ << ">;\n";
      _(constant) << "};\n";

      _(main) <<
        "float3 normal_map = (tex2D(samp" << sampler_ << ",uv" << uv_ << ")).xyz * 2.0 - 1.0;\n"
        "float normapmap_specular = pow(max(1-dot(normal_map, view_direction), 0.0), tangent_map_color.w);\n"
        "color.x+=normapmap_specular*tangent_map_color.x;\n"
        "color.y+=normapmap_specular*tangent_map_color.y;\n"
        "color.z+=normapmap_specular*tangent_map_color.z;\n"
        ;
    }

    void TangentMapBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      color_.a = power_;
      dev->SetPixelShaderConstantF(sc.pixel_register_offset, color_.ToArray(), 1);
      sc.dd->SetTexture(decal_, sampler_);
    }

    //////////////////////////////////////
    // ColoredNormalMap

    void ColoredNormalMapBody::BaseColor(const Color& color)
    {
      this->base_color_ = color;
    }

    void ColoredNormalMapBody::CenterSpecular(const Color& color)
    {
      this->center_specular_ = color;
    }

    void ColoredNormalMapBody::CenterPower(float power)
    {
      this->center_power_ = power;
    }

    void ColoredNormalMapBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      
    }

    int ColoredNormalMapBody::PipeType(int format)
    {
      
      return 0;
    }

    void ColoredNormalMapBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      

      _(constant) <<
        "float4 colored_normalmap_color:register(c" << used_constant << ");\n"
        "float4 colored_normalmap_specular:register(c" << used_constant + 1 << ");\n"
        ;
      used_constant += 2;

      _(main) <<
        //"color.xyz+=colored_normalmap_color.xyz*normal_map.z;\n"
        "color.xyz+=colored_normalmap_specular.xyz*pow(dot(normal_map,view_direction),colored_normalmap_specular.w);\n"
        ;
    }

    void ColoredNormalMapBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      dev->SetPixelShaderConstantF(sc.pixel_register_offset, base_color_.ToArray(), 1);
      center_specular_.a = center_power_;
      dev->SetPixelShaderConstantF(sc.pixel_register_offset + 1, center_specular_.ToArray(), 1);
    }

    //////////////////////////////////////
    // AlphaBlendTexture

    AlphaBlendTextureImpl::AlphaBlendTextureImpl()
      :uv_(0)
    {
    }

    void AlphaBlendTextureImpl::SetTexture(Texture* t)
    {
      decal_ = t;
    }

    void AlphaBlendTextureImpl::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      
    }

    int AlphaBlendTextureImpl::PipeType(int format)
    {
      
      return FTX_TEX1;
    }

    void AlphaBlendTextureImpl::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      

      sampler_ = used_sampler++;

      _(constant) << "texture tex" << sampler_ << ";\n";
      _(constant) << "sampler samp" << sampler_ << ":register(s" << sampler_ << ")=sampler_state\n";
      _(constant) << "{\n";
      _(constant) << "Texture = <tex" << sampler_ << ">;\n";
      _(constant) << "};\n";

      _(main) << "color.a=color.a*tex2D(samp" << sampler_ << ",uv" << uv_ << ").a;\n";
    }

    void AlphaBlendTextureImpl::SetupConstant(ShaderConstant& sc)
    {
      sc.dd->SetTexture(decal_, sampler_);
    }

    //////////////////////////////////////
    // RGBModAlpha

    RGBModAlpha* CreateRGBModAlpha()
    {
      return new RGBModAlphaBody;
    }

    void RGBModAlphaBody::Mod(const Color& color)
    {
      this->color_ = color;
    }

    void RGBModAlphaBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      
    }

    int RGBModAlphaBody::PipeType(int format)
    {
      
      return 0;
    }

    void RGBModAlphaBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      

      _(constant) << "float4 rgb_mod_alpha_color:register(c" << used_constant << ");\n";
      used_constant += 1;

      _(main) << "color.a=color.a*rgb_mod_alpha_color.a;\n";
      _(main) << "color.rgb=rgb_mod_alpha_color.rgb;\n";
    }

    void RGBModAlphaBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      dev->SetPixelShaderConstantF(sc.pixel_register_offset, color_.ToArray(), 1);
    }

    //////////////////////////////////////
    // SetRGBA

    void SetRGBABody::Set(const Color& color)
    {
      this->color_ = color;
    }

    void SetRGBABody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      
    }

    int SetRGBABody::PipeType(int format)
    {
      
      return 0;
    }

    void SetRGBABody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      

      _(constant) << "float4 set_rgba_color:register(c" << used_constant << ");\n";
      used_constant += 1;

      _(main) << "color=set_rgba_color;\n";
    }

    void SetRGBABody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      dev->SetPixelShaderConstantF(sc.pixel_register_offset, color_.ToArray(), 1);
    }

    //////////////////////////////////////
    // Particle

    ParticleBody::ParticleBody()
    {
      this->count_[0] = 1;
      this->count_[1] = 1;
    }
    void ParticleBody::SetCount(float count)
    {
      this->count_[2] = count;
      this->count_[3] = count * count / 2;
    }

    void ParticleBody::SetSize(float size)
    {
      this->count_[0] = size;
      this->count_[1] = size;
    }

    void ParticleBody::SetCamera(Camera* camera)
    {
      this->camera_ = static_cast<CameraBody*>(camera);
    }

    void ParticleBody::SetPosition(Vector3 pos)
    {
      this->position_ = pos;
    }

    void ParticleBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      

      _(constant) << "float4 Particle_count:register(c" << (used_constant + 0) << ");\n";
      _(constant) << "float4 Particle_position:register(c" << (used_constant + 1) << ");\n";
      _(constant) << "float4x4 mat_view_projection:register(c" << (used_constant + 2) << ");\n";
      used_constant += 6;

      _(main) << "position.xyz=normal*Particle_count.w+position.xyz*Particle_count.z;\n";
      _(main) << "position.xyz=position.xyz+Particle_position.xyz;\n";
      _(main) << "position=mul(position,mat_view_projection);\n";
      _(main) << "position.xy=position.xy+(uv0-float2(0.5,0.5))*Particle_count.xy;\n";
      _(main) << "position.y=position.y*Particle_position.w;\n";

      //vertex_slot_camera_position_=used_constant++;
      //constant<<"float3 camera_position:register(c"<<vertex_slot_camera_position_<<");\n";
    }

    int ParticleBody::PipeType(int format)
    {
      
      return 0;
    }

    void ParticleBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
    }

    void ParticleBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 0, count_, 1);
      position_.w = camera_->projection_._22;
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 1, (float*)&position_, 1);
      float* f = MatrixTranspose(camera_->view_projection_);
      f[4] /= camera_->projection_._22;
      f[5] /= camera_->projection_._22;
      f[6] /= camera_->projection_._22;
      f[7] /= camera_->projection_._22;
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 2, f, 4);
    }

    //////////////////////////////////////
    // Pointicle

    PointicleBody::PointicleBody()
    {
      this->count_[0] = 1;
      this->count_[1] = 1;
    }
    void PointicleBody::SetCount(float count, float tail_count)
    {
      this->count_[2] = count;
      this->count_[1] = tail_count;
    }

    void PointicleBody::SetAlpha(float alpha)
    {
      this->count_[3] = alpha;
    }

    void PointicleBody::SetSize(float size)
    {
      this->count_[0] = size;
    }

    void PointicleBody::SetCamera(Camera* camera)
    {
      this->camera_ = static_cast<CameraBody*>(camera);
    }

    void PointicleBody::SetPosition(Vector3 pos)
    {
      this->position_ = pos;
    }

    void PointicleBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      

      _(constant) << "float4 Pointicle_count:register(c" << (used_constant + 0) << ");\n";
      _(constant) << "float4 Pointicle_position:register(c" << (used_constant + 1) << ");\n";
      _(constant) << "float4x4 mat_view_projection:register(c" << (used_constant + 2) << ");\n";
      used_constant += 6;

      _(main) << "position.xyz=Pointicle_position.xyz+position.xyz*Pointicle_count.z;\n";
      if (format & FTX_TANGENT)
      {
        _(main) << "position.xyz=position.xyz+tangent*Pointicle_count.y;\n";
      }
      _(main) << "position=mul(position,mat_view_projection);\n";
      if (format & FTX_NORMAL)
      {
        _(main) << "normal=mul(normal,mat_view_projection);\n";
        _(main) << "normal.xy=normalize(normal.xy);\n";
        _(main) << "position.xy=position.xy+normal.xy*Pointicle_count.xx;\n";
      }
      else
      {
        _(main) << "position.xy=position.xy+uv0*Pointicle_count.xx;\n";
      }
      _(main) << "position.x=position.x*Pointicle_position.w;\n";
      if (format & FTX_DIFFUSE)
      {
        _(main) << "diffuse.a=diffuse.a*Pointicle_count.w;\n";
      }

      //vertex_slot_camera_position_=used_constant++;
      //constant<<"float3 camera_position:register(c"<<vertex_slot_camera_position_<<");\n";
    }

    int PointicleBody::PipeType(int format)
    {
      
      return format | FTX_TEX1;
    }

    void PointicleBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      

      //main<<"float point_alpha=1.2-dot(uv0,uv0);\n";
      //main<<"point_alpha=min(1,max(0,point_alpha));\n";
      _(main) << "float point_alpha=1-dot(uv0,uv0);\n";
      //main<<"point_alpha=point_alpha*point_alpha;\n";
      _(main) << "point_alpha=max(0,point_alpha);\n";
      _(main) << "color.a=color.a*point_alpha*point_alpha;\n";
      //main<<"color.xyz=color.xyz*color.a;\n";
    }

    void PointicleBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      static float aspect = (float)sc.dd->Height() / sc.dd->Width();
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 0, count_, 1);
      position_.w = aspect;
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 1, (float*)&position_, 1);
      float* f = MatrixTranspose(camera_->view_projection_);
      f[0] /= aspect;
      f[1] /= aspect;
      f[2] /= aspect;
      f[3] /= aspect;
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 2, f, 4);
    }

    //////////////////////////////////////
    // LoopJet

    LoopJetHLSL::LoopJetHLSL()
      :count_(0)
    {
    }
    void LoopJetHLSL::SetCount(float count)
    {
      this->count_ = count;
    }

    void LoopJetHLSL::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      

      _(constant) << "float4 LoopJet_count:register(c" << (used_constant + 0) << ");\n";
      used_constant += 1;

      _(main) << "float time_bias=fmod(LoopJet_count.x+position.z,1.0);\n"
        "position.z=time_bias*time_bias;\n";
      if (format & FTX_DIFFUSE)
      {
        _(main) << "diffuse.a=diffuse.a*(1.0-pow(time_bias,4.0));\n";
      }
    }

    int LoopJetHLSL::PipeType(int format)
    {
      return format | FTX_TEX1;
    }

    void LoopJetHLSL::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      

      _(main) << "float point_alpha=1-dot(uv0,uv0);\n";
      _(main) << "point_alpha=max(0,point_alpha);\n";
      _(main) << "color.a=color.a*point_alpha*point_alpha;\n";
    }

    void LoopJetHLSL::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      float count[4] = {
        count_,
        0,
        0,
        0,
      };
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 0, count, 1);
    }

    //////////////////////////////////////
    // Billboard

    BillboardHLSL::BillboardHLSL()
      :aspect_(1.6f / 1.0f)
      , size_(1.0f)
    {
    }

    void BillboardHLSL::SetAspect(float aspect)
    {
      this->aspect_ = aspect;
    }

    void BillboardHLSL::SetSize(float size)
    {
      this->size_ = size;
    }

    void BillboardHLSL::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      

      _(constant) << "float4 Billboard_count:register(c" << (used_constant + 0) << ");\n";
      used_constant += 1;

      _(main) <<

        "position.xy=position.xy+uv0*Billboard_count.xy;\n";
    }

    int BillboardHLSL::PipeType(int format)
    {
      return format | FTX_TEX1;
    }

    void BillboardHLSL::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
    }

    void BillboardHLSL::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      float count[4] = {
        size_,
        size_ * aspect_,
        1.0f,
        1.0f
      };
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 0, count, 1);
    }

    //////////////////////////////////////
    // CneterSpecular

    void CenterSpecularBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      

      _(constant) << "float4 CenterSpecular_color:register(c" << used_constant << ");\n";
      used_constant += 1;

      _(main) << "specular.xyz=specular.xyz+CenterSpecular_color.xyz*max(0,-dot(normalize(position.xyz),normal.xyz));\n";

    }

    int CenterSpecularBody::PipeType(int format)
    {
      return format | FTX_SPECULAR;
    }

    void CenterSpecularBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      
    }

    void CenterSpecularBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      dev->SetVertexShaderConstantF(sc.vertex_register_offset, specular_.ToArray(), 1);
    }

    //////////////////////////////////////
    // EdgeSpecular

    void EdgeSpecularBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      

      _(constant) << "float4 EdgeSpecular_color:register(c" << used_constant << ");\n";
      used_constant += 1;

      _(main) << "specular=specular+EdgeSpecular_color.xyz*pow(1-max(0,dot(camera_direction.xyz,normal.xyz)),EdgeSpecular_color.w);\n";

    }

    int EdgeSpecularBody::PipeType(int format)
    {
      return FTX_SPECULAR;
    }

    void EdgeSpecularBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
    }

    void EdgeSpecularBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      color_.a = power_;
      dev->SetVertexShaderConstantF(sc.vertex_register_offset, color_.ToArray(), 1);
    }

    //////////////////////////////////////
    // BlowNormal

    void BlowNormalBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      _(constant) << "float4 BlowNormal_scale:register(c" << used_constant << ");\n";
      used_constant += 1;

      _(main) << "position.xyz=position.xyz+normal.xyz*BlowNormal_scale.x;\n";

    }

    int BlowNormalBody::PipeType(int format)
    {
      return format;
    }

    void BlowNormalBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
    }

    void BlowNormalBody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      float temp[4] = { scale_,0,0,0 };
      dev->SetVertexShaderConstantF(sc.vertex_register_offset, temp, 1);
    }

    //////////////////////////////////////
    // SetAngleRGBA

    void SetAngleRGBABody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      

      _(constant) << "float4 SetAngleRGBA_center:register(c" << used_constant << ");\n";
      used_constant += 1;
      _(constant) << "float4 SetAngleRGBA_edge:register(c" << used_constant << ");\n";
      used_constant += 1;
      _(main) << "float camera_angle=abs(dot(camera_direction.xyz,normal.xyz));\n";
      _(main) << "diffuse=SetAngleRGBA_center*camera_angle;\n";
      _(main) << "diffuse+=SetAngleRGBA_edge*(1-camera_angle);\n";

    }

    int SetAngleRGBABody::PipeType(int format)
    {
      return format | FTX_DIFFUSE;
    }

    void SetAngleRGBABody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      
    }

    void SetAngleRGBABody::SetupConstant(ShaderConstant& sc)
    {
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 0, center_.ToArray(), 1);
      dev->SetVertexShaderConstantF(sc.vertex_register_offset + 1, edge_.ToArray(), 1);
    }

    //////////////////////////////////////
    // ScreenSpaceDiffuse

    ScreenSpaceDiffuseBody::ScreenSpaceDiffuseBody()
      :uv_(0)
    {
    }

    void ScreenSpaceDiffuseBody::SetTexture(Texture* t)
    {
      diffuse_ = t;
    }

    void ScreenSpaceDiffuseBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      
    }

    int ScreenSpaceDiffuseBody::PipeType(int format)
    {
      
      return FTX_VIEWPORT_POSITION;
    }

    void ScreenSpaceDiffuseBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      

      sampler_ = used_sampler++;

      _(constant) << "float2 ScreenSpace_size:register(c" << used_constant << ");\n";
      used_constant += 1;

      _(constant) << "texture tex" << sampler_ << ";\n";
      _(constant) << "sampler samp" << sampler_ << ":register(s" << sampler_ << ")=sampler_state\n";
      _(constant) << "{\n";
      _(constant) << "Texture = <tex" << sampler_ << ">;\n";
      _(constant) << "};\n";

      //_(main) << "float2 vvpos;\n";

      _(main) <<
#if 1
        "color=color*tex2D(samp" << sampler_ << ",vpos*ScreenSpace_size);\n"
#else
        "color=tex2D(samp" << sampler_ << ",vpos*ScreenSpace_size);\n"
#endif
        ;
    }

    void ScreenSpaceDiffuseBody::SetupConstant(ShaderConstant& sc)
    {
      sc.dd->SetTexture(diffuse_, sampler_);
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      dev->SetSamplerState(sampler_, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      dev->SetSamplerState(sampler_, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      float size[4];
      size[0] = 1.0f / sc.dd->Width();
      size[1] = 1.0f / sc.dd->Height();
      size[2] = 0;
      size[3] = 0;
      dev->SetPixelShaderConstantF(sc.pixel_register_offset + 0, size, 1);
    }

    //////////////////////////////////////
    // GaussianFilter

    GaussianFilterBody::GaussianFilterBody()
      :uv_(0)
    {
    }

    void GaussianFilterBody::SetTexture(Texture* t)
    {
      diffuse_ = t;
    }

    void GaussianFilterBody::Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      
      
      
      
    }

    int GaussianFilterBody::PipeType(int format)
    {
      return FTX_TEX1;
    }

    void GaussianFilterBody::Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main)
    {
      
      

      sampler_ = used_sampler++;

      _(constant) << "float4 GaussianTexture_size:register(c" << used_constant << ");\n";
      used_constant += 1;

      _(constant) << "texture tex" << sampler_ << ";\n";
      _(constant) << "sampler samp" << sampler_ << ":register(s" << sampler_ << ")=sampler_state\n";
      _(constant) << "{\n";
      _(constant) << "Texture = <tex" << sampler_ << ">;\n";
      _(constant) << "};\n";

      //_(main) << "float2 vvpos;\n";

      _(main) <<
        "float3 gaus=float3(0,0,0);\n"
#define HALF_SAMPLE
#ifndef HALF_SAMPLE
        "float3 gaus_scale0=float3(-GaussianTexture_size.x, -GaussianTexture_size.y, 1.0/16.0);\n"
        "float3 gaus_scale1=float3(0,                       -GaussianTexture_size.y, 2.0/16.0);\n"
        "float3 gaus_scale2=float3( GaussianTexture_size.x, -GaussianTexture_size.y, 1.0/16.0);\n"
        "float3 gaus_scale3=float3(-GaussianTexture_size.x, 0,                       2.0/16.0);\n"
        "float3 gaus_scale4=float3(0,                       0,                       4.0/16.0);\n"
        "float3 gaus_scale5=float3( GaussianTexture_size.x, 0,                       2.0/16.0);\n"
        "float3 gaus_scale6=float3(-GaussianTexture_size.x, GaussianTexture_size.y,  1.0/16.0);\n"
        "float3 gaus_scale7=float3(0,                       GaussianTexture_size.y,  2.0/16.0);\n"
        "float3 gaus_scale8=float3( GaussianTexture_size.x, GaussianTexture_size.y,  1.0/16.0);\n"
        "gaus+=tex2D(samp" << sampler_ << ",uv" << uv_ << "+gaus_scale0.xy)*gaus_scale0.z;\n"
        "gaus+=tex2D(samp" << sampler_ << ",uv" << uv_ << "+gaus_scale1.xy)*gaus_scale1.z;\n"
        "gaus+=tex2D(samp" << sampler_ << ",uv" << uv_ << "+gaus_scale2.xy)*gaus_scale2.z;\n"
        "gaus+=tex2D(samp" << sampler_ << ",uv" << uv_ << "+gaus_scale3.xy)*gaus_scale3.z;\n"
        "gaus+=tex2D(samp" << sampler_ << ",uv" << uv_ << "+gaus_scale4.xy)*gaus_scale4.z;\n"
        "gaus+=tex2D(samp" << sampler_ << ",uv" << uv_ << "+gaus_scale5.xy)*gaus_scale5.z;\n"
        "gaus+=tex2D(samp" << sampler_ << ",uv" << uv_ << "+gaus_scale6.xy)*gaus_scale6.z;\n"
        "gaus+=tex2D(samp" << sampler_ << ",uv" << uv_ << "+gaus_scale7.xy)*gaus_scale7.z;\n"
        "gaus+=tex2D(samp" << sampler_ << ",uv" << uv_ << "+gaus_scale8.xy)*gaus_scale8.z;\n"
#else
        "float2 gaus_scale0=float2(GaussianTexture_size.z, GaussianTexture_size.w);\n"
        "float2 gaus_scale1=float2(GaussianTexture_size.x, GaussianTexture_size.w);\n"
        "float2 gaus_scale2=float2(GaussianTexture_size.z, GaussianTexture_size.y);\n"
        "float2 gaus_scale3=float2(GaussianTexture_size.x, GaussianTexture_size.y);\n"
        "gaus+=tex2D(samp" << sampler_ << ",uv" << uv_ << "+gaus_scale0.xy)*0.25;\n"
        "gaus+=tex2D(samp" << sampler_ << ",uv" << uv_ << "+gaus_scale1.xy)*0.25;\n"
        "gaus+=tex2D(samp" << sampler_ << ",uv" << uv_ << "+gaus_scale2.xy)*0.25;\n"
        "gaus+=tex2D(samp" << sampler_ << ",uv" << uv_ << "+gaus_scale3.xy)*0.25;\n"
#endif
        "color.xyz=color.xyz*gaus;\n";

    }

    void GaussianFilterBody::SetupConstant(ShaderConstant& sc)
    {
      sc.dd->SetTexture(diffuse_, sampler_);
      LPDIRECT3DDEVICE9 dev = sc.dd->RawDevice();
      dev->SetSamplerState(sampler_, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      dev->SetSamplerState(sampler_, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      float size[4];
#ifndef HALF_SAMPLE
      size[0] = 1.0f / diffuse_->Desc().width;
      size[1] = 1.0f / diffuse_->Desc().height;
      size[2] = 0;
      size[3] = 0;
#else
      size[0] = 0.5f / diffuse_->Desc().width;
      size[1] = 0.5f / diffuse_->Desc().height;
      size[2] = -0.5f / diffuse_->Desc().width;
      size[3] = -0.5f / diffuse_->Desc().height;
#endif
      dev->SetPixelShaderConstantF(sc.pixel_register_offset + 0, size, 1);
    }

  }
}

#endif
