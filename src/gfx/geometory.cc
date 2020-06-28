#include "geometory.h"
#include <math.h>

#ifdef _WIN32
//#include <d3dx9.h>
#pragma comment(lib,"d3dx9.lib")
#endif

namespace gh {
  Matrix& Matrix::RotateX(float r)
  {
    float s = sinf(r), c = cosf(r);
    float m12 = _12, m22 = _22, m32 = _32, m42 = _42;
    _12 = m12 * c - _13 * s; _13 = m12 * s + _13 * c;
    _22 = m22 * c - _23 * s; _23 = m22 * s + _23 * c;
    _32 = m32 * c - _33 * s; _33 = m32 * s + _33 * c;
    _42 = m42 * c - _43 * s; _43 = m42 * s + _43 * c;
    return *this;
  }

  Matrix& Matrix::RotateY(float r)
  {
    float s = sinf(r), c = cosf(r);
    float m11 = _11, m21 = _21, m31 = _31, m41 = _41;
    _11 = m11 * c + _13 * s; _13 = _13 * c - m11 * s;
    _21 = m21 * c + _23 * s; _23 = _23 * c - m21 * s;
    _31 = m31 * c + _33 * s; _33 = _33 * c - m31 * s;
    _41 = m41 * c + _43 * s; _43 = _43 * c - m41 * s;
    return *this;
  }

  Matrix& Matrix::RotateZ(float r)
  {
    float s = sinf(r), c = cosf(r);
    float m11 = _11, m21 = _21, m31 = _31, m41 = _41;
    _11 = m11 * c - _12 * s; _12 = m11 * s + _12 * c;
    _21 = m21 * c - _22 * s; _22 = m21 * s + _22 * c;
    _31 = m31 * c - _32 * s; _32 = m31 * s + _32 * c;
    _41 = m41 * c - _42 * s; _42 = m41 * s + _42 * c;
    return *this;
  }

  Matrix& Matrix::Translate(float x, float y, float z)
  {
    _11 += _14 * x; _12 += _14 * y; _13 += _14 * z;
    _21 += _24 * x; _22 += _24 * y; _23 += _24 * z;
    _31 += _34 * x; _32 += _34 * y; _33 += _34 * z;
    _41 += _44 * x; _42 += _44 * y; _43 += _44 * z;
    return *this;
  }

  Matrix& Matrix::operator *=(const Matrix& Mat)
  {
    Matrix temp = *this;
    _11 = temp._11 * Mat._11 + temp._12 * Mat._21 + temp._13 * Mat._31 + temp._14 * Mat._41;
    _12 = temp._11 * Mat._12 + temp._12 * Mat._22 + temp._13 * Mat._32 + temp._14 * Mat._42;
    _13 = temp._11 * Mat._13 + temp._12 * Mat._23 + temp._13 * Mat._33 + temp._14 * Mat._43;
    _14 = temp._11 * Mat._14 + temp._12 * Mat._24 + temp._13 * Mat._34 + temp._14 * Mat._44;
    _21 = temp._21 * Mat._11 + temp._22 * Mat._21 + temp._23 * Mat._31 + temp._24 * Mat._41;
    _22 = temp._21 * Mat._12 + temp._22 * Mat._22 + temp._23 * Mat._32 + temp._24 * Mat._42;
    _23 = temp._21 * Mat._13 + temp._22 * Mat._23 + temp._23 * Mat._33 + temp._24 * Mat._43;
    _24 = temp._21 * Mat._14 + temp._22 * Mat._24 + temp._23 * Mat._34 + temp._24 * Mat._44;
    _31 = temp._31 * Mat._11 + temp._32 * Mat._21 + temp._33 * Mat._31 + temp._34 * Mat._41;
    _32 = temp._31 * Mat._12 + temp._32 * Mat._22 + temp._33 * Mat._32 + temp._34 * Mat._42;
    _33 = temp._31 * Mat._13 + temp._32 * Mat._23 + temp._33 * Mat._33 + temp._34 * Mat._43;
    _34 = temp._31 * Mat._14 + temp._32 * Mat._24 + temp._33 * Mat._34 + temp._34 * Mat._44;
    _41 = temp._41 * Mat._11 + temp._42 * Mat._21 + temp._43 * Mat._31 + temp._44 * Mat._41;
    _42 = temp._41 * Mat._12 + temp._42 * Mat._22 + temp._43 * Mat._32 + temp._44 * Mat._42;
    _43 = temp._41 * Mat._13 + temp._42 * Mat._23 + temp._43 * Mat._33 + temp._44 * Mat._43;
    _44 = temp._41 * Mat._14 + temp._42 * Mat._24 + temp._43 * Mat._34 + temp._44 * Mat._44;
    return *this;
  }

