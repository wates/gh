

#include <gtest/gtest.h>

#include "../src/sys/viewport.h"

#include <thread>
#include <chrono>
using namespace gh;

TEST(viewport, init)
{
  struct TestVP :public ViewportMessage {
    void CreateFail() {

    }
    void CloseButton() {
      is_close = true;
    }
    void Mouse(int x, int y, int z, unsigned int button) {

    }
    void Open() {

    }
    void Close() {
      is_close = true;
    }
    void Resize(int w, int h) {

    }
    bool is_close = false;
  }test1;

  ViewportInitializeParameter p;
  strcpy(p.name, "test");

  auto vp = CreateViewport(p, &test1);
  while (!test1.is_close) {
    vp->MessageFetch();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
