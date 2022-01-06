
#ifndef GH_FERTEX_H_
#define GH_FERTEX_H_

#include "geometory.h"

namespace gh {

  static const int FTX_POSITION = 0x0001;//xyz
  static const int FTX_NORMAL = 0x0002;//xyz
  static const int FTX_DIFFUSE = 0x0004;//rgba
  static const int FTX_SPECULAR = 0x0008;//rgba
  static const int FTX_WEIGHT1 = 0x0010;//
  static const int FTX_WEIGHT2 = 0x0020;//
  static const int FTX_WEIGHT3 = 0x0030;//
  static const int FTX_WEIGHT4 = 0x0040;//
  static const int FTX_WEIGHTMASK = 0x00F0;
  static const int FTX_TEX1 = 0x0100;//uv
  static const int FTX_TEX2 = 0x0200;//uvuv
  static const int FTX_TEX3 = 0x0300;//uvuvuv
  static const int FTX_TEX4 = 0x0400;
  static const int FTX_TEXTUREMASK = 0x0F00;
  static const int FTX_TANGENT = 0x1000;
  static const int FTX_BINORMAL = 0x2000;
  static const int FTX_VIEW_DIRECTION = 0x10000;
  static const int FTX_LIGHT_DIRECTION = 0x20000;
  static const int FTX_VIEWPORT_POSITION = 0x40000;//for VPOS / gl_FlagCoord

  template<int FTX, int MASK = FTX & FTX_POSITION>
  struct FTX_Position
  {
    Vector3 position;
  };
  template<int FTX>
  struct FTX_Position<FTX, 0> {};

  template<int FTX, int MASK = FTX & FTX_NORMAL>
  struct FTX_Normal :
    public FTX_Position<FTX>
  {
    Vector3 normal;
  };
  template<int FTX>
  struct FTX_Normal<FTX, 0>
    :public FTX_Position<FTX> {};

  template<int FTX, int MASK = FTX & FTX_TANGENT>
  struct FTX_Tangent :
    public FTX_Normal<FTX>
  {
    Vector3 tangent;
  };
  template<int FTX>
  struct FTX_Tangent<FTX, 0>
    :public FTX_Normal<FTX> {};

  template<int FTX, int MASK = FTX & FTX_DIFFUSE>
  struct FTX_Diffuse :
    public FTX_Tangent<FTX>
  {
    Color diffuse;
  };

  template<int FTX>
  struct FTX_Diffuse<FTX, 0> :
    public FTX_Tangent<FTX> {};

  template<int FTX, int MASK = FTX & FTX_SPECULAR>
  struct FTX_Specular
    :public FTX_Diffuse<FTX>
  {
    Color specular;
  };
  template<int FTX>
  struct FTX_Specular<FTX, 0>
    :public FTX_Diffuse<FTX> {};

  template<int FTX, int MASK = FTX & FTX_WEIGHTMASK>
  struct FTX_Weights
    :public FTX_Specular<FTX> {};
  template<int FTX>
  struct FTX_Weights<FTX, FTX_WEIGHT1>
    :public FTX_Specular<FTX>
  {
    float weightindex;
  };

  template<int FTX>
  struct FTX_Weights<FTX, FTX_WEIGHT2>
    :public FTX_Specular<FTX>
  {
    float weightindex[2];
    float weight[2];
  };
  template<int FTX>
  struct FTX_Weights<FTX, FTX_WEIGHT3>
    :public FTX_Specular<FTX>
  {
    float weightindex[3];
    float weight[3];
  };
  template<int FTX>
  struct FTX_Weights<FTX, FTX_WEIGHT4>
    :public FTX_Specular<FTX>
  {
    float weightindex[4];
    float weight[4];
  };

  template<int FTX, int MASK = FTX & FTX_TEXTUREMASK>
  struct FTX_Texture :
    public FTX_Weights<FTX> {};
  template<int FTX>
  struct FTX_Texture<FTX, FTX_TEX1>
    :public FTX_Weights<FTX>
  {
    float u, v;
  };
  template<int FTX>
  struct FTX_Texture<FTX, FTX_TEX2>
    :public FTX_Weights<FTX>
  {
    float u, v, u2, v2;
  };
  template<int FTX>
  struct FTX_Texture<FTX, FTX_TEX3>
    :public FTX_Weights<FTX>
  {
    float u, v, u2, v2, u3, v3;
  };
  template<int FTX>
  struct FTX_Texture<FTX, FTX_TEX4>
    :public FTX_Weights<FTX>
  {
    float u, v, u2, v2, u3, v3, u4, v4;
  };

