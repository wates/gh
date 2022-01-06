
#ifndef GEOMETORY_H_INCLUDED
#define GEOMETORY_H_INCLUDED

#include <math.h>

namespace gh
{
  static const float PI = 3.141593f;
  static const float EPSILON = 1.192092896e-07F;

  template<typename T>
  inline T Abs(const T& f)
  {
    return f < 0 ? -f : f;
  }

  inline float Deg(float deg)
  {
    return deg / 180 * PI;
  }

  inline void AdjustPI(float& r)
  {
    if (r < 0)
      r += (int)(r / (2 * PI) + 1) * (2 * PI);
    else
      r -= (int)(r / (2 * PI)) * (2 * PI);
  }

  template<typename T>
  inline bool IsNear(const T& a, const T& b, const T& factor)
  {
    return Abs(a - b) < factor;
  }

  template <int N, typename T = float>struct VectorValue
  {
    T v[N];
  };
  template <typename T>struct VectorValue<1, T> { union { T v[1]; struct { T x; }; }; };
  template <typename T>struct VectorValue<2, T> { union { T v[2]; struct { T x, y; }; }; };
  template <typename T>struct VectorValue<3, T> { union { T v[3]; struct { T x, y, z; }; }; };
  template <typename T>struct VectorValue<4, T> { union { T v[4]; struct { T x, y, z, w; }; }; };

  //template <typename T>
  //struct Vector<0,T>
  //{
  //    bool operator ==(const Vector &a){return true;}
  //};

  template <int N, typename T = float>struct Vector
    :public VectorValue<N, T>
  {
    typedef VectorValue<N, T> Super;
    Vector& operator=(T a)
    {
      for (int i = 0; i < N; i++)
        Super::v[i] = a;
      return *this;
    }
    //template<int M>
    //Vector(const Vector<M> &a)
    //{
    //    for(int i=0;i<M&&i<N;i++)
    //        this->v[i]=a[i];
    //}
    Vector() {
    }
    Vector(T x) {
      this->x = x;
    }
    Vector(T x, T y) {
      this->x = x;
      this->y = y;
    }
    Vector(T x, T y, T z) {
      this->x = x;
      this->y = y;
      this->z = z;
    }
    Vector(T x, T y, T z, T w) {
      this->x = x;
      this->y = y;
      this->z = z;
      this->w = w;
    }
    template<int D>
    Vector(const Vector<D>& a) {
      *this = a;
    }


    bool operator ==(const Vector<N, T>& a)
    {
      for (int i = 0; i < N; i++)
        if (Super::v[i] != a.v[i])
          return false;
      return true;
    }
    bool operator !=(const Vector<N, T>& a)
    {
      return !(*this == a);
    }
    template<int M, typename A>
    Vector<N, T>& operator=(const Vector<M, A>& a)
    {
      int i;
      for (i = 0; i < N && i < M; i++)
        Super::v[i] = a.Super::v[i];
      for (; i < N; i++)
        Super::v[i] = 0;
      return *this;
    }
    template<int M>
    Vector<N, T>& operator=(const Vector<M, T>& a)
    {
      int i;
      for (i = 0; i < N && i < M; i++)
        Super::v[i] = a.v[i];
      for (; i < N; i++)
        Super::v[i] = 0;
      return *this;
    }
    template<int M>
    Vector<N, T>& operator=(const T(&a)[M])
    {
      int i;
      for (i = 0; i < N && i < M; i++)
        Super::v[i] = a[i];
      for (; i < N; i++)
        Super::v[i] = 0;
      return *this;
    }
    void operator +=(T a)
    {
      for (int i = 0; i < N; i++)
        Super::v[i] += a;
    }
    void operator +=(const Vector<N, T>& a)
    {
      for (int i = 0; i < N; i++)
        Super::v[i] += a.Super::v[i];
    }
    Vector<N, T> operator +(T a)const
    {
      Vector<N, T> n;
      for (int i = 0; i < N; i++)
        n[i] = Super::v[i] + a;
      return n;
    }
    Vector<N, T> operator +(const Vector<N, T>& a)const
    {
      Vector<N, T> n;
      for (int i = 0; i < N; i++)
        n[i] = Super::v[i] + a.Super::v[i];
      return n;
    }
    void operator -=(T a)
    {
      for (int i = 0; i < N; i++)
        Super::v[i] -= a;
    }
    void operator -=(const Vector<N, T>& a)
    {
      for (int i = 0; i < N; i++)
        Super::v[i] -= a.Super::v[i];
    }
    Vector<N, T> operator -(T a)const
    {
      Vector<N, T> n;
      for (int i = 0; i < N; i++)
        n[i] = Super::v[i] - a;
      return n;
    }
    Vector<N, T> operator -(const Vector<N, T>& a)const
    {
      Vector<N, T> n;
      for (int i = 0; i < N; i++)
        n[i] = Super::v[i] - a.Super::v[i];
      return n;
    }
    Vector<N, T> operator -()const
    {
      Vector<N, T> n;
      for (int i = 0; i < N; i++)
        n[i] = -Super::v[i];
      return n;
    }
    void operator *=(const T& a)
    {
      for (int i = 0; i < N; i++)
        Super::v[i] *= a;
    }
    Vector<N, T> operator *(const T& a)const
    {
      Vector<N, T> n;
      for (int i = 0; i < N; i++)
        n[i] = Super::v[i] * a;
      return n;
    }
    void operator /=(const T& a)
    {
      for (int i = 0; i < N; i++)
        Super::v[i] /= a;
    }
    Vector<N, T> operator /(const T& a)const
    {
      Vector<N> n;
      for (int i = 0; i < N; i++)
        n[i] = Super::v[i] / a;
      return n;
    }
    inline T& operator[](const int& n)
    {
      return Super::v[n];
    }
    inline const T& operator[](const int& n)const
    {
      return Super::v[n];
    }
    T Sqlen()const
    {
      return Dot(*this, *this);
    }
    float Magnitude()const
    {
      return sqrtf(Sqlen());
    }
    inline Vector<N, T>& Normalize()
    {
      float len = Magnitude();
      *this /= len;
      return *this;
    }
    //T Dot(const Vector<N,T> &a)const
    //{
    //	T f=Super::v[0]*a.Super::v[0];
    //	for(int i=1;i<N;i++)
    //		f+=Super::v[i]*a.Super::v[i];
    //	return f;
    //}
  };

