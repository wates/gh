

#include <gtest/gtest.h>

#include "../src/sys/viewport.h"

#include <thread>
#include <chrono>
using namespace gh;

TEST(viewport, init)
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

    }
    void Close() {
      is_close = true;
    }
    void Resize(int w, int h) {

    }
    bool is_open = false;
    bool is_close = false;
  }m;

  ViewportInitializeParameter p;
  strcpy(p.name, "test");

  auto vp = CreateViewport(p, &m);
  int count = 0;
  while (!m.is_close) {
    vp->MessageFetch();
    if (m.is_open) {
      count++;
      if (count > 1000) {
        vp->Close();

      }

    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  DeleteViewport(vp);
}
