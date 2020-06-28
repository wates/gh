#include <gtest/gtest.h>

#include "../src/sys/fs.h"

#include <thread>
#include <chrono>

using namespace gh;

TEST(fs, init)
{
  Fs* f = gh::Fs::Create();


  delete f;
}

TEST(fs, read)
{
  Fs* f = gh::Fs::Create();

  f->FetchFromFile("/font/mplus-1mn-bold.ttf");
  EXPECT_EQ(Fs::Complete, f->Status("/font/mplus-1mn-bold.ttf"));
  delete f;
}
