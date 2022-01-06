
#ifndef GH_RENDER_H_
#define GH_RENDER_H_

#include "graphics.h"
#include "polygon.h"

namespace gh {

  struct Faces
  {
    int faces;
    Material material;
  };

  struct BoneIndex
  {
    Matrix offset;
    std::string index;
  };

  struct Primitive
  {
    VertexBuffer* vb;
    IndexBuffer* ib;
    std::vector<char> raw_vertex;
    std::vector<Faces> subset;
    std::vector<BoneIndex> boneindex;
    std::vector<Morphing> morph;
    void ApplyMorph(std::vector<float> weight);
  };

  struct Model //deprecated
  {
    Matrix transform;
    std::string name;
    std::vector<Primitive> primitive;
    std::vector<Model> child;
  };

  class Mesh
  {
  public:
    /**
     * @param primitive_type
     */
    virtual void InitOneSkin(const Shape& shape, PrimitiveType primitive_type = gh::PRIMITIVE_TRIANGLELIST) = 0;
    virtual void InitOneSkinMultiForm(const Shape& shape, int max_form = 30) = 0;
    virtual void InitWireframe(const Shape& shape) = 0;
    virtual void Release() = 0;
    virtual void SetPathPrefix(const std::string prefix) = 0;

    virtual void Draw(const Matrix& mat) = 0;
    virtual void DrawFaces(const Matrix& mat, int faces) = 0;
    virtual void DrawMultiForm(const Matrix* mats, int count, int faces) = 0;
    virtual void DrawPose(const Matrix& mat, AnimatedPose* pose) = 0;
    virtual void DrawPoseLighting(const Matrix& mat, AnimatedPose* pose) = 0;
    virtual void FlushMultiform() = 0;

    virtual void ApplyMorph(const std::vector<float>& weight) = 0;
    virtual void ApplyNamedMorph(const std::map<std::string, float>& weight) = 0;
    virtual void SetCenterSpecular(int subset, const Color& color) = 0;
    virtual void SetEdgeSpecular(int subset, const Color& color, float power) = 0;
    virtual void SetBlowNormal(int subset, float scale) = 0;
    virtual void SetRGBA(int subset, const Color& col) = 0;
    virtual void SetAngleRGBA(int subset, const Color& center, const Color& edge) = 0;
    virtual void EnableNormalmap(int subset) = 0;
    virtual void EnableTangentmap(int subset, const Color& color, float power) = 0;
    virtual void EnableColoredNormalmap(const Color& color, const Color& specular, float power) = 0;
    virtual void SetScreenSpaceDiffuse(int subset, Texture* diffuse) = 0;

    virtual void UseCamera(bool use) = 0;
    virtual void EnableAddAlpha() = 0;
  protected:
    inline virtual ~Mesh() {};
  };


  struct PoseBuffer
  {
    std::vector<std::string> bone;
    Texture* buffer;
  };

  class Grid
  {
  public:
    virtual bool Build(int width, int height, const std::string& texture) = 0;
    virtual void Draw(const Matrix& mat) = 0;
    virtual void UpdateTile(const int* tile) = 0;
  public:
    inline Grid() {}
    inline virtual ~Grid() {}
  };

  class Particle
  {
  public:
    struct Chip {
      Vector3 velocity;
      Vector3 acceleration;
    };

    virtual bool Init(const std::string& texture, std::vector<Chip> motion) = 0;
    virtual void Draw(const Vector3& position, const Color& color, float count, float size) = 0;
  public:
    inline Particle() {}
    inline virtual ~Particle() {}
  };

  class Pointicle
  {
  public:
    struct Chip {
      Vector3 velocity;
      Color color;
    };

    virtual bool Init(std::vector<Chip> motion) = 0;
    virtual bool InitTri(std::vector<Chip> motion) = 0;
    virtual void Draw(const Vector3& position, float alpha, float count, float size) = 0;
  public:
    inline Pointicle() {}
    inline virtual ~Pointicle() {}
  };

  class Linecle
  {
  public:
    struct Chip {
      Vector3 velocity;
      Color color;
    };

    virtual bool Init(std::vector<Chip> motion) = 0;
    virtual void Draw(const Vector3& position, float alpha, float count, float length, float size) = 0;
  public:
    inline Linecle() {}
    inline virtual ~Linecle() {}
  };

