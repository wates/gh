
#ifndef WTS_GFX_SHADER_D3D9_H_
#define WTS_GFX_SHADER_D3D9_H_

#ifdef _WIN32

#include "shader.h"
#include <string>

namespace gh {
  namespace d3d9 {
    class GraphicsD3D;
  }
  namespace hlsl {
    using namespace gh::shader;

    struct ShaderTuple {
    };

    struct ShaderConstant {
      d3d9::GraphicsD3D* dd;
      int vertex_register_offset;
      int pixel_register_offset;
    };

    struct ShaderGenerator
    {
      virtual void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main) = 0;
      virtual int PipeType(int format) = 0;
      virtual void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main) = 0;
      virtual void SetupConstant(ShaderConstant& sc) = 0;

      bool compiled_ = false;
    };

    class WorldTransformBody
      :public WorldTransform
      , public ShaderGenerator
    {
    private:
      ClassID(WorldTransform)
        inline void Transform(const Matrix& mat) {
        transform_ = mat;
        transform_.Transpose();
      }

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      Matrix transform_;

    };

    class BlendTransformBody
      :public BlendTransform
      , public ShaderGenerator
    {
    public:
      BlendTransformBody();

    private:
      static const int max_matrix_buffer = 48;

      ClassID(BlendTransform)
        inline void ClearTransform() {
        max_using_ = 0;
      }
      void Transform(const Matrix& mat, int slot);
      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      float transform_[16 * max_matrix_buffer];
      int fixed_;
      int max_using_;

    };

    class HugeBlendTransformBody
      :public BlendTransform
      , public ShaderGenerator
    {
    public:
      HugeBlendTransformBody();
    private:
      static const int max_matrix_buffer = 256;
      static const int buffer_width = 4;

      ClassID(HugeBlendTransform)
        inline void ClearTransform() {
        max_using_ = 0;
      }
      void Transform(const Matrix& mat, int slot);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      int sampler_slot_;
      float transform_[16 * max_matrix_buffer];
      int fixed_;
      int max_using_;
      Texture* blends_;
    };

    class CameraBody
      :public Camera
      , public ShaderGenerator
    {
    public:
      ClassID(Camera)
        CameraBody();
      void ScreenBias(float x, float y, float z);
      void PerspectiveFov(float nearclip, float farclip, float fov, float aspect);
      void Ortho(float nearclip, float farclip, float w, float h);
      void LookAt(const Vector3& from, const Vector3& at, const Vector3& up);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      inline const Vector3& Direction()const
      {
        return dir_;
      }

      inline const Matrix& View()const {
        return view_;
      }
      inline const Matrix& Proj()const {
        return projection_;
      }

      Matrix view_;
      Matrix projection_;
      Matrix view_projection_;

      Vector3 bias_;

      Vector3 from_;
      Vector3 dir_;
    };

    class DirectionalLightBody
      :public DirectionalLight
      , public ShaderGenerator
    {
    private:
      ClassID(DirectionalLight)
        inline void Direction(const Vector3& dir) {
        direction_ = dir;
        direction_.Normalize();
      }
      inline void Diffuse(const Color& color) {
        color_ = color;
      }

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      Vector4 direction_;
      Color color_;

    };

    class AmbientLightBody
      :public AmbientLight
      , public ShaderGenerator
    {
    private:
      ClassID(AmbientLight)
        void Ambient(const Color& color);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      Color color_;
    };

    class SpecularLightBody
      :public SpecularLight
      , public ShaderGenerator
    {
    private:
      ClassID(SpecularLight)
        void Specular(const Color& color);
      void Power(float power);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      Color color_;
      float power_;
    };

    class DiffuseLightBody
      :public DiffuseLight
      , public ShaderGenerator
    {
      ClassID(DiffuseLight);
      void Diffuse(const Color& color);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      Vector4 diffuse_;
    };

    class FractalBody
      :public Fractal
      , public ShaderGenerator
    {
    public:
      FractalBody();

      ClassID(Fractal);
      void ColorTable(Texture*);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      Texture* table_;
      int sampler_;
      //int vertex_slot_color;
      //Color color;
    };

    class DecalTextureBody
      :public DecalTexture
      , public ShaderGenerator
    {
    public:
      DecalTextureBody();
    private:

      ClassID(DecalTexture)

        void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);
      void SetTexture(Texture* t);

      Texture* decal_;
      int sampler_;
      int uv_;
    };

    class NormalMapBody
      :public NormalMap
      , public ShaderGenerator
    {
    public:
      NormalMapBody();
    private:

      ClassID(NormalMap)

        void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);
      void SetTexture(Texture* t);

      Texture* decal_;
      int sampler_;
      int uv_;
    };

    class TangentMapBody
      :public TangentMap
      , public ShaderGenerator
    {
    public:
      TangentMapBody();
    private:

      ClassID(TangentMap)

        void Specular(const Color& color);
      void Power(float power);
      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);
      void SetTexture(Texture* t);

      Texture* decal_;
      int sampler_;
      int uv_;
      Color color_;
      float power_;
    };

    class ColoredNormalMapBody
      :public ColoredNormalMap
      , public ShaderGenerator
    {
    private:
      ClassID(ColoredNormalMap)
        void BaseColor(const Color& color);
      void CenterSpecular(const Color& color);
      void CenterPower(float power);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      Color base_color_;
      Color center_specular_;
      float center_power_;
    };

    class AlphaBlendTextureImpl
      :public DecalTexture
      , public ShaderGenerator
    {
    public:
      AlphaBlendTextureImpl();
    private:

      ClassID(AlphaBlendTexture)

        void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);
      void SetTexture(Texture* t);

      Texture* decal_;
      int sampler_;
      int uv_;
    };

    class RGBModAlphaBody
      :public RGBModAlpha
      , public ShaderGenerator
    {
    private:
      ClassID(RGBModAlpha)
        void Mod(const Color& color);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      Color color_;
    };

    class SetRGBABody
      :public SetRGBA
      , public ShaderGenerator
    {
    private:
      ClassID(SetRGBA)
        void Set(const Color& color);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      Color color_;
    };

    class ParticleBody
      :public Particle
      , public ShaderGenerator
    {
    public:
      ParticleBody();
    private:
      ClassID(Particle)

        void SetCount(float count);
      void SetCamera(Camera* camera);
      void SetPosition(Vector3 pos);
      void SetSize(float size);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      CameraBody* camera_;
      float count_[4];
      Vector4 position_;
    };

    class PointicleBody
      :public Pointicle
      , public ShaderGenerator
    {
    public:
      PointicleBody();
    private:
      ClassID(Pointicle)

        inline void SetAspect(float aspect) {};
      void SetCount(float count, float tail_count);
      void SetAlpha(float alpha);
      void SetCamera(Camera* camera);
      void SetPosition(Vector3 pos);
      void SetSize(float size);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      CameraBody* camera_;
      float aspect_;
      float count_[4];
      Vector4 position_;
    };

    class LoopJetHLSL
      :public shader::LoopJet
      , public ShaderGenerator
    {
    public:
      LoopJetHLSL();
    private:
      ClassID(LoopJet)

        void SetCount(float count);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      float count_;
    };

    class BillboardHLSL
      :public shader::Billboard
      , public ShaderGenerator
    {
    public:
      BillboardHLSL();
    private:
      ClassID(Billboard)

        void SetAspect(float aspect);
      void SetSize(float size);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      float aspect_;
      float size_;
    };

    class CenterSpecularBody
      :public CenterSpecular
      , public ShaderGenerator
    {
    public:
      inline CenterSpecularBody() {};
    private:
      ClassID(CenterSpecular)

        inline void Specular(const Color& color) {
        specular_ = color;
      }

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      Color specular_;
    };

    class EdgeSpecularBody
      :public EdgeSpecular
      , public ShaderGenerator
    {
    private:
      ClassID(EdgeSpecular)
        inline void Specular(const Color& color) {
        color_ = color;
      }
      inline void Power(float power) {
        power_ = power;
      }

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      Color color_;
      float power_;
    };

    class BlowNormalBody
      :public BlowNormal
      , public ShaderGenerator
    {
    private:
      ClassID(BlowNormal)
        inline void Set(float scale) {
        scale_ = scale;
      }

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      float scale_;
    };

    class SetAngleRGBABody
      :public SetAngleRGBA
      , public ShaderGenerator
    {
    private:
      ClassID(SetAngleRGBA)
        void Set(const Color& center, const Color& edge) {
        center_ = center;
        edge_ = edge;
      }

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);

      Color center_;
      Color edge_;
    };

    class ScreenSpaceDiffuseBody
      :public ScreenSpaceDiffuse
      , public ShaderGenerator
    {
    public:
      ScreenSpaceDiffuseBody();
    private:

      ClassID(ScreenSpaceDiffuse)

        void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);
      void SetTexture(Texture* t);

      Texture* diffuse_;
      int sampler_;
      int uv_;
    };

    class GaussianFilterBody
      :public GaussianFilter
      , public ShaderGenerator
    {
    public:
      GaussianFilterBody();
    private:

      ClassID(GaussianFilter);

      void Vertex(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      int PipeType(int format);
      void Pixel(int format, std::string& constant, int& used_constant, int& used_sampler, std::string& main);
      void SetupConstant(ShaderConstant& sc);
      void SetTexture(Texture* t);

      Texture* diffuse_;
      int sampler_;
      int uv_;
    };

  }
}


#endif


#endif
