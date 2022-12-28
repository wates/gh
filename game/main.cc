#include "sys/viewport.h"
#include "gfx/graphics.h"
#include "gfx/fontmap.h"
#include "sys/fs.h"
#include "main/underscore.h"

#include "gfx/render.h"
#include "gfx/polygon.h"

#include <thread>
#include <chrono>
#include <functional>
using namespace gh;
#include "doc.h"

typedef typename TUID<struct Controller> CID;
IdGenerator<std::uint32_t> IdTypeGenerator<struct Controller>::gen;

std::vector<Controller*> con;
struct Controller {
  CID parent;
  virtual void Update() = 0;
  virtual void Draw() = 0;
  virtual int Mouse(int x, int y, int z, unsigned int button) = 0;
};

struct VP :public ViewportMessage {
  void CreateFail() {

  }
  void CloseButton() {
    is_close = true;
  }
  void Mouse(int x, int y, int z, unsigned int button) {
    for (auto i : con) {
      i->Mouse(x, y, z, button);
    }

  }
  void Open() {
    is_open = true;
    gf = gh::CreateGraphicsD3D();
    gf->InitializeFromViewport(vp);
    gf->SetBgColor(0xff444444);
    if (setup) {
      setup();
    }
  }
  void Close() {
    is_close = true;
  }
  void Resize(int w, int h) {
  }
  bool is_open = false;
  bool is_close = false;
  gh::Graphics* gf;
  gh::Viewport* vp;
  std::function<void()> setup;
};

struct Scene1 :public Controller {
  int c = 0;
  int uc = 0;
  Graphics* gf;
  std::vector<struct Render*> ren;
  Node root;
  FovCamera cam;
  bool ltouch = false;
  float forward = 0;
  float yaw = PI / 2, pitch = 0;
  float delta_x = 0, delta_y = 0;
  HexBlock b;

  Scene1() {
    cam.from = { 10, 5, -20 };
    cam.at = { 0, 0, 0 };
    cam.up = { 0, 1, 0 };
    cam.near_clip = 1;
    cam.far_clip = 100;
    cam.fov = PI / 3;
    cam.aspect = 9.0f / 16.0f;

    root.Append(&cam);

    b.x = 0;
    b.y = 0;
    b.bottom = 0;
    b.top = 5;

    root.Append(&b);

    for (int i = 0; i < 100; i++) {
      const char* trees[] = { "tree_blocks.glb",
        "tree_blocks_dark.glb",
        "tree_blocks_fall.glb",
        "tree_cone.glb",
        "tree_cone_dark.glb",
        "tree_cone_fall.glb",
        "tree_default.glb",
        "tree_default_dark.glb",
        "tree_default_fall.glb",
        "tree_detailed.glb",
        "tree_detailed_dark.glb",
        "tree_detailed_fall.glb",
        "tree_fat.glb",
        "tree_fat_darkh.glb",
        "tree_fat_fall.glb",
        "tree_oak.glb",
        "tree_oak_dark.glb",
        "tree_oak_fall.glb",
        "tree_palm.glb",
        "tree_palmBend.glb",
        "tree_palmDetailedShort.glb",
        "tree_palmDetailedTall.glb",
        "tree_palmShort.glb",
        "tree_palmTall.glb",
        "tree_pineDefaultA.glb",
        "tree_pineDefaultB.glb",
        "tree_pineGroundA.glb",
        "tree_pineGroundB.glb",
        "tree_pineRoundA.glb",
        "tree_pineRoundB.glb",
        "tree_pineRoundC.glb",
        "tree_pineRoundD.glb",
        "tree_pineRoundE.glb",
        "tree_pineRoundF.glb",
        "tree_pineSmallA.glb",
        "tree_pineSmallB.glb",
        "tree_pineSmallC.glb",
        "tree_pineSmallD.glb",
        "tree_pineTallA.glb",
        "tree_pineTallA_detailed.glb",
        "tree_pineTallB.glb",
        "tree_pineTallB_detailed.glb",
        "tree_pineTallC.glb",
        "tree_pineTallC_detailed.glb",
        "tree_pineTallD.glb",
        "tree_pineTallD_detailed.glb",
        "tree_plateau.glb",
        "tree_plateau_dark.glb",
        "tree_plateau_fall.glb",
        "tree_simple.glb",
        "tree_simple_dark.glb",
        "tree_simple_fall.glb",
        "tree_small.glb",
        "tree_small_dark.glb",
        "tree_small_fall.glb",
        "tree_tall.glb",
        "tree_tall_dark.glb",
        "tree_tall_fall.glb",
        "tree_thin.glb",
        "tree_thin_dark.glb",
        "tree_thin_fall.glb" };
      DocMesh* m = new DocMesh();
      m->path = "/model/kenny_nature/";
      m->path += trees[i % (sizeof(trees) / sizeof(*trees))];
      m->pose.Identity();
      m->pose.Scale(-1, 1, 1);
      m->pose.RotateY(i / 10.0f);
      m->pose.Translate((i % 10) * 2 - 10, 0, (i / 10) * 2 - 10);
      root.Append(m);
    }

  }