  Matrix Matrix::operator *(const Matrix& Mat)const
  {
    Matrix temp;
    temp._11 = _11 * Mat._11 + _12 * Mat._21 + _13 * Mat._31 + _14 * Mat._41;
    temp._12 = _11 * Mat._12 + _12 * Mat._22 + _13 * Mat._32 + _14 * Mat._42;
    temp._13 = _11 * Mat._13 + _12 * Mat._23 + _13 * Mat._33 + _14 * Mat._43;
    temp._14 = _11 * Mat._14 + _12 * Mat._24 + _13 * Mat._34 + _14 * Mat._44;
    temp._21 = _21 * Mat._11 + _22 * Mat._21 + _23 * Mat._31 + _24 * Mat._41;
    temp._22 = _21 * Mat._12 + _22 * Mat._22 + _23 * Mat._32 + _24 * Mat._42;
    temp._23 = _21 * Mat._13 + _22 * Mat._23 + _23 * Mat._33 + _24 * Mat._43;
    temp._24 = _21 * Mat._14 + _22 * Mat._24 + _23 * Mat._34 + _24 * Mat._44;
    temp._31 = _31 * Mat._11 + _32 * Mat._21 + _33 * Mat._31 + _34 * Mat._41;
    temp._32 = _31 * Mat._12 + _32 * Mat._22 + _33 * Mat._32 + _34 * Mat._42;
    temp._33 = _31 * Mat._13 + _32 * Mat._23 + _33 * Mat._33 + _34 * Mat._43;
    temp._34 = _31 * Mat._14 + _32 * Mat._24 + _33 * Mat._34 + _34 * Mat._44;
    temp._41 = _41 * Mat._11 + _42 * Mat._21 + _43 * Mat._31 + _44 * Mat._41;
    temp._42 = _41 * Mat._12 + _42 * Mat._22 + _43 * Mat._32 + _44 * Mat._42;
    temp._43 = _41 * Mat._13 + _42 * Mat._23 + _43 * Mat._33 + _44 * Mat._43;
    temp._44 = _41 * Mat._14 + _42 * Mat._24 + _43 * Mat._34 + _44 * Mat._44;
    return temp;
  }

  Matrix& Matrix::Scale(float x, float y, float z)
  {
    _11 *= x; _21 *= x; _31 *= x; _41 *= x;
    _12 *= y; _22 *= y; _32 *= y; _42 *= y;
    _13 *= z; _23 *= z; _33 *= z; _43 *= z;
    return *this;
  }

  Matrix& Matrix::Identity()
  {
    _11 = 1.0f; _12 = 0.0f; _13 = 0.0f; _14 = 0.0f;
    _21 = 0.0f; _22 = 1.0f; _23 = 0.0f; _24 = 0.0f;
    _31 = 0.0f; _32 = 0.0f; _33 = 1.0f; _34 = 0.0f;
    _41 = 0.0f; _42 = 0.0f; _43 = 0.0f; _44 = 1.0f;
    return *this;
  }

