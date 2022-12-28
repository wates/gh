#include "sys/viewport.h"
#include "sys/fs.h"
#include "main/underscore.h"

#include <thread>
#include <chrono>
#include <functional>
#include <vector>
using namespace gh;

struct VP :public ViewportMessage {
  void CreateFail() {

  }
  void CloseButton() {
    is_close = true;
  }
  void Mouse(int x, int y, int z, unsigned int button) {
    if (button & ViewportMessage::MOUSE_L) {
      //ltouch = true;
    }
    else {
      //ltouch = false;
    }
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
  gh::Viewport* vp;
};

#include <Windows.h>
#include <WinUser.h>

int main(int, char**)
{
  VP m;
  ViewportInitializeParameter p;
  p.using_framebuffer = true;

  m.vp = CreateViewport(p, &m);
  while (!m.is_open) {
    m.vp->MessageFetch();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  while (!m.is_close) {
    m.vp->MessageFetch();

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  DeleteViewport(m.vp);
}