  void Update() {
    if (0 == c) {
      ren.push_back(new DocRender(gf));
    }

    if (ltouch) {

    }
    Vector3 eye;
    eye.x = cosf(yaw) * cosf(pitch);
    eye.y = sinf(pitch);
    eye.z = sinf(yaw) * cosf(pitch);
    if (fabsf(forward) < 0.01f) {
      forward = 0;
    }
    else {
      Vector3 f = eye;
      f.y *= fabsf(f.y);
      cam.from += f.Normalize() * forward * 0.1f;
      forward *= 0.9f;
    }
    cam.at = cam.from + eye * 10;

    c++;
  }
  void Draw() {
    if (0 == uc) {
    }

    _(ren).each([&](Render* a) {
      a->Draw(&root);
      });
    uc++;
  }

  int Mouse(int x, int y, int z, unsigned int button) {
    if (button & ViewportMessage::MOUSE_L) {
      ltouch = true;
    }
    else {
      ltouch = false;
    }
    if (x) {
      if (0 == delta_x) {
        delta_x = x;
      }
      if (ltouch)
        yaw += (x - delta_x) / -200.0f;
      delta_x = x;
    }
    else {
      delta_x = 0;
    }
    if (y) {
      if (0 == delta_y) {
        delta_y = 0;
      }
      if (ltouch)
        pitch += (y - delta_y) / -200.0f;
      if (pitch > PI / 2 - 0.01f) {
        pitch = PI / 2 - 0.01f;
      }
      if (pitch < -PI / 2 + 0.01f) {
        pitch = -PI / 2 + 0.01f;
      }

      delta_y = y;
    }
    else {
      delta_y = 0;
    }
    if (0 < z) {
      forward += 1.0f;
    }
    else if (0 > z) {
      forward -= 1.0f;
    }
    return 0;
  }
};

int main(int, char**)
{
  VP m;
  ViewportInitializeParameter p;
  strcpy(p.name, "game");
  Fs* f = gh::Fs::Create();
  f->FetchFromFile("/font/mplus-1mn-bold.ttf");
  Fontmap* fm = Fontmap::Create(f->Data("/font/mplus-1mn-bold.ttf"), f->Length("/font/mplus-1mn-bold.ttf"), 48);

  Scene1 scn;

  ManagedGraphics* mg;
  m.setup = [&]() {
    fm->Init(m.gf);
    scn.gf = m.gf;
  };

  con.push_back(&scn);

  m.vp = CreateViewport(p, &m);
  while (!m.is_open) {
    m.vp->MessageFetch();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  int count = 0;
  while (!m.is_close) {
    m.vp->MessageFetch();
    count++;
    _(con).each([](Controller* c) {
      c->Update();
      });
    m.gf->clear();
    m.gf->BeginScene();
    Matrix mat;

    _(con).each([](Controller* c) {
      c->Draw();
      });
    mat.Identity();
    mat.Scale(10);
    m.gf->EndScene();
    m.gf->Flip(m.vp);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  DeleteViewport(m.vp);
}

