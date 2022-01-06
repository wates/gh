#include <set>
#include <random>

template<typename T>
struct UID_ {
  const T id;
  UID_(T a) :id(a) {}
  UID_() = delete;
  //UID_(const UID_&) = delete;
  //void operator=(const UID_&) = delete;
};

template<typename T, class Engine = std::minstd_rand>
struct IdGenerator {
  std::set<T> get;
  Engine engine;
  const UID_<T> Generate() {
    T n;
    do {
      n = engine();
    } while (get.find(n) != get.end());
    get.insert(n);
    return UID_<T>(n);
  }
  void Release(UID_<T> u) {
    //get.erase(get.find(u.id));
  }
};

template<typename Generator>
struct IdTypeGenerator {
  static IdGenerator<std::uint32_t> gen;
};

template <typename Type, typename T = std::uint32_t>
struct TUID : public UID_<T> {
  TUID() :UID_<T>(IdTypeGenerator<Type>::gen.Generate()) {
  }
  ~TUID() {
    IdTypeGenerator<Type>::gen.Release(UID_<T>::id);
  }
};
