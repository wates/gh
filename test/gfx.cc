#include <gtest/gtest.h>

#include "../src/sys/viewport.h"
#include "../src/gfx/graphics.h"
#include "../src/gfx/fontmap.h"

#include <thread>
#include <chrono>
using namespace gh;

struct Basic :public ViewportMessage {
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

/*
TEST(gfx, init)
{
  Basic m;
  ViewportInitializeParameter p;
  strcpy(p.name, "test");

  m.vp = CreateViewport(p, &m);
  int count = 0;
  while (!m.is_close) {
    m.vp->MessageFetch();
    if (m.is_open) {
      count++;
      if (count > 1000) {
        m.vp->Close();

      }
      m.gf->clear();
      m.gf->Flip(m.vp);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  DeleteViewport(m.vp);
}
*/
#include "../src/sys/fs.h"

TEST(gfx, font)
{
  Basic m;
  ViewportInitializeParameter p;
  strcpy(p.name, "test");
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
      if (count > 1000) {
        m.vp->Close();

      }
      m.gf->clear();
      m.gf->BeginScene();
      Matrix mat;
      mat.Identity();
      mat.Scale(0.001f);
      fm->DrawColor(mat, L"abcdEFG", 0xffaaffee);
      m.gf->EndScene();
      m.gf->Flip(m.vp);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  DeleteViewport(m.vp);
}

