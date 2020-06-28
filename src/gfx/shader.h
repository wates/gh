
#ifndef WTS_GFX_SHADER_H_
#define WTS_GFX_SHADER_H_

#include "geometory.h"
#include "color.h"
namespace gh {

  class Texture;

  namespace shader {

    // type infomation
    enum ShaderType
    {
      ClassID_WorldTransform = 1,
      ClassID_BlendTransform,
      ClassID_HugeBlendTransform,
      ClassID_Camera,
      ClassID_DirectionalLight,
      ClassID_AmbientLight,
      ClassID_DiffuseLight,
      ClassID_SpecularLight,
      ClassID_Fractal,
      ClassID_DecalTexture,
      ClassID_NormalMap,
      ClassID_TangentMap,
      ClassID_ColoredNormalMap,
      ClassID_AlphaBlendTexture,
      ClassID_RGBModAlpha,
      ClassID_SetRGBA,
      ClassID_Particle,
      ClassID_Pointicle,
      ClassID_LoopJet,
      ClassID_CenterSpecular,
      ClassID_EdgeSpecular,
      ClassID_BlowNormal,
      ClassID_SetAngleRGBA,
      ClassID_Billboard,
      ClassID_ScreenSpaceDiffuse,
      ClassID_GaussianFilter,
    };
#define ClassID(c)\
    bool compiled_;\
    inline unsigned char GetClassID()const{return ClassID_##c;}\
    inline void* GetShaderGenerator(){return static_cast<ShaderGenerator*>(this);}

    template<ShaderType T>struct TypeInterface;
    template<>struct TypeInterface < ClassID_WorldTransform > { typedef class WorldTransform type; };
    template<>struct TypeInterface < ClassID_BlendTransform > { typedef class BlendTransform type; };
    template<>struct TypeInterface < ClassID_HugeBlendTransform > { typedef class BlendTransform type; };
    template<>struct TypeInterface < ClassID_Camera > { typedef class Camera type; };
    template<>struct TypeInterface < ClassID_DirectionalLight > { typedef class DirectionalLight type; };
    template<>struct TypeInterface < ClassID_AmbientLight > { typedef class AmbientLight type; };
    template<>struct TypeInterface < ClassID_DiffuseLight > { typedef class DiffuseLight type; };
    template<>struct TypeInterface < ClassID_SpecularLight > { typedef class SpecularLight type; };
    template<>struct TypeInterface < ClassID_Fractal > { typedef class Fractal type; };
    template<>struct TypeInterface < ClassID_DecalTexture > { typedef class DecalTexture type; };
    template<>struct TypeInterface < ClassID_NormalMap > { typedef class NormalMap type; };
    template<>struct TypeInterface < ClassID_TangentMap > { typedef class TangentMap type; };
    template<>struct TypeInterface < ClassID_ColoredNormalMap > { typedef class ColoredNormalMap type; };
    template<>struct TypeInterface < ClassID_AlphaBlendTexture > { typedef class DecalTexture type; };
    template<>struct TypeInterface < ClassID_RGBModAlpha > { typedef class RGBModAlpha type; };
    template<>struct TypeInterface < ClassID_SetRGBA > { typedef class SetRGBA type; };
    template<>struct TypeInterface < ClassID_Particle > { typedef class Particle type; };
    template<>struct TypeInterface < ClassID_Pointicle > { typedef class Pointicle type; };
    template<>struct TypeInterface < ClassID_LoopJet> { typedef class LoopJet type; };
    template<>struct TypeInterface < ClassID_CenterSpecular > { typedef class CenterSpecular type; };
    template<>struct TypeInterface < ClassID_EdgeSpecular > { typedef class EdgeSpecular type; };
    template<>struct TypeInterface < ClassID_BlowNormal > { typedef class BlowNormal type; };
    template<>struct TypeInterface < ClassID_SetAngleRGBA > { typedef class SetAngleRGBA type; };
    template<>struct TypeInterface < ClassID_Billboard> { typedef class Billboard type; };
    template<>struct TypeInterface < ClassID_ScreenSpaceDiffuse> { typedef class ScreenSpaceDiffuse type; };
    template<>struct TypeInterface < ClassID_GaussianFilter> { typedef class GaussianFilter type; };

    class Shader
    {
    public:
      /** @return specified generator class
       */
      virtual unsigned char GetClassID()const = 0;
      virtual void* GetShaderGenerator() = 0;
      virtual ~Shader();
    };

    Shader* ShaderEnd();
    Shader* ShaderIgnore();

    class WorldTransform
      :public Shader
    {
    public:
      virtual void Transform(const Matrix& mat) = 0;
    };

    class BlendTransform
      :public Shader
    {
    public:
      virtual void ClearTransform() = 0;
      virtual void Transform(const Matrix& mat, int slot) = 0;
    };

    class Camera
      :public Shader
    {
    public:
      virtual void ScreenBias(float x, float y, float z) = 0;
      virtual void Ortho(float nearclip, float farclip, float w, float h) = 0;
      virtual void PerspectiveFov(float nearclip, float farclip, float fov, float aspect) = 0;
      virtual void LookAt(const Vector3& from, const Vector3& at, const Vector3& up) = 0;
      virtual const Vector3& Direction()const = 0;
      virtual const Matrix& View()const = 0;
      virtual const Matrix& Proj()const = 0;
    };

    class DirectionalLight
      :public Shader
    {
    public:
      virtual void Direction(const Vector3& dir) = 0;
      virtual void Diffuse(const Color& color) = 0;
    };

    class AmbientLight
      :public Shader
    {
    public:
      virtual void Ambient(const Color& color) = 0;
    };

    class SpecularLight
      :public Shader
    {
    public:
      virtual void Specular(const Color& color) = 0;
      virtual void Power(float power) = 0;
    };

    class DiffuseLight
      :public Shader
    {
    public:
      virtual void Diffuse(const Color& color) = 0;
    };

    class Fractal
      :public Shader
    {
    public:
      virtual void ColorTable(Texture* tex) = 0;
    };

    class DecalTexture
      :public Shader
    {
    public:
      virtual void SetTexture(Texture* tex) = 0;
    };

    class NormalMap
      :public Shader
    {
    public:
      virtual void SetTexture(Texture* tex) = 0;
    };

    class TangentMap
      :public Shader
    {
    public:
      virtual void Specular(const Color& color) = 0;
      virtual void Power(float power) = 0;
      virtual void SetTexture(Texture* tex) = 0;
    };

    class ColoredNormalMap
      :public Shader
    {
    public:
      virtual void BaseColor(const Color& color) = 0;
      virtual void CenterSpecular(const Color& color) = 0;
      virtual void CenterPower(float power) = 0;
    };

    class RGBModAlpha
      :public Shader
    {
    public:
      virtual void Mod(const Color& rgb) = 0;
    };

    class SetRGBA
      :public Shader
    {
    public:
      virtual void Set(const Color& rgba) = 0;
    };

    class Particle
      :public Shader
    {
    public:
      virtual void SetCount(float count) = 0;
      virtual void SetCamera(Camera* camera) = 0;
      virtual void SetPosition(Vector3 pos) = 0;
      virtual void SetSize(float size) = 0;
    };

    class Pointicle
      :public Shader
    {
    public:
      virtual void SetAspect(float aspect) = 0;
      virtual void SetCount(float count, float tail_count = 0) = 0;
      virtual void SetAlpha(float alpha) = 0;
      virtual void SetCamera(Camera* camera) = 0;
      virtual void SetPosition(Vector3 pos) = 0;
      virtual void SetSize(float size) = 0;
    };

    class LoopJet
      :public Shader
    {
    public:
      virtual void SetCount(float count) = 0;
    };

    class Billboard
      :public Shader
    {
    public:
      virtual void SetAspect(float aspect) = 0;
      virtual void SetSize(float size) = 0;
    };

    class CenterSpecular
      :public Shader
    {
    public:
      virtual void Specular(const Color& color) = 0;
    };

    class EdgeSpecular
      :public Shader
    {
    public:
      virtual void Specular(const Color& color) = 0;
      virtual void Power(float power) = 0;
    };

    class BlowNormal
      :public Shader
    {
    public:
      virtual void Set(float scale) = 0;
    };

    class SetAngleRGBA
      :public Shader
    {
    public:
      virtual void Set(const Color& center, const Color& edge) = 0;
    };

    class ScreenSpaceDiffuse
      :public Shader
    {
    public:
      virtual void SetTexture(Texture* tex) = 0;
    };

    class GaussianFilter
      :public Shader
    {
    public:
      virtual void SetTexture(Texture* tex) = 0;
    };

  }
}

#endif