  Matrix& Matrix::Zero()
  {
    _11 = 0.0f; _12 = 0.0f; _13 = 0.0f; _14 = 0.0f;
    _21 = 0.0f; _22 = 0.0f; _23 = 0.0f; _24 = 0.0f;
    _31 = 0.0f; _32 = 0.0f; _33 = 0.0f; _34 = 0.0f;
    _41 = 0.0f; _42 = 0.0f; _43 = 0.0f; _44 = 0.0f;
    return *this;
  }

  void Matrix::Transform(Vector3& v)const
  {

#ifdef D3DX_DEFAULT
    Vector4 v4;
    D3DXVec3Transform((D3DXVECTOR4*)&v4, (D3DXVECTOR3*)&v, (D3DXMATRIX*)this);
    v.x = v4.x / v4.w;
    v.y = v4.y / v4.w;
    v.z = v4.z / v4.w;
#else
    Vector3 t;
    t.x = v.x * _11 + v.y * _21 + v.z * _31 + _41;
    t.y = v.x * _12 + v.y * _22 + v.z * _32 + _42;
    t.z = v.x * _13 + v.y * _23 + v.z * _33 + _43;
    v = t;
#endif
  }
  void Matrix::Transform(Vector4& v)const
  {
    Vector4 t;
    t.x = v.x * _11 + v.y * _21 + v.z * _31 + v.w * _41;
    t.y = v.x * _12 + v.y * _22 + v.z * _32 + v.w * _42;
    t.z = v.x * _13 + v.y * _23 + v.z * _33 + v.w * _43;
    t.w = v.x * _14 + v.y * _24 + v.z * _34 + v.w * _44;
    v = t;
  }

  void Matrix::PerspectiveFovLH(float near, float far, float fov, float aspect)
  {
    float cot;

    cot = 1.0f / (float)tanf(fov * 0.5f);

    m[0] = cot * aspect;
    m[1] = 0;
    m[2] = 0;
    m[3] = 0;

    m[4] = 0;
    m[5] = cot;
    m[6] = 0;
    m[7] = 0;

    m[8] = 0;
    m[9] = 0;
    m[10] = far / (far - near);
    m[11] = 1;

    m[12] = 0;
    m[13] = 0;
    m[14] = -near * far / (far - near);
    m[15] = 0;

  }

  void Matrix::OrthoLH(float near, float far, float w, float h)
  {
    m[0] = 2 / w;
    m[1] = 0;
    m[2] = 0;
    m[3] = 0;

    m[4] = 0;
    m[5] = 2 / h;
    m[6] = 0;
    m[7] = 0;

    m[8] = 0;
    m[9] = 0;
    m[10] = 1 / (far - near);
    m[11] = 0;

    m[12] = 0;
    m[13] = 0;
    m[14] = near / (near - far);
    m[15] = 1;

  }

  void Matrix::LookAtLH(Vector3 from, Vector3 at, Vector3 up)
  {
    Vector3 x, y, z;

    z = at - from;
    z.Normalize();

    x = Cross(up, z);
    x.Normalize();

    y = Cross(z, x);

    m[0] = x.x;
    m[1] = y.x;
    m[2] = z.x;
    m[3] = 0;

    m[4] = x.y;
    m[5] = y.y;
    m[6] = z.y;
    m[7] = 0;

    m[8] = x.z;
    m[9] = y.z;
    m[10] = z.z;
    m[11] = 0;

    m[12] = -Dot(x, from);
    m[13] = -Dot(y, from);
    m[14] = -Dot(z, from);
    m[15] = 1;
  }

  void Matrix::Transform(Vector2& v)const
  {
    Vector2 a;
    a.x = v.x * _11 + v.y * _21 + _41;
    a.y = v.x * _12 + v.y * _22 + _42;
    v = a;
  }

  void Matrix::TransformRotation(Vector3& v)const
  {
    Vector3 a;
    a.x = v.x * _11 + v.y * _21 + v.z * _31;
    a.y = v.x * _12 + v.y * _22 + v.z * _32;
    a.z = v.x * _13 + v.y * _23 + v.z * _33;
    v = a;
  }