  template<int FTX>
  struct FTX_Type
  {
    typedef int type;
  };
  template<>
  struct FTX_Type<FTX_POSITION>
  {
    typedef Vector3 type;
  };
  template<>
  struct FTX_Type<FTX_NORMAL>
  {
    typedef Vector3 type;
  };
  template<>
  struct FTX_Type<FTX_TANGENT>
  {
    typedef Vector3 type;
  };
  template<>
  struct FTX_Type<FTX_DIFFUSE>
  {
    typedef Color type;
  };
  template<>
  struct FTX_Type<FTX_SPECULAR>
  {
    typedef Color type;
  };
  template<>
  struct FTX_Type<FTX_WEIGHT1>
  {
    typedef FTX_Weights<0, FTX_WEIGHT1> type;
  };
  template<>
  struct FTX_Type<FTX_WEIGHT2>
  {
    typedef FTX_Weights<0, FTX_WEIGHT2> type;
  };
  template<>
  struct FTX_Type<FTX_WEIGHT3>
  {
    typedef FTX_Weights<0, FTX_WEIGHT3> type;
  };
  template<>
  struct FTX_Type<FTX_WEIGHT4>
  {
    typedef FTX_Weights<0, FTX_WEIGHT4> type;
  };
  template<>
  struct FTX_Type<FTX_TEX1>
  {
    typedef FTX_Texture<0, FTX_TEX1> type;
  };
  template<>
  struct FTX_Type<FTX_TEX2>
  {
    typedef FTX_Texture<0, FTX_TEX2> type;
  };
  template<>
  struct FTX_Type<FTX_TEX3>
  {
    typedef FTX_Texture<0, FTX_TEX3> type;
  };
  template<>
  struct FTX_Type<FTX_TEX4>
  {
    typedef FTX_Texture<0, FTX_TEX4> type;
  };

  template<int FTX>
  struct Fertex
    :public FTX_Texture<FTX>
  {
    static const int format = FTX;
    static const int size = (int)sizeof(FTX_Texture<FTX>);
  };

  inline int GetFertexSize(int format)
  {
    int size = 0;
    if (format & FTX_POSITION)
      size += sizeof(FTX_Position<0, 1>);
    if (format & FTX_NORMAL)
      size += sizeof(FTX_Normal<0, 1>);
    if (format & FTX_TANGENT)
      size += sizeof(FTX_Tangent<0, 1>);
    if (format & FTX_DIFFUSE)
      size += sizeof(FTX_Diffuse<0, 1>);
    if (format & FTX_SPECULAR)
      size += sizeof(FTX_Specular<0, 1>);
    if ((format & FTX_WEIGHTMASK) == FTX_WEIGHT1)
      size += sizeof(FTX_Weights<0, FTX_WEIGHT1>);
    else if ((format & FTX_WEIGHTMASK) == FTX_WEIGHT2)
      size += sizeof(FTX_Weights<0, FTX_WEIGHT2>);
    else if ((format & FTX_WEIGHTMASK) == FTX_WEIGHT3)
      size += sizeof(FTX_Weights<0, FTX_WEIGHT3>);
    else if ((format & FTX_WEIGHTMASK) == FTX_WEIGHT4)
      size += sizeof(FTX_Weights<0, FTX_WEIGHT4>);
    if ((format & FTX_TEXTUREMASK) == FTX_TEX1)
      size += sizeof(FTX_Texture<0, FTX_TEX1>);
    else if ((format & FTX_TEXTUREMASK) == FTX_TEX2)
      size += sizeof(FTX_Texture<0, FTX_TEX2>);
    else if ((format & FTX_TEXTUREMASK) == FTX_TEX3)
      size += sizeof(FTX_Texture<0, FTX_TEX3>);
    else if ((format & FTX_TEXTUREMASK) == FTX_TEX4)
      size += sizeof(FTX_Texture<0, FTX_TEX4>);
    return size;
  }
  inline FTX_Type<FTX_POSITION>::type& GetFertexOffsetPosition(void* ftx, int format)
  {
    return *(FTX_Type<FTX_POSITION>::type*)ftx;
  }
  inline FTX_Type<FTX_NORMAL>::type& GetFertexOffsetNormal(void* ftx, int format)
  {
    int offset = 0;
    if (format & FTX_POSITION)
      offset += sizeof(FTX_Position<0, 1>);
    return *(FTX_Type<FTX_NORMAL>::type*)(((char*)ftx) + offset);
  }
  inline FTX_Type<FTX_TANGENT>::type& GetFertexOffsetTangent(void* ftx, int format)
  {
    int offset = 0;
    if (format & FTX_POSITION)
      offset += sizeof(FTX_Position<0, 1>);
    if (format & FTX_NORMAL)
      offset += sizeof(FTX_Normal<0, 1>);
    return *(FTX_Type<FTX_TANGENT>::type*)(((char*)ftx) + offset);
  }
  inline FTX_Type<FTX_DIFFUSE>::type& GetFertexOffsetDiffuse(void* ftx, int format)
  {
    int offset = 0;
    if (format & FTX_POSITION)
      offset += sizeof(FTX_Position<0, 1>);
    if (format & FTX_NORMAL)
      offset += sizeof(FTX_Normal<0, 1>);
    if (format & FTX_TANGENT)
      offset += sizeof(FTX_Tangent<0, 1>);
    return *(FTX_Type<FTX_DIFFUSE>::type*)(((char*)ftx) + offset);
  }
  inline FTX_Type<FTX_SPECULAR>::type& GetFertexOffsetSpecular(void* ftx, int format)
  {
    int offset = 0;
    if (format & FTX_POSITION)
      offset += sizeof(FTX_Position<0, 1>);
    if (format & FTX_NORMAL)
      offset += sizeof(FTX_Normal<0, 1>);
    if (format & FTX_TANGENT)
      offset += sizeof(FTX_Tangent<0, 1>);
    if (format & FTX_DIFFUSE)
      offset += sizeof(FTX_Diffuse<0, 1>);
    return *(FTX_Type<FTX_SPECULAR>::type*)(((char*)ftx) + offset);
  }

