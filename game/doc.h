

#include "tuid.h"
#include <map>
#include <list>
#include "gfx/geometory.h"

typedef typename TUID<struct Element> EID;

struct Element {
  enum Type {
    ELEMENT,
    NODE,
    FOVCAMERA,
    POSE,
    GRID,
    MESH,
    HEXBLOCK,
    ICOSPHERE,
    AACUBE
  }type;
  EID id;
  inline Element(Type t) :type(t) {}
};

struct Node :public Element {
  inline Node() :Element(NODE) {};
  std::list<Element*> node;
  void Append(Element*);
};

struct FovCamera :public Element {
  inline FovCamera() :Element(FOVCAMERA) {};
  gh::Vector3 from;
  gh::Vector3 at;
  gh::Vector3 up;
  float fov;
  float near_clip;
  float far_clip;
  float aspect;
};

struct Pose :public Element {
  gh::Matrix pose;
};

struct Grid :public Element {
  int width;
  int height;
  float size;
};

struct DocMesh :public Element {
  inline DocMesh() :Element(MESH) {};
  std::string path;
  gh::Matrix pose;
  int shape_index = 0;
};

struct HexBlock :public Element {
  inline HexBlock() :Element(HEXBLOCK) {};
  int x, y;
  int bottom, top;
};

struct IcoSphere :public Element {
  inline IcoSphere() :Element(ICOSPHERE) {};

};

struct AACube :public Element {
  gh::Vector3 pos;
};


#include "gfx/render.h"

struct Render {
  virtual void Draw(Node* root) = 0;
};

struct DocRender :public Render {
  gh::Graphics* gf;
  gh::ManagedGraphics* mg;
  gh::shader::Camera* cam;
  std::map<std::string, gh::Mesh*> mesh;
  DocRender(gh::Graphics* gf);
  void Draw(Node* root);
};