  Matrix& Matrix::Transpose()
  {
    float t;
    t = _12; _12 = _21; _21 = t;
    t = _13; _13 = _31; _31 = t;
    t = _14; _14 = _41; _41 = t;
    t = _23; _23 = _32; _32 = t;
    t = _24; _24 = _42; _42 = t;
    t = _34; _34 = _43; _43 = t;
    return *this;
  }

  Matrix& Matrix::Rotate(float r, float x, float y, float z)
  {
    float c = cosf(r);
    float s = sinf(r);
    float ic = 1 - c;

    Matrix n;

    n.m[0] = x * x * ic + c;
    n.m[1] = x * y * ic - z * s;
    n.m[2] = x * z * ic + y * s;
    n.m[3] = 0;
    n.m[4] = y * x * ic + z * s;
    n.m[5] = y * y * ic + c;
    n.m[6] = y * z * ic - x * s;
    n.m[7] = 0;
    n.m[8] = x * z * ic - y * s;
    n.m[9] = y * z * ic + x * s;
    n.m[10] = z * z * ic + c;
    n.m[11] = 0;
    n.m[12] = 0;
    n.m[13] = 0;
    n.m[14] = 0;
    n.m[15] = 1;

    *this *= n;
    return *this;
  }

  Matrix& Matrix::Rotate(const Quaternion& q)
  {
    Matrix mat;
#if 1
    mat.Identity();
    mat.m[0] = 1.0f - 2.0f * (-q.y * -q.y + -q.z * -q.z);
    mat.m[1] = 2.0f * (-q.x * -q.y - -q.z * q.w);
    mat.m[2] = 2.0f * (-q.z * -q.x + -q.y * q.w);

    mat.m[4] = 2.0f * (-q.x * -q.y + -q.z * q.w);
    mat.m[5] = 1.0f - 2.0f * (-q.z * -q.z + -q.x * -q.x);
    mat.m[6] = 2.0f * (-q.y * -q.z - -q.x * q.w);

    mat.m[8] = 2.0f * (-q.z * -q.x - -q.y * q.w);
    mat.m[9] = 2.0f * (-q.y * -q.z + -q.x * q.w);
    mat.m[10] = 1.0f - 2.0f * (-q.y * -q.y + -q.x * -q.x);
    *this *= mat;
#else
    mat.MakeRotate(q);
    *this *= mat;
#endif
    return *this;
  }

  Matrix& Matrix::MakeRotate(const Quaternion& q)
  {
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;

    m[0] = 1 - (yy + zz);
    m[1] = xy - wz;
    m[2] = xz + wy;
    m[4] = xy + wz;
    m[5] = 1 - (xx + zz);
    m[6] = yz - wx;
    m[8] = xz - wy;
    m[9] = yz + wx;
    m[10] = 1 - (xx + yy);
    m[3] = 0;
    m[7] = 0;
    m[11] = 0;
    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[15] = 1;
    return *this;
  }

