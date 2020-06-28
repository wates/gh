#include <string>

namespace gh {

  struct ViewportInitializeParameter
  {
    char name[64] = { "a" };
    int width = 1280;
    int height = 720;
    bool full_screen = false;
    bool fixed_resolution = false;
    bool layered = false;
  };

  struct ViewportMessage
  {
    static const unsigned int MOUSE_L = 1;
    static const unsigned int MOUSE_R = 2;
    static const unsigned int MOUSE_M = 4;

    virtual void CreateFail() = 0;
    virtual void CloseButton() = 0;
    virtual void Mouse(int x, int y, int z, unsigned int button) = 0;
    virtual void Open() = 0;
    virtual void Close() = 0;
    virtual void Resize(int w, int h) = 0;

    inline virtual ~ViewportMessage() {};
  };

  struct Viewport
  {
    virtual void Close() = 0;
    virtual void* GetWindowHandle() = 0;
    virtual void MessageFetch() = 0;
    virtual int Width()const = 0;
    virtual int Height()const = 0;
    virtual void SetSize(int w, int h) = 0;

    inline virtual ~Viewport() {};
  };

  Viewport* CreateViewport(const ViewportInitializeParameter& ip, ViewportMessage* cb);
  void DeleteViewport(Viewport* vp);

}
