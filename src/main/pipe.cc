#include "pipe.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace wts
{
  const char* memmem(const char* p, int sz_p, const char* cmp, int sz_cmp)
  {
    if (sz_p < sz_cmp)
      return 0;
    for (int sz = 0; sz <= sz_p - sz_cmp; sz++)
    {
      if (*((char*)p + sz) == *(char*)cmp &&
        memcmp((char*)p + sz, cmp, sz_cmp) == 0)
        return (char*)p + sz;
    }
    return NULL;
  }

}


//////////////////////////////////
// binary print

namespace gh
{

  void BinaryPrint::Open()
  {
    printf("%08p Open\n", this);
  }

  void BinaryPrint::Close()
  {
    printf("%08p Close\n", this);
  }

  int BinaryPrint::Transfer(const uint8_t* buffer, int length)
  {
    printf("%08p length= 0x%X ( %d bytes)\n", this, length, length);
    const unsigned char* p = (const unsigned char*)buffer;
    for (int i = 0; i <= (length) / 16; i++)
    {
      int left = length - i * 16;
      if (left > 16)
        left = 16;

      //hex
      printf("%04X: ", i * 16);

      for (int j = 0; j < 16; j++)
      {
        if (j == 8)
          printf(" ");

        if (j < left)
        {
          printf("%02X ", p[i * 16 + j]);
        }
        else
        {
          printf("   ");
        }
      }

      //ascii
      char ascii[17];
      for (int j = 0; j < 16; j++)
      {
        if (j < left)
        {
          ascii[j] = p[i * 16 + j];
        }
        else
        {
          ascii[j] = 0x20;
        }

        //replace command char
        if (ascii[j] < 0x20)
          ascii[j] = ' ';
      }
      ascii[16] = '\0';

      printf("  :  %16s\n", ascii);
    }
    return length;
  }

  ByteQueue::ByteQueue(int length, int capacity)
    :buffer_(new uint8_t[length])
    , length_(length)
    , capacity_(capacity)
    , start_(0)
    , end_(0)
  {
  }

  ByteQueue::~ByteQueue()
  {
    delete[]buffer_;
  }

  int ByteQueue::Size()
  {
    return end_ - start_;
  }

  bool ByteQueue::Reserve(uint8_t** pointer, int len)
  {
    if (Size() + len < length_)
    {
      if (end_ + len > length_)
      {
        memmove(buffer_, buffer_ + start_, Size());
        end_ -= start_;
        start_ = 0;
      }
      *pointer = buffer_ + end_;
      end_ += len;
    }
    else if (Size() + len < capacity_)
    {
      length_ = length_ + len * 2;
      uint8_t* temp = new uint8_t[length_];
      memcpy(temp, buffer_ + start_, Size());
      *pointer = temp + Size();
      end_ = Size() + len;
      start_ = 0;
      delete[]buffer_;
      buffer_ = temp;
    }
    else
    {
      return false;
    }
    return true;
  }

  bool ByteQueue::Push(const uint8_t* data, int len)
  {
    if (Size() + len < length_)
    {
      if (end_ + len > length_)
      {
        memmove(buffer_, buffer_ + start_, Size());
        end_ -= start_;
        start_ = 0;
      }
      memcpy(buffer_ + end_, data, len);
      end_ += len;
    }
    else if (Size() + len < capacity_)
    {
      length_ = length_ + len * 2;
      uint8_t* temp = new uint8_t[length_];
      memcpy(temp, buffer_ + start_, Size());
      memcpy(temp + Size(), data, len);
      end_ = Size() + len;
      start_ = 0;
      delete[]buffer_;
      buffer_ = temp;
    }
    else
    {
      return false;
    }
    return true;
  }

  void ByteQueue::Shrink(int len)
  {
    start_ += len;
    if (start_ == end_)
      start_ = end_ = 0;
  }

  uint8_t* ByteQueue::Data()
  {
    return buffer_ + start_;
  }

  Buffer::Buffer()
  {
    close_flag_ = false;
    enable_ = false;
  }

  void Buffer::Open()
  {
    close_flag_ = false;
    enable_ = true;
    if (Next())
      Next()->Open();
  }

  void Buffer::Close()
  {
    close_flag_ = true;
  }

  int Buffer::Transfer(const uint8_t* buffer, int length)
  {
    que_.Push(buffer, length);
    //int td=next->Transfer(que.data(),que.size());
    //que.shrink(td);
    return length;
  }

  void Buffer::Update(int something)
  {
    if (enable_ && que_.Size() && Next())
    {
      int td = Next()->Transfer(que_.Data(), que_.Size());
      que_.Shrink(td);
    }
    if (enable_ && close_flag_ && 0 == que_.Size())
    {
      enable_ = false;
      if (Next())
        Next()->Close();
    }
  }
}