  Matrix& Matrix::Inverse()
  {
    Matrix t = *this;
    const float* n = t.m;

    float det =
      +n[0] * n[5] * n[10] * n[15] + n[0] * n[6] * n[11] * n[13] + n[0] * n[7] * n[9] * n[14]
      + n[1] * n[4] * n[11] * n[14] + n[1] * n[6] * n[8] * n[15] + n[1] * n[7] * n[10] * n[12]
      + n[2] * n[4] * n[9] * n[15] + n[2] * n[5] * n[11] * n[12] + n[2] * n[7] * n[8] * n[13]
      + n[3] * n[4] * n[10] * n[13] + n[3] * n[5] * n[8] * n[14] + n[3] * n[6] * n[9] * n[12]
      - n[0] * n[5] * n[11] * n[14] - n[0] * n[6] * n[9] * n[15] - n[0] * n[7] * n[10] * n[13]
      - n[1] * n[4] * n[10] * n[15] - n[1] * n[6] * n[11] * n[12] - n[1] * n[7] * n[8] * n[14]
      - n[2] * n[4] * n[11] * n[13] - n[2] * n[5] * n[8] * n[15] - n[2] * n[7] * n[9] * n[12]
      - n[3] * n[4] * n[9] * n[14] - n[3] * n[5] * n[10] * n[12] - n[3] * n[6] * n[8] * n[13];

    float inv_det = 1 / det;

    m[0] = inv_det * (n[5] * n[10] * n[15] + n[6] * n[11] * n[13] + n[7] * n[9] * n[14] - n[5] * n[11] * n[14] - n[6] * n[9] * n[15] - n[7] * n[10] * n[13]);
    m[1] = inv_det * (n[1] * n[11] * n[14] + n[2] * n[9] * n[15] + n[3] * n[10] * n[13] - n[1] * n[10] * n[15] - n[2] * n[11] * n[13] - n[3] * n[9] * n[14]);
    m[2] = inv_det * (n[1] * n[6] * n[15] + n[2] * n[7] * n[13] + n[3] * n[5] * n[14] - n[1] * n[7] * n[14] - n[2] * n[5] * n[15] - n[3] * n[6] * n[13]);
    m[3] = inv_det * (n[1] * n[7] * n[10] + n[2] * n[5] * n[11] + n[3] * n[6] * n[9] - n[1] * n[6] * n[11] - n[2] * n[7] * n[9] - n[3] * n[5] * n[10]);
    m[4] = inv_det * (n[4] * n[11] * n[14] + n[6] * n[8] * n[15] + n[7] * n[10] * n[12] - n[4] * n[10] * n[15] - n[6] * n[11] * n[12] - n[7] * n[8] * n[14]);
    m[5] = inv_det * (n[0] * n[10] * n[15] + n[2] * n[11] * n[12] + n[3] * n[8] * n[14] - n[0] * n[11] * n[14] - n[2] * n[8] * n[15] - n[3] * n[10] * n[12]);
    m[6] = inv_det * (n[0] * n[7] * n[14] + n[2] * n[4] * n[15] + n[3] * n[6] * n[12] - n[0] * n[6] * n[15] - n[2] * n[7] * n[12] - n[3] * n[4] * n[14]);
    m[7] = inv_det * (n[0] * n[6] * n[11] + n[2] * n[7] * n[8] + n[3] * n[4] * n[10] - n[0] * n[7] * n[10] - n[2] * n[4] * n[11] - n[3] * n[6] * n[8]);
    m[8] = inv_det * (n[4] * n[9] * n[15] + n[5] * n[11] * n[12] + n[7] * n[8] * n[13] - n[4] * n[11] * n[13] - n[5] * n[8] * n[15] - n[7] * n[9] * n[12]);
    m[9] = inv_det * (n[0] * n[11] * n[13] + n[1] * n[8] * n[15] + n[3] * n[9] * n[12] - n[0] * n[9] * n[15] - n[1] * n[11] * n[12] - n[3] * n[8] * n[13]);
    m[10] = inv_det * (n[0] * n[5] * n[15] + n[1] * n[7] * n[12] + n[3] * n[4] * n[13] - n[0] * n[7] * n[13] - n[1] * n[4] * n[15] - n[3] * n[5] * n[12]);
    m[11] = inv_det * (n[0] * n[7] * n[9] + n[1] * n[4] * n[11] + n[3] * n[5] * n[8] - n[0] * n[5] * n[11] - n[1] * n[7] * n[8] - n[3] * n[4] * n[9]);
    m[12] = inv_det * (n[4] * n[10] * n[13] + n[5] * n[8] * n[14] + n[6] * n[9] * n[12] - n[4] * n[9] * n[14] - n[5] * n[10] * n[12] - n[6] * n[8] * n[13]);
    m[13] = inv_det * (n[0] * n[9] * n[14] + n[1] * n[10] * n[12] + n[2] * n[8] * n[13] - n[0] * n[10] * n[13] - n[1] * n[8] * n[14] - n[2] * n[9] * n[12]);
    m[14] = inv_det * (n[0] * n[6] * n[13] + n[1] * n[4] * n[14] + n[2] * n[5] * n[12] - n[0] * n[5] * n[14] - n[1] * n[6] * n[12] - n[2] * n[4] * n[13]);
    m[15] = inv_det * (n[0] * n[5] * n[10] + n[1] * n[6] * n[8] + n[2] * n[4] * n[9] - n[0] * n[6] * n[9] - n[1] * n[4] * n[10] - n[2] * n[5] * n[8]);
    return *this;
  }