  typedef Vector<2> Vector2;
  typedef Vector<3> Vector3;
  typedef Vector<4> Vector4;

  inline Vector2 Vec2(const float x, const float y)
  {
    return {x,y};
  }

  inline Vector3 Vec3(const float x, const float y, const float z)
  {
    return { x,y,z };
  }

  inline Vector4 Vec4(const float x, const float y, const float z, const float w)
  {
    return { x,y,z,w };
  }


  struct Quaternion
    :public Vector<4>
  {
    void Identity();
    void Conjugate();
    void Inverse();
    void FromAxisAngle(const Vector3& axis, float angle);
    void FromMatrix(const struct Matrix& rot);
    Quaternion operator *(const Quaternion& a)const;
    Quaternion& operator *=(const Quaternion& a);
    Quaternion& operator *=(float a);
    void Transform(Vector3& v)const;
    void RotateX(float angle);
    void RotateY(float angle);
    void RotateZ(float angle);
    struct Matrix Mat()const;
    inline const Vector3& XYZ()const {
      return *reinterpret_cast<const Vector3*>(this);
    }
  };

  template<int N, typename T>
  inline T Dot(const Vector<N, T>& a, const Vector <N, T>& b)
  {
    T n = a[0] * b[0];
    for (int i = 1; i < N; i++)
      n += a.v[i] * b.v[i];
    return n;
  }
  template<int N, typename T>
  inline T Sqlen(const Vector<N, T>& a)
  {
    return Dot(a, a);
  }
  template<int N, typename T>
  inline T Magnitude(const Vector<N, T>& a)
  {
    return sqrtf(Sqlen(a));
  }
  template<int N, typename T>
  inline Vector<N, T> Normalize(const Vector<N, T>& a)
  {
    return a / Magnitude(a);
  }
  template<int N>
  inline bool IsNear(const Vector<N>& a, const Vector <N>& b, const float& factor)
  {
    for (int i = 0; i < N; i++)
      if (!IsNear(a.v[i], b.v[i], factor))
        return false;
    return true;
  }
  inline float Atan2(const Vector2& a)
  {
    return atan2f(a.y, a.x);
  }

