#ifndef FS_H_INCLUDED
#define FS_H_INCLUDED

#include <string>

namespace gh {
  struct Fs {
    enum LoadStatus {
      NotExist,
      Loading,
      Complete
    };
    virtual void FetchFromFile(const std::string& path, const std::string& alias = "") = 0;
    virtual LoadStatus Status(const std::string& alias) = 0;
    virtual int Length(const std::string& alias) = 0;
    virtual const void* Data(const std::string& alias) = 0;
    virtual ~Fs();

    static Fs* Create();
  };
}

#endif // !FS_H_INCLUDED