  void Quaternion::Identity()
  {
    x = 0;
    y = 0;
    z = 0;
    w = 1;
  }

  void Quaternion::Conjugate()
  {
    x = -x;
    y = -y;
    z = -z;
  }

  void Quaternion::Inverse()
  {
    Conjugate();
    Normalize();
  }

  Quaternion Quaternion::operator *(const Quaternion& a)const
  {
    Quaternion n;
#if 0
    n.x = y * a.z - z * a.y + w * a.x + x * a.w;
    n.y = z * a.x - x * a.z + w * a.y + y * a.w;
    n.z = x * a.y - y * a.x + w * a.z + z * a.w;
    n.w = -x * a.x - y * a.y - z * a.z + w * a.w;
#else
    n.y = y * a.w + w * a.y + z * a.x - x * a.z;
    n.z = z * a.w + w * a.z + x * a.y - y * a.x;
    n.x = x * a.w + w * a.x + y * a.z - z * a.y;
    n.w = w * a.w - x * a.x - y * a.y - z * a.z;
#endif
    return n;
  }
  Quaternion& Quaternion::operator *=(const Quaternion& a)
  {
    *this = *this * a;
    return *this;
  }
  Quaternion& Quaternion::operator*=(float a)
  {
    x *= a;
    y *= a;
    z *= a;
    this->Normalize();
    return *this;
  }

  void Quaternion::Transform(Vector3& v)const
  {
    float ix = w * v.x + y * v.z - z * v.y;
    float iy = w * v.y + z * v.x - x * v.z;
    float iz = w * v.z + x * v.y - y * v.x;
    float iw = -x * v.x - y * v.y - z * v.z;
    v.x = ix * w + iw * -x + iy * -z - iz * -y;
    v.y = iy * w + iw * -y + iz * -x - ix * -z;
    v.z = iz * w + iw * -z + ix * -y - iy * -x;
  }

  Matrix Quaternion::Mat()const
  {
    Matrix mat;
    mat._11 = 1 - 2 * (y * y + z * z);
    mat._12 = 2 * (x * y + w * z);
    mat._13 = 2 * (x * z - w * y);
    mat._14 = 0;

    mat._21 = 2 * (x * y - w * z);
    mat._22 = 1 - 2 * (x * x + z * z);
    mat._23 = 2 * (y * z + w * x);
    mat._24 = 0;

    mat._31 = 2 * (x * z + w * y);
    mat._32 = 2 * (y * z - w * x);
    mat._33 = 1 - 2 * (x * x + y * y);
    mat._34 = 0;

    mat._41 = 0;
    mat._42 = 0;
    mat._43 = 0;
    mat._44 = 1;
    return mat;
  }

  void Quaternion::FromAxisAngle(const Vector3& axis, float angle)
  {
    angle *= 0.5f;
    float s = sinf(angle);
    x = axis.x * s;
    y = axis.y * s;
    z = axis.z * s;
    w = cosf(angle);
  }

