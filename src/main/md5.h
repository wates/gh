
#ifndef WTS_MD5_H_
#define WTS_MD5_H_

#include <stdint.h>

namespace gh
{
  class MD5
  {
  public:
    MD5();

    void Update(const void* buffer, int length);
    void Final();
    const uint8_t* Result()const;
    const char* Hex();
  private:
    char context_[112];
    uint8_t result_[16];
    char hex_[33];
  };

}

#endif
