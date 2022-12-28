#include "gfx/graphics.h"
#include "gfx/polygon.h"
#include <functional>
class Mesh
{
public:
  virtual void Setup(gh::Graphics*) = 0;
  virtual void InitOneSkin(const gh::Shape& shape, gh::PrimitiveType primitive_type = gh::PRIMITIVE_TRIANGLELIST) = 0;
  virtual void Draw() = 0;
  virtual void Draw2(std::function<void(int)> f) = 0;
};

Mesh* CreateMesh();

class Scene {
public:
  virtual gh::Bone* GetBone(const std::string& name)=0;
};

void CreateScene(Scene** a);
