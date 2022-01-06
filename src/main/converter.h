
#ifndef GH_CONVERTER_H_
#define GH_CONVERTER_H_

#include <vector>
#include <map>
#include <string>
#include <assert.h>

namespace gh
{
  ///////////////////////////////
  // for POD

  template<typename T>
  inline bool Convert(T &c, char &v, const char *info)
  {
    return ConvertPOD(c, v, info);
  }
  template<typename T>
  inline bool Convert(T &c, unsigned char &v, const char *info)
  {
    return ConvertPOD(c, v, info);
  }
  template<typename T>
  inline bool Convert(T &c, short &v, const char *info)
  {
    return ConvertPOD(c, v, info);
  }
  template<typename T>
  inline bool Convert(T &c, unsigned short &v, const char *info)
  {
    return ConvertPOD(c, v, info);
  }
  template<typename T>
  inline bool Convert(T &c, int &v, const char *info)
  {
    return ConvertPOD(c, v, info);
  }
  template<typename T>
  inline bool Convert(T &c, unsigned int &v, const char *info)
  {
    return ConvertPOD(c, v, info);
  }
  template<typename T>
  inline bool Convert(T &c, float &v, const char *info)
  {
    return ConvertPOD(c, v, info);
  }

  ///////////////////////////////////////
  // Binary Writer

  struct BinaryWriter
  {
    std::vector<char> output;
  };

  template<typename T>
  inline bool ConvertPOD(BinaryWriter &c, T &v, const char *info)
  {
    if (2 == sizeof(T))
    {
      uint16_t val;
      val = (static_cast<uint16_t>(reinterpret_cast<const uint8_t*>(&v)[0]));
      val |= (static_cast<uint16_t>(reinterpret_cast<const uint8_t*>(&v)[1])) << 8;
      c.output.insert(c.output.end(),(char*)&val, ((char*)&val) + sizeof(T));
    }
    else if (4 == sizeof(T))
    {
      uint32_t val;
      val = (static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(&v)[0]));
      val |= (static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(&v)[1])) << 8;
      val |= (static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(&v)[2])) << 16;
      val |= (static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(&v)[3])) << 24;
      c.output.insert(c.output.end(), (char*)&val, ((char*)&val)+sizeof(T));
    }
    else if (1 == sizeof(T))
    {
      c.output.insert(c.output.end(),(char*)&v, ((char*)&v)+sizeof(T));
    }
    else
    {
      //*((int*)0) = 0;
      assert(false);
    }
    return true;
  }
  template<typename ArrayType>
  inline bool Convert(BinaryWriter &c, std::basic_string<ArrayType> &v, const char *info)
  {
    typedef ArrayType Char;
    c.output.insert(c.output.end(),(char*)v.data(),((char*)v.data())+ sizeof(Char)*(v.size() + 1));
    return true;
  }

  template<typename T>
  inline bool Convert(BinaryWriter &c, const std::vector<T> &v, const char *info)
  {
    unsigned int count = v.size();
    if (!Convert(c, count, info))
      return false;
    for (auto i : v)
      if (!Convert(c, i, info))
        return false;
    return true;
  }

  template<typename T, typename TypeSize>
  inline bool Convert(BinaryWriter &c, std::vector<T, TypeSize> &v, const char *info)
  {
    TypeSize count = v.size();
    if (!Convert(c, count, info))
      return false;
    for (TypeSize i = 0; i < count; i++)
      if (!Convert(c, v[i], info))
        return false;
    return true;
  }

  inline bool ObjectStart(BinaryWriter &w, const char *info)
  {
    return true;
  }

  inline bool ObjectEnd(BinaryWriter &w)
  {
    return true;
  }

  ////////////////////////////////
  // Binary Reader

  struct BinaryReader
  {
    const char *position;
    const char *end;
  };

  template<typename T>
  inline bool ConvertPOD(BinaryReader &c, T &v, const char *info)
  {
    if (c.position + sizeof(T) > c.end)
      return false;
#ifdef CONVERTER_LIST_NAMES
    if(typelist.find(typeid(T).name())==typelist.end())
      typelist[typeid(T).name()]=0;
    typelist[typeid(T).name()]++;
#endif

#if 1 
    if (2 == sizeof(T))
    {
      *reinterpret_cast<uint16_t*>(&v) = (static_cast<uint16_t>(reinterpret_cast<const uint8_t*>(c.position)[0]));
      *reinterpret_cast<uint16_t*>(&v) |= (static_cast<uint16_t>(reinterpret_cast<const uint8_t*>(c.position)[1])) << 8;
    }
    else if (4 == sizeof(T))
    {
      *reinterpret_cast<uint32_t*>(&v) = (static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(c.position)[0]));
      *reinterpret_cast<uint32_t*>(&v) |= (static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(c.position)[1])) << 8;
      *reinterpret_cast<uint32_t*>(&v) |= (static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(c.position)[2])) << 16;
      *reinterpret_cast<uint32_t*>(&v) |= (static_cast<uint32_t>(reinterpret_cast<const uint8_t*>(c.position)[3])) << 24;
    }
    else
    {
      v = *(T*)c.position;
    }
#else
    v=*(T*)c.position;