  void Quaternion::FromMatrix(const Matrix& rot)
  {
    float trace = rot._11 + rot._22 + rot._33;
#if 0
    if (trace > 0) {
      float s = 0.5 / sqrtf(trace + 1.0);
      w = 0.25 / s;
      x = (rot._32 - rot._23) * s;
      y = (rot._13 - rot._31) * s;
      z = (rot._21 - rot._12) * s;

    }
    else if (rot._11 > rot._22 && rot._11 > rot._33) {
      float s = 2.0 * sqrtf(1.0 + rot._11 - rot._22 - rot._33);
      w = (rot._32 - rot._23) / s;
      x = 0.25 * s;
      y = (rot._12 + rot._21) / s;
      z = (rot._13 + rot._31) / s;

    }
    else if (rot._22 > rot._33) {
      float s = 2.0 * sqrtf(1.0 + rot._22 - rot._11 - rot._33);
      w = (rot._13 - rot._31) / s;
      x = (rot._12 + rot._21) / s;
      y = 0.25 * s;
      z = (rot._23 + rot._32) / s;

    }
    else {
      float s = 2.0 * sqrtf(1.0 + rot._33 - rot._11 - rot._22);
      w = (rot._21 - rot._12) / s;
      x = (rot._13 + rot._31) / s;
      y = (rot._23 + rot._32) / s;
      z = 0.25 * s;

    }
#else
    if (trace > 0) {
      float s = 0.5 / sqrtf(trace + 1.0);
      w = 0.25 / s;
      x = (rot._23 - rot._32) * s;
      y = (rot._31 - rot._13) * s;
      z = (rot._12 - rot._21) * s;

    }
    else if (rot._11 > rot._22 && rot._11 > rot._33) {
      float s = 2.0 * sqrtf(1.0 + rot._11 - rot._22 - rot._33);
      w = (rot._23 - rot._32) / s;
      x = 0.25 * s;
      y = (rot._21 + rot._12) / s;
      z = (rot._31 + rot._13) / s;

    }
    else if (rot._22 > rot._33) {
      float s = 2.0 * sqrtf(1.0 + rot._22 - rot._11 - rot._33);
      w = (rot._31 - rot._13) / s;
      x = (rot._21 + rot._12) / s;
      y = 0.25 * s;
      z = (rot._32 + rot._23) / s;

    }
    else {
      float s = 2.0 * sqrtf(1.0 + rot._33 - rot._11 - rot._22);
      w = (rot._12 - rot._21) / s;
      x = (rot._31 + rot._13) / s;
      y = (rot._32 + rot._23) / s;
      z = 0.25 * s;

    }
#endif
  }

  void Quaternion::RotateX(float angle)
  {
    angle *= 0.5f;
    float s = sinf(angle);
    float c = cosf(angle);
    Quaternion n;
    n.x = w * s + x * c;
    n.y = z * s + y * c;
    n.z = -y * s + z * c;
    n.w = -x * s + w * c;
    *this = n;
  }

  void Quaternion::RotateY(float angle)
  {
    angle *= 0.5f;
    float s = sinf(angle);
    float c = cosf(angle);
    Quaternion n;
    n.x = -z * s + x * c;
    n.y = w * s + y * c;
    n.z = x * s + z * c;
    n.w = -y * s + w * c;
    *this = n;
  }

  void Quaternion::RotateZ(float angle)
  {
    angle *= 0.5f;
    float s = sinf(angle);
    float c = cosf(angle);
    Quaternion n;
    n.x = y * s + x * c;
    n.y = -x * s + y * c;
    n.z = w * s + z * c;
    n.w = -z * s + w * c;
    *this = n;
  }

  Quaternion Slerp(const Quaternion& a, Quaternion b, float n)
  {
    float dot = Dot(a, b);
    if (dot < 0) {
      dot = -dot;
      b.x = -b.x;
      b.y = -b.y;
      b.z = -b.z;
      b.w = -b.w;
    }
    float dots = acosf(dot);
    float fsin = sinf(dots);
    if (fsin > EPSILON || fsin < -EPSILON)
    {
      Quaternion q;
      float w0 = sinf((1 - n) * dots) / fsin;
      float w1 = sinf(n * dots) / fsin;
      q.x = a.x * w0 + b.x * w1;
      q.y = a.y * w0 + b.y * w1;
      q.z = a.z * w0 + b.z * w1;
      q.w = a.w * w0 + b.w * w1;
      return q;
    }
    else
    {
      return (n < 0.5f) ? a : b;
    }
  }
}
