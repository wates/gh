#ifndef UNDERSCORE_H_INCLUDED
#define UNDERSCORE_H_INCLUDED

#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <locale.h>
#include <functional>
#if defined(ANDROID)
#include <sstream>
#endif

#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)
#endif

static struct underscore {
  template<typename T, typename B>
  static T& push(T &base, const B& append, std::size_t append_length = 1) {
    base.insert(base.begin(), append, append + append_length);
    return base;
  }
  template<typename T>
  static typename T::value_type& push(T &base) {
    base.resize(base.size() + 1);
    return base.back();
  }
  template<typename T>
  static T& increase(T &base) {
    base.resize(base.size() + 1);
    return base;
  }
  template<typename T>
  static T& decrease(T&base) {
    base.resize(base.size() - 1);
    return base;
  }
  template<typename T, typename B>
  static bool has(const T&base, const B& val) {
    return base.end() != std::find(base.begin(), base.end(), val);
  }
  template<typename K, typename V>
  static bool has(const std::map<K, V> &a, const K&b) {
    return a.end() != a.find(b);
  }
  template<typename T, typename B>
  static B* find(T&base, const B& val) {
    auto i = std::find(base.begin(), base.end(), val);
    if (i != base.end()) {
      return &*i;
    }
    return nullptr;
  }
  template<typename T, typename B>
  static bool find_and_remove(T&base, const B& val) {
    auto i = std::find(base.begin(), base.end(), val);
    if (i != base.end()) {
      base.erase(i);
      return true;
    }
    return false;
  }
  template<typename T>
  static T& splice(T&base, int start, int num = 1) {
    base.erase(base.begin() + start, base.begin() + start + num);
    return base;
  }
  template<typename T, typename A>
  static size_t indexOf(const T&base, const A&a) {
    size_t n = 0;
    for (auto &i : base) {
      if (i == a)return n;
      n++;
    }
    return -1;
  }
  std::string toHex(std::vector<uint8_t> buf) {
    static const char kHex[] = "0123456789ABCDEF";
    std::string r;
    for (auto i : buf) {
      r += kHex[i / 16];
      r += kHex[i % 16];
    }
    return r;
  }
  template<typename T>
  struct ext_string :
    public std::basic_string<T> {
    typedef typename std::basic_string<T> MyT;
    MyT &src_;
    ext_string(MyT &src)
      :src_(src) {}
    ext_string& operator<<(const T *a) {
      src_ += a;
      return *this;
    }
    ext_string& operator<<(const MyT &a) {
      src_ += a;
      return *this;
    }
    ext_string& operator>>(const MyT &a) {
      if (src_.size() > a.size()) {
        src_.erase(src_.end() - a.size(), src_.end());
      }
      src_ += a;
      return *this;
    }
    static void tostring(ext_string<char>&a, int b) {
#if defined(ANDROID)
      std::stringstream ss;
      ss << b;
      a.src_ += ss.str();

#else
      a.src_ += std::to_string(b);
#endif
    }
    static void tostring(ext_string<wchar_t>&a, int b) {
#if defined(ANDROID)
      std::wstringstream ss;
      ss << b;
      a.src_ += ss.str();

#else
      a.src_ += std::to_wstring(b);
#endif
    }
    static void tostring(ext_string<char>&a, float b) {
#if defined(ANDROID)
      std::stringstream ss;
      ss << b;
      a.src_ += ss.str();

#else
      a.src_ += std::to_string(b);
#endif
    }
    static void tostring(ext_string<wchar_t>&a, float b) {
#if defined(ANDROID)
      std::wstringstream ss;
      ss << b;
      a.src_ += ss.str();

#else
      a.src_ += std::to_wstring(b);
#endif
    }
    ext_string& operator<<(int a) {
      tostring(*this, a);
      return *this;
    }
    ext_string& operator<<(float a) {
      tostring(*this, a);
      return *this;
    }
    static void assign(ext_string<char>&a, int b) {
      a += std::to_string(b);
    }
    static void assign(ext_string<wchar_t>&a, const std::string &b) {
      wchar_t *wc = new wchar_t[b.length() + 1];
      std::mbstowcs(wc, b.c_str(), b.length() + 1);
      a.src_ = wc;
      delete[]wc;
    }
    static void assign(ext_string<char>&a, const std::wstring &b) {
      setlocale(LC_ALL, "ja-JP");
      char *wc = new char[b.length() * 4 + 1];
      std::wcstombs(wc, b.c_str(), b.length() * 4 + 1);
      a.src_ = wc;
      delete[]wc;
    }
    template<typename K>
    ext_string& operator=(const K &a) {
      assign(*this, a);
      return *this;
    }
  };

  template<typename T>
  ext_string<T> operator()(std::basic_string<T> &str) {
    return ext_string<T>(str);
  }

  template<typename T>
  struct ext_vector :
    public std::vector<T> {
    typedef typename std::vector<T> MyT;
    MyT& src_;
    ext_vector(MyT& src)
      :src_(src) {}
    void each(std::function<void(T)> f) {
      std::for_each(src_.begin(), src_.end(), f);
    }
  };

  template<typename T>
  ext_vector<T> operator()(std::vector<T>& str) {
    return ext_vector<T>(str);
  }
}_;

#endif