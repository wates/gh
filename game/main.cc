#include "sys/viewport.h"
#include "gfx/graphics.h"
#include "gfx/fontmap.h"
#include "sys/fs.h"

#include <thread>
#include <chrono>
#include <functional>
using namespace gh;

struct VP :public ViewportMessage {
  void CreateFail() {

  }
  void CloseButton() {
    is_close = true;
  }
  void Mouse(int x, int y, int z, unsigned int button) {

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


int main(int, char**)
{
  VP m;
  ViewportInitializeParameter p;
  strcpy(p.name, "game");
  Fs* f = gh::Fs::Create();
  f->FetchFromFile("/font/mplus-1mn-bold.ttf");
  Fontmap* fm = Fontmap::Create(f->Data("/font/mplus-1mn-bold.ttf"), f->Length("/font/mplus-1mn-bold.ttf"), 32);

  m.setup = [&]() {
    fm->Init(m.gf);
  };

  m.vp = CreateViewport(p, &m);
  int count = 0;
  while (!m.is_close) {
    m.vp->MessageFetch();
    if (m.is_open) {
      count++;
      m.gf->clear();
      m.gf->BeginScene();
      Matrix mat;
      mat.Identity();
      mat.Scale(0.001f);
      fm->DrawColor(mat, L"abcdEFG", 0xff44ffee);
      m.gf->EndScene();
      m.gf->Flip(m.vp);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  DeleteViewport(m.vp);
}

