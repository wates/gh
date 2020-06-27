
#ifndef OBSERVER_H_INCLUDED
#define OBSERVER_H_INCLUDED

namespace gh
{
  struct Subject
  {
    virtual void Update(int something) = 0;
    virtual ~Subject();
  };

  struct Observer
  {
    virtual bool Append(Subject* any) = 0;
    virtual bool Remove(Subject* any) = 0;
    virtual void Update(int something) = 0;
    virtual ~Observer();
  };

  Observer* CreateBasicObserver();
  void DeleteBasicObserver(Observer* obs);
}

#endif

