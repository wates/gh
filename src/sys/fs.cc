#include "fs.h"
#include <map>
#include <filesystem>
#include <fstream>
using namespace std;

namespace gh {
  struct FsImpl
    :public Fs {

    struct File {
      LoadStatus state;
      vector<unsigned char>* buffer;
    };

    std::map<std::string, File> vfs;
    std::string root;

    void FetchFromFile(const std::string& path, const std::string& alias) {
      ifstream in(root + path, ios::in | ios::binary);
      File f;
      if (in.is_open()) {
        f.buffer = new vector<unsigned char>(std::istreambuf_iterator<char>(in), {});
        f.state = Complete;
      }
      else {
        f.buffer = nullptr;
        f.state = NotExist;
      }
      vfs[alias == "" ? path : alias] = f;
    }
    LoadStatus Status(const std::string& alias) {
      return vfs[alias].state;
    }
    int Length(const std::string& alias) {
      return vfs[alias].buffer->size();
    }
    const void* Data(const std::string& alias) {
      return vfs[alias].buffer->data();
    }

    FsImpl() {
      root = ".";
      string base = ".";
      bool found = false;
      int upper = 0;
      while (!found && upper < 5) {
        for (auto& p : std::filesystem::directory_iterator(base)) {
          if (p.path().filename() == "gh_assets") {
            root = base + "/" + p.path().filename().string();
            found = true;
            break;
          }
        }
        base += "/..";
        upper++;
      }
    }
    ~FsImpl() {
      for (auto& p : vfs) {
        if (p.second.buffer) {
          delete p.second.buffer;
        }
      }
    }
  };

  Fs* Fs::Create() {
    return new FsImpl;
  }
}