  template<int N>Vector<N> Lerp(const Vector<N>& a, const Vector<N>& b, float f)
  {
    Vector<N> n;
    for (int i = 0; i < N; i++)
      n.v[i] = (1 - f) * a.v[i] + f * b.v[i];
    return n;
  }
  inline Vector<3> Cross(const Vector<3>& a, const Vector<3>& b)
  {
    Vector<3> n;
    n.x = a.y * b.z - a.z * b.y;
    n.y = -(a.x * b.z - a.z * b.x);
    n.z = a.x * b.y - a.y * b.x;
    return n;
  }
  void Hermite(Vector3& out, const Vector3& pos0, const Vector3& tan0, const Vector3& pos1, const Vector3& tan1, float s);
  Quaternion Slerp(const Quaternion& a, Quaternion b, float f);


  //matrix

  struct Matrix
  {
    union
    {
      float m[16];
      struct {
        float _11, _12, _13, _14;
        float _21, _22, _23, _24;
        float _31, _32, _33, _34;
        float _41, _42, _43, _44;
      };
      struct
      {
        Vector4 X, Y, Z, W;
      };
    };
    inline Matrix() {}
    Matrix& Identity();
    Matrix& Zero();
    Matrix& Translate(float x, float y, float z);
    Matrix& RotateX(float r);
    Matrix& RotateY(float r);
    Matrix& RotateZ(float r);
    Matrix& Rotate(float r, float x, float y, float z);
    Matrix& Rotate(const Quaternion& q);
    Matrix& MakeRotate(const Quaternion& q);
    Matrix& Scale(float x, float y, float z);
    Matrix& Inverse();
    void Transform(Vector2& v)const;
    void Transform(Vector3& v)const;
    void Transform(Vector4& v)const;
    void TransformRotation(Vector3& v)const;
    Matrix& Transpose();
    Matrix& operator *=(const Matrix& a);

    Matrix operator *(const Matrix& a)const;
    void LookAtLH(Vector3 from, Vector3 at, Vector3 up);
    void PerspectiveFovLH(float near, float far, float fov, float aspect);
    void OrthoLH(float near, float far, float w, float h);

    inline Matrix& Translate(Vector3 xyz)
    {
      return Translate(xyz.x, xyz.y, xyz.z);
    }
    inline Matrix& Rotate(float r, Vector3 axis)
    {
      return Rotate(r, axis.x, axis.y, axis.z);
    }
    inline Matrix& Scale(const Vector3& s)
    {
      return Scale(s.x, s.y, s.z);
    }
    inline Matrix& Scale(float s)
    {
      return Scale(s, s, s);
    }

    inline static Matrix MakeIdentity() {
      Matrix m;
      m.Identity();
      return m;
    }
  };

  template<typename T>
  struct Motion
  {
    T position;
    T velocity;
    T acceleration;

    inline void Forward(float count)
    {
      position += acceleration * count * count / 2 + velocity * count;
      velocity += acceleration * count;
    }
  };

  template<typename T>
  struct MotionFriction
  {
    T position;
    T velocity;
    T acceleration;
    float friction;

    inline void Forward(float count)
    {
      const float ekm = powf(2.71828f, -1 / friction * count);
      position = ((velocity - acceleration * friction) * (ekm - 1) - acceleration * count) * -friction + position;
      velocity = (velocity - acceleration * friction) * ekm + acceleration * friction;
    }
    void Zero()
    {
      position = 0;
      velocity = 0;
      acceleration = 0;
      friction = 0;
    }
  };
}


#endif