  class LoopJet
  {
  public:
    struct Chip {
      Vector3 velocity;
      Color color;
      float offset;//0.0 - 1.0
    };

    virtual bool Init(std::vector<Chip> motion) = 0;
    virtual bool InitTri(std::vector<Chip> motion) = 0;
    virtual void Draw(const Matrix& pose, float count, float size) = 0;
  public:
    inline LoopJet() {}
    inline virtual ~LoopJet() {}
  };

  struct Frect {
    float x1;
    float y1;
    float x2;
    float y2;
  };
  inline Frect MakeFrect(float x1, float y1, float x2, float y2)
  {
    Frect f = { x1,y1,x2,y2 };
    return f;
  }

  static const Frect FULL_RECT = { 0.0f,0.0f,0.0f,0.0f };

  class ManagedGraphics
  {
  public:
    ManagedGraphics();
    bool Initialize(Graphics* raw);
    bool Finalize();
    inline Graphics* Raw() { return raw_; }

    Texture* GetTexture(const char* path);
    void AppendTexture(const char* path, Texture* t);

    Texture* CreateColorTexture(int width, int height, const Color* bitmap);

    Mesh* CreateMesh();

    void CreatePrimitiveFromSubset(Primitive* primitive, const Shape& shape, int subset_number);
    void CreateVBIBFromShape(VertexBuffer** vb, IndexBuffer** ib, const Shape& shape);
    void CreateModelFromBone(Model* model, const Bone& bone);
    bool CreateGrid(Grid** iface);
    void CreateParticle(Particle** iface);
    void CreatePointicle(Pointicle** obj);
    void CreateLinecle(Linecle** obj);
    void CreateLoopJet(LoopJet** obj);

    void DeleteModel(const std::string& name);

    Bone* GetBone(const std::string& name);
    Model* GetModel(const std::string& name);
    Animation* GetAnimation(const std::string& name);
    AnimatedPose* GetAnimatedPose(const std::string& bonename, const std::string& animname);

    void DrawPrimitive(const Matrix& mat, const Primitive& primitive);
    void DrawSprite(const Matrix& mat, Texture* tex, Frect fr = FULL_RECT, uint32_t color = 0x00000000);
    enum Centering {
      LEFT_TOP, TOP, RIGHT_TOP,
      LEFT, CENTER, RIGHT,
      LEFT_BOTTOM, BOTTOM, RIGHT_BOTTOM
    };
    void DrawAspectedSprite(Centering c, const Matrix& mat, Texture* tex, Frect fr = FULL_RECT, uint32_t color = 0x00000000);
    void DrawGaussian(Texture* from, Texture* to);
    void DrawModel(Matrix mat, const Model* model);
    void DrawAnimatedModel(Matrix mat, const Model& model, const AnimatedPose& pose);

  public:
    bool directional_light_enable_;
    Graphics* raw_;

    shader::Camera* camera_;
    shader::DirectionalLight* directional_light_;
    shader::SpecularLight* specular_light_;
    shader::AmbientLight* ambient_light_;
    shader::WorldTransform* world_transform_;
    shader::BlendTransform* blend_transform_;
    shader::BlendTransform* huge_blend_transform_;
    shader::DiffuseLight* diffuse_light_;
    shader::DecalTexture* decal_texture_;
    shader::NormalMap* normal_map_;
    shader::TangentMap* tangent_map_;
    shader::ColoredNormalMap* colored_normalmap_;
    shader::Particle* particle_;
    shader::SetRGBA* set_rgba_;
    shader::Pointicle* pointicle_;
    shader::LoopJet* loop_jet_;
    shader::CenterSpecular* center_specular_;
    shader::EdgeSpecular* edge_specular_;
    shader::BlowNormal* blow_normal_;
    shader::SetAngleRGBA* set_angle_rgba_;
    shader::Billboard* billboard_;
    shader::ScreenSpaceDiffuse* ssdiffuse_;
    shader::GaussianFilter* gaussian_filter_;

    std::map<std::string, Bone> bone_map_;
    std::map<std::string, Model> model_map_;
    std::map<std::string, Animation> animation_map_;
    std::map<std::pair<std::string, std::string>, AnimatedPose> pose_map_;

    std::map<std::string, Texture*> textures_;
  public:
    inline ~ManagedGraphics() {}
  private:
    void BuildPreset();
  };

  bool CreateManagedGraphics(ManagedGraphics** mg);
}


#endif

