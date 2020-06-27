
#include "observer.h"
#include "underscore.h"
#include <vector>
namespace gh
{
  Subject::~Subject()
  {
  }

  Observer::~Observer()
  {
  }

  struct BasicObserver
    :public Observer
  {
    bool Append(Subject* any);
    bool Remove(Subject* any);
    void Update(int something);

    std::vector<Subject*> subject_;
  };

  bool BasicObserver::Append(Subject* any)
  {
    subject_.push_back(any);
    return true;
  }

  bool BasicObserver::Remove(Subject* any)
  {
    return _.find_and_remove(subject_, any);
  }

  void BasicObserver::Update(int something)
  {
    auto cpy = subject_;
    for (auto& i : cpy) {
      i->Update(something);
    }
  }

  Observer* CreateBasicObserver()
  {
    return new BasicObserver();
  }

  void DeleteBasicObserver(Observer* obs)
  {
    delete static_cast<BasicObserver*>(obs);
  }

}

