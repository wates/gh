#include <gtest/gtest.h>

#include "../src/sys/viewport.h"
#include "../src/gfx/graphics.h"

#include <thread>
#include <chrono>
using namespace gh;

TEST(gfx, init)
{
  struct Msg :public ViewportMessage {
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
    }
    void Close() {
      is_close = true;
    }
    void Resize(int w, int h) {
    }
    bool is_open = false;
    bool is_close = false;
    gh::Graphics *gf;
    gh::Viewport* vp;
  }m;

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
