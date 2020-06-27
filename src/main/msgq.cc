#include "msgq.h"

#include <thread>
#include <mutex>

namespace gh
{
  struct MessageQueueImpl
    :public MessageQueue
  {
    enum
    {
      CMD_OPEN = 1,
      CMD_CLOSE,
      CMD_TRANSFER
    };
    MessageQueueImpl();
    ~MessageQueueImpl();

    void Fetch();
    inline void SetNext(Endpoint* p) { next_ = p; }
    inline Endpoint* Next() { return next_; }

    void Open();
    void Close();
    int Transfer(const uint8_t* buffer, int length);

    bool Lock(uint8_t** buffer, int length);
    void Unlock();

    Endpoint* next_;
    std::mutex m;
    ByteQueue queue_;
    std::vector<uint8_t> command_buffer_;
  };

  MessageQueue* MessageQueue::Create()
  {
    return new MessageQueueImpl();
  }

  MessageQueueImpl::MessageQueueImpl()
  {
  }

  MessageQueueImpl::~MessageQueueImpl()
  {
  }

  void MessageQueueImpl::Open()
  {
    std::lock_guard<std::mutex> lk(m);
    uint8_t command = CMD_OPEN;
    queue_.Push(&command, sizeof(command));
  }

  void MessageQueueImpl::Close()
  {
    std::lock_guard<std::mutex> lk(m);
    uint8_t command = CMD_CLOSE;
    queue_.Push(&command, sizeof(command));
  }

  bool MessageQueueImpl::Lock(uint8_t** buffer, int length)
  {
    m.lock();
    uint8_t command = CMD_TRANSFER;
    queue_.Push(&command, sizeof(command));
    queue_.Push(reinterpret_cast<const uint8_t*>(&length), sizeof(length));
    queue_.Reserve(buffer, length);
    return true;
  }

  void MessageQueueImpl::Unlock()
  {
    m.unlock();
  }

  int MessageQueueImpl::Transfer(const uint8_t* buffer, int length)
  {
    std::lock_guard<std::mutex> lk(m);
    uint8_t command = CMD_TRANSFER;
    queue_.Push(&command, sizeof(command));
    queue_.Push(reinterpret_cast<const uint8_t*>(&length), sizeof(length));
    queue_.Push(buffer, length);
    return length;
  }

  void MessageQueueImpl::Fetch()
  {
    for (;;)
    {
      uint8_t command = 0;
      int length = 0;
      {
        std::lock_guard<std::mutex> lk(m);
        if (queue_.Size() > 0)
        {
          command = *queue_.Data();
          queue_.Shrink(1);

          if (CMD_TRANSFER == command)
          {
            if (queue_.Size() < (int)sizeof(length))
            {
              //critical error
            }
            length = *reinterpret_cast<const int*>(queue_.Data());
            if (queue_.Size() < (int)sizeof(length) + length)
            {
              //critical error
            }
            if (command_buffer_.size() < length)
            {
              command_buffer_.resize(length);
            }
            memcpy(command_buffer_.data(), queue_.Data() + sizeof(length), length);
            queue_.Shrink(sizeof(length) + length);
          }
        }
      }
      if (command)
      {
        if (CMD_OPEN == command)
        {
          Next()->Open();
        }
        else if (CMD_CLOSE == command)
        {
          Next()->Close();
        }
        else if (CMD_TRANSFER == command)
        {
          Next()->Transfer(command_buffer_.data(), length);
        }
        continue;
      }
      break;
    }
  }
}