  inline FTX_Type<FTX_WEIGHT1>::type& GetFertexOffsetWeight1(void* ftx, int format)
  {
    int offset = 0;
    if (format & FTX_POSITION)
      offset += sizeof(FTX_Position<0, 1>);
    if (format & FTX_NORMAL)
      offset += sizeof(FTX_Normal<0, 1>);
    if (format & FTX_TANGENT)
      offset += sizeof(FTX_Tangent<0, 1>);
    if (format & FTX_DIFFUSE)
      offset += sizeof(FTX_Diffuse<0, 1>);
    if (format & FTX_SPECULAR)
      offset += sizeof(FTX_Specular<0, 1>);
    return *(FTX_Type<FTX_WEIGHT1>::type*)(((char*)ftx) + offset);
  }
  inline FTX_Type<FTX_WEIGHT2>::type& GetFertexOffsetWeight2(void* ftx, int format)
  {
    int offset = 0;
    if (format & FTX_POSITION)
      offset += sizeof(FTX_Position<0, 1>);
    if (format & FTX_NORMAL)
      offset += sizeof(FTX_Normal<0, 1>);
    if (format & FTX_TANGENT)
      offset += sizeof(FTX_Tangent<0, 1>);
    if (format & FTX_DIFFUSE)
      offset += sizeof(FTX_Diffuse<0, 1>);
    if (format & FTX_SPECULAR)
      offset += sizeof(FTX_Specular<0, 1>);
    return *(FTX_Type<FTX_WEIGHT2>::type*)(((char*)ftx) + offset);
  }
  inline FTX_Type<FTX_WEIGHT3>::type& GetFertexOffsetWeight3(void* ftx, int format)
  {
    int offset = 0;
    if (format & FTX_POSITION)
      offset += sizeof(FTX_Position<0, 1>);
    if (format & FTX_NORMAL)
      offset += sizeof(FTX_Normal<0, 1>);
    if (format & FTX_TANGENT)
      offset += sizeof(FTX_Tangent<0, 1>);
    if (format & FTX_DIFFUSE)
      offset += sizeof(FTX_Diffuse<0, 1>);
    if (format & FTX_SPECULAR)
      offset += sizeof(FTX_Specular<0, 1>);
    return *(FTX_Type<FTX_WEIGHT3>::type*)(((char*)ftx) + offset);
  }
  inline FTX_Type<FTX_WEIGHT4>::type& GetFertexOffsetWeight4(void* ftx, int format)
  {
    int offset = 0;
    if (format & FTX_POSITION)
      offset += sizeof(FTX_Position<0, 1>);
    if (format & FTX_NORMAL)
      offset += sizeof(FTX_Normal<0, 1>);
    if (format & FTX_TANGENT)
      offset += sizeof(FTX_Tangent<0, 1>);
    if (format & FTX_DIFFUSE)
      offset += sizeof(FTX_Diffuse<0, 1>);
    if (format & FTX_SPECULAR)
      offset += sizeof(FTX_Specular<0, 1>);
    return *(FTX_Type<FTX_WEIGHT4>::type*)(((char*)ftx) + offset);
  }
  inline FTX_Type<FTX_TEX1>::type& GetFertexOffsetTex1(void* ftx, int format)
  {
    int offset = GetFertexSize(format & (FTX_POSITION | FTX_NORMAL | FTX_TANGENT | FTX_DIFFUSE | FTX_SPECULAR | FTX_WEIGHTMASK));
    return *(FTX_Type<FTX_TEX1>::type*)(((char*)ftx) + offset);
  }

}

#endif