#endif
    c.position += sizeof(T);
    return true;
  }

  //template<typename T>
  //inline bool Convert(BinaryReader &c,Array<T> &v,const char *info)
  //{
  //	unsigned int count;
  //	if(!Convert(c,count,info))
  //		return false;
  //	v.Resize(count);
  //	for(unsigned int i=0;i<count;i++)
  //		if(!Convert(c,v[i],info))
  //			return false;
  //	return true;
  //}
  template<typename T,typename SizeType>
  struct type_size_vector :public std::vector<T> {
  };

  template<typename T,typename SizeType>
  inline bool Convert(BinaryReader &c, type_size_vector<T,SizeType> &v, const char *info)
  {
    SizeType count;
    if (!Convert(c, count, info))
      return false;
    v.resize(count);
    for (uint32_t i = 0; i < count; i++)
      if (!Convert(c, v[i], info))
        return false;
    return true;
  }

  template<typename T>
  inline bool Convert(BinaryReader &c, std::vector<T> &v, const char *info)
  {
    uint32_t count;
    if (!Convert(c, count, info))
      return false;
    v.resize(count);
    for (uint32_t i = 0; i < count; i++)
      if (!Convert(c, v[i], info))
        return false;
    return true;
  }
  //template<typename T, typename TypeSize>
  //inline bool Convert(BinaryReader &c, Array<T, TypeSize> &v, const char *info)
  //{
  //  TypeSize count;
  //  if (!Convert(c, count, info))
  //    return false;
  //  v.Resize(count);
  //  for (TypeSize i = 0; i < count; i++)
  //    if (!Convert(c, v[i], info))
  //      return false;
  //  return true;
  //}

  template<typename ArrayType>
  inline bool Convert(BinaryReader &c, std::basic_string<ArrayType> &v, const char *info)
  {
    typedef ArrayType Char;
    Char *begin = (Char*)c.position;
    Char *delim = (Char*)c.position;
    Char *end = (Char*)c.end;
    while (delim < end)
    {
      if (0 == *delim)
      {
        v = begin;
        c.position = (char*)(delim + 1);
        return true;
      }
      ++delim;
    }
    return false;
  }

  inline bool ObjectStart(BinaryReader &w, const char *info)
  {
    return true;
  }

  inline bool ObjectEnd(BinaryReader &w)
  {
    return true;
  }

  template<typename Z, uint32_t N, typename T>
  inline bool Convert(Z &c, T(&v)[N], const char *info)
  {
    for (uint32_t i = 0; i < N; i++)
      if (!Convert(c, v[i], info))
        return false;
    return true;
  }
  

}

#define CONVERT_OBJECT_0(type)\
namespace gh {\
template<typename Func>bool Convert(Func &f,type &value,const char *info){\
return ObjectStart(f,info)\
    && ObjectEnd(f);}}

#define CONVERT_OBJECT_1(type,m0)\
    namespace gh {\
    template<typename Func>bool Convert(Func &f,type &value,const char *info){\
    return ObjectStart(f,info)\
    && Convert(f,value.m0,#m0)\
    && ObjectEnd(f);}}

#define CONVERT_OBJECT_2(type,m0,m1)\
    namespace gh {\
    template<typename Func>bool Convert(Func &f,type &value,const char *info){\
    return ObjectStart(f,info)\
    && Convert(f,value.m0,#m0)\
    && Convert(f,value.m1,#m1)\
    && ObjectEnd(f);}}

#define CONVERT_OBJECT_3(type,m0,m1,m2)\
    namespace gh {\
    template<typename Func>bool Convert(Func &f,type &value,const char *info){\
    return ObjectStart(f,info)\
    && Convert(f,value.m0,#m0)\
    && Convert(f,value.m1,#m1)\
    && Convert(f,value.m2,#m2)\
    && ObjectEnd(f);}}

#define CONVERT_OBJECT_4(type,m0,m1,m2,m3)\
    namespace gh {\
    template<typename Func>bool Convert(Func &f,type &value,const char *info){\
    return ObjectStart(f,info)\
    && Convert(f,value.m0,#m0)\
    && Convert(f,value.m1,#m1)\
    && Convert(f,value.m2,#m2)\
    && Convert(f,value.m3,#m3)\
    && ObjectEnd(f);}}

#define CONVERT_OBJECT_5(type,m0,m1,m2,m3,m4)\
    namespace gh {\
    template<typename Func>bool Convert(Func &f,type &value,const char *info){\
    return ObjectStart(f,info)\
    && Convert(f,value.m0,#m0)\
    && Convert(f,value.m1,#m1)\
    && Convert(f,value.m2,#m2)\
    && Convert(f,value.m3,#m3)\
    && Convert(f,value.m4,#m4)\
    && ObjectEnd(f);}}\

#define CONVERT_OBJECT_6(type,m0,m1,m2,m3,m4,m5)\
    namespace gh {\
    template<typename Func>bool Convert(Func &f,type &value,const char *info){\
    return ObjectStart(f,info)\
    && Convert(f,value.m0,#m0)\
    && Convert(f,value.m1,#m1)\
    && Convert(f,value.m2,#m2)\
    && Convert(f,value.m3,#m3)\
    && Convert(f,value.m4,#m4)\
    && Convert(f,value.m5,#m5)\
    && ObjectEnd(f);}}


#define CONVERT_OBJECT_BEGIN(type)\
	namespace gh\
{\
	template<typename Func>\
	bool Convert(Func &f,type &value,const char *info)\
{\
	return ObjectStart(f,info)

#define CONVERT_OBJECT_MEMBER(member)\
	&& Convert(f,value.member,#member)

#define CONVERT_OBJECT_MEMBER_TYPE(member,type)\
	&& Convert(f,(*reinterpret_cast<type*>(&value.member)),#member)

#define CONVERT_OBJECT_END\
	&& ObjectEnd(f);\
}\
}

#endif
