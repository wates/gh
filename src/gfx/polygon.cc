
#include "polygon.h"
#include "../main/parser.h"
#include "../main/underscore.h"
#include <math.h>
#include <stdlib.h>
#ifdef _WIN32
#include <d3dx9.h>
#endif
using namespace std;
using namespace gh;

namespace xfile
{
  using namespace parser;

  typedef CharRange<'0', '9'> tNum;
  typedef CharRange<'1', '9'> tNonZeroNum;
  typedef Or<Rule<tNonZeroNum, Any<tNum> >, Char<'0'> > tUnsignedInt;
  typedef Rule<Option<Char<'+'> >, tUnsignedInt> tPositiveNum;
  typedef Rule<Char<'-'>, tUnsignedInt > tNegativeNum;
  typedef Or<tPositiveNum, tNegativeNum> tInt;

  typedef Or<tNum, CharRange<'a', 'f'>, CharRange<'A', 'F'> > tHex;

  typedef Rule<Char<'E', 'e'>, Option<Char<'+', '-'> >, More<tNum> > tLog;
  typedef Rule<tInt, Option<tLog> > tLogInt;

  typedef Rule<tInt, Char<'.'>, More<tNum>, Option<tLogInt> > tFloat;

  typedef More<Or<Char<0x20>, Char<0x09>, Char<0x0d>, Char<0x0a> > > tS;

  typedef Text<'x', 'o', 'f', ' '> tXof;
  typedef Text<'0', '3', '0', '3'> t0303;
  typedef Text<'0', '3', '0', '2'> t0302;
  typedef Text<'t', 'x', 't', ' '> tTxt;
  typedef Text<'0', '0', '3', '2'> t0032;
  typedef Text<'0', '0', '6', '4'> t0064;

  typedef Rule<tXof, Or<t0303, t0302>, tTxt, Or<t0032, t0064> > tFileHeader;

  //typedef Or<CharRange<'a','z'>,CharRange<'A','Z'>,Char<'_','<','>'> > tNameCharInitial;
  typedef Or<CharRange<'a', 'z'>, CharRange<'A', 'Z'>, Char<'_', '[', ']', '<', '>', '.', '-', '"'> > tNameCharInitial;
  typedef Or<CharRange<'0', '9'>, tNameCharInitial> tNameChar;
  typedef Rule<tNameCharInitial, Any<tNameChar> >  tName;

  struct tBlockType
    :public Action<tName>
  {
    std::string blocktype;
    void Hit(const char* text, type& r)
    {
      blocktype = text;
    }
  };

  struct tBlockName
    :public Action<tName>
  {
    string blockname;
    void Hit(const char* text, type& r)
    {
      blockname = text;
    }
  };

  enum ElementType
  {
    ET_Unknown,
    ET_Block,
    ET_String,
    ET_Int,
    ET_Float
  };

  struct Element
  {
    ElementType type;
    union
    {
      void* p;
      float f;
      int i;
    };
    Element()
    {
      type = ET_Unknown;
      p = 0;
    }
    explicit Element(const Element& cpy);
    void operator=(const Element& cpy);
    ~Element();
    operator float& ()
    {
      //if(type!=ET_Float)throw "bad_type";
      return f;
    }
    operator int& ()
    {
      //if(type!=ET_Int)throw "bad_type";
      return i;
    }
    operator string& ()
    {
      //if(type!=ET_String)throw "bad_type";
      return *(string*)p;
    }
    string& str()
    {
      //if(type!=ET_String)throw "bad_type";
      return *(string*)p;
    }
  private:

  };
  struct AnyElements
  {
    vector<Element> elements;
    bool Parse(const char*& text);
  };

  typedef Rule<
    Option<tS>,
    Option<tBlockType>,
    Option<Rule<tS, tBlockName> >,
    Option<tS>,
    Char<'{'>,
    AnyElements,
    Option<tS>,
    Char<'}'>
  > tBlock;

  bool AnyElements::Parse(const char*& text)
  {
    typedef Any<Or<tS, Char<';'>, Char<','> > > tDelim;
    struct tString
      :public Action<tName>
    {
      string str;
      void Hit(const char* text, type& r)
      {
        if (*text == '"')
        {
          str = text + 1;
          str.pop_back();
        }
        else
          str = text;
      }
    };
    struct tFloatElement
      :public Action<tFloat>
    {
      float f;
      void Hit(const char* text, type& r)
      {
        f = (float)atof(text);
      }
    };
    struct tIntElement
      :public Action<tInt>
    {
      int i;
      void Hit(const char* text, type& r)
      {
        i = atoi(text);
      }
    };

    tDelim delim;
    for (const char* start = text;; text = start)
    {
      if (delim.Parse(start))
      {
        Element& elem = _.push(elements);

        tBlock* block = new tBlock;
        if (block->Parse(start))
        {
          elem.type = ET_Block;
          elem.p = block;
          continue;
        }
        delete block;
        tFloatElement f;
        if (f.Parse(start))
        {
          elem.type = ET_Float;
          elem.f = f.f;
          continue;
        }
        tIntElement i;
        if (i.Parse(start))
        {
          elem.type = ET_Int;
          elem.i = i.i;
          continue;
        }
        tString str;
        if (str.Parse(start))
        {
          elem.type = ET_String;
          elem.p = new string(str.str.data());
          continue;
        }
        elements.pop_back();
      }
      text = start;
      break;
    }
    return true;
  }

  Element::Element(const Element& cpy)
  {
    type = cpy.type;
    if (type == ET_Block)
    {
      p = new tBlock;
      *(tBlock*)(p) = *(tBlock*)cpy.p;
    }
    if (type == ET_String)
      p = new string(*(string*)cpy.p);
    if (type == ET_Int)
      i = cpy.i;
    if (type == ET_Float)
      f = cpy.f;
  }

  void Element::operator=(const Element& cpy)
  {
    if (type == ET_Block)
      delete (tBlock*)p;
    else if (type == ET_String)
      delete (string*)p;
    type = cpy.type;
    if (type == ET_Block)
    {
      p = new tBlock;
      *(tBlock*)(p) = *(tBlock*)cpy.p;
    }
    if (type == ET_String)
      p = new string(*(string*)cpy.p);
    if (type == ET_Int)
      i = cpy.i;
    if (type == ET_Float)
      f = cpy.f;
  }

  Element::~Element()
  {
    if (type == ET_Block)
      delete (tBlock*)p;
    else if (type == ET_String)
      delete (string*)p;
  }

  typedef Rule<tFileHeader, AnyElements> tFormat;

  static void ReadBlock(tBlock& block, Bone& bone, AnimationSet& animation, void* prestate = 0)
  {
    vector<Element>& elem = block.elements;
    if (block.blocktype == "Frame")
    {
      Bone& child = _.push(bone.child);
      child.name = block.blockname;
      for (int i = 0; i < elem.size(); i++)
      {
        if (elem[i].type == ET_Block)
          ReadBlock(*(tBlock*)(elem[i].p), child, animation);
      }
      return;
    }
    if (block.blocktype == "FrameTransformMatrix")
    {
      bone.transform.Identity();
      for (int i = 0; i < 16; i++)
      {
        //bone.transform.m[i]=elem[i];
      }
    }

    //mesh
    struct MeshState
    {
      Shape* shape;
      bool hasnormal;
      vector<vector<int> > face;
      vector<vector<int> > normalface;
      vector<vector<int> > materialindices;
    }meshstate;
    if (block.blocktype == "Mesh")
    {
      Shape& shape = _.push(bone.shape);
      prestate = &meshstate;
      meshstate.shape = &shape;
      int n = 0;
      int positions = elem[n++];
      shape.position.resize(positions);
      for (int i = 0; i < positions; i++)
      {
        shape.position[i].x = elem[n++];
        shape.position[i].y = elem[n++];
        shape.position[i].z = elem[n++];
      }

      vector<vector<int> >& face = meshstate.face;
      int faces = elem[n++];
      face.resize(faces);
      for (int i = 0; i < faces; i++)
      {
        int verts = elem[n++];
        face[i].resize(verts);
        for (int j = 0; j < verts; j++)
          face[i][j] = (elem[n++]);
      }

      meshstate.hasnormal = false;

      for (int i = n; i < elem.size(); i++)
      {
        if (elem[i].type == ET_Block)
          ReadBlock(*(tBlock*)(elem[i].p), bone, animation, &meshstate);
      }
      //build mesh
      for (int i = 0; i < (int)shape.subset.size(); i++)
      {
        for (int j = 0; j < (int)meshstate.materialindices[i].size(); j++)
        {
          IndexVertex first, second, third;
          first.position = face[meshstate.materialindices[i][j]][0];
          if (meshstate.normalface.size())
            first.normal = meshstate.normalface[meshstate.materialindices[i][j]][0];
          else
            first.normal = -1;
          first.texture = first.position;
          first.color = -1;

          second.position = face[meshstate.materialindices[i][j]][1];
          if (meshstate.normalface.size())
            second.normal = meshstate.normalface[meshstate.materialindices[i][j]][1];
          else
            second.normal = -1;
          second.texture = second.position;
          second.color = -1;

          third.color = -1;
          for (int k = 2; k < (int)face[meshstate.materialindices[i][j]].size(); k++)
          {
            third.position = face[meshstate.materialindices[i][j]][k];
            if (meshstate.normalface.size())
              third.normal = meshstate.normalface[meshstate.materialindices[i][j]][k];
            else
              third.normal = -1;
            third.texture = third.position;
            shape.subset[i].indices.push_back(first);
            shape.subset[i].indices.push_back(second);
            shape.subset[i].indices.push_back(third);
            second = third;
          }
        }
      }
      return;
    }
    if (block.blocktype == "MeshTextureCoords")
    {
      MeshState& state = *(MeshState*)prestate;
      Shape& shape = *state.shape;
      vector<Element>& elem = block.elements;
      int n = 0;
      int coords = elem[n++];
      shape.texture.resize(coords);
      for (int i = 0; i < coords; i++)
      {
        shape.texture[i].x = elem[n++];
        shape.texture[i].y = elem[n++];
      }
    }
    if (block.blocktype == "MeshNormals")
    {
      MeshState& state = *(MeshState*)prestate;
      Shape& shape = *state.shape;
      state.hasnormal = true;
      vector<Element>& elem = block.elements;
      int n = 0;
      int normals = elem[n++];
      shape.normal.resize(normals);
      for (int i = 0; i < normals; i++)
      {
        shape.normal[i].x = elem[n++];
        shape.normal[i].y = elem[n++];
        shape.normal[i].z = elem[n++];
        shape.normal[i] *= -1;
      }
      int faces = elem[n++];
      state.normalface.resize(faces);
      for (int i = 0; i < faces; i++)
      {
        int verts = elem[n++];
        state.normalface[i].resize(verts);
        for (int j = 0; j < verts; j++)
          state.normalface[i][j] = (elem[n++]);
      }
    }
    if (block.blocktype == "SkinWeights")
    {
      MeshState& state = *(MeshState*)prestate;
      Shape& shape = *state.shape;
      vector<Element>& elem = block.elements;
      int n = 0;
      string targetbone = elem[n++];
      int count = elem[n++];
      Weight& weight = _.push(shape.weight);
      weight.bonename = targetbone;
      weight.position.resize(count);
      for (int i = 0; i < count; i++)
        weight.position[i].index = (int)elem[n++];
      for (int i = 0; i < count; i++)
        weight.position[i].weight = elem[n++];
      for (int i = 0; i < 16; i++)
        weight.offset.m[i] = elem[n++];
    }
    if (block.blocktype == "MeshMaterialList")
    {
      MeshState& state = *(MeshState*)prestate;
      Shape& shape = *state.shape;
      vector<Element>& elem = block.elements;
      int n = 0;
      int materials = elem[n++];
      int faces = elem[n++];
      state.materialindices.resize(materials);
      for (int i = 0; i < faces; i++)
        state.materialindices[(int)elem[n++]].push_back(i);

      //Material
      shape.subset.resize(materials);
      for (int i = 0; i < materials; i++)
      {
        tBlock& block = *(tBlock*)(elem[n++].p);
        vector<Element>& elem = block.elements;
        int n = 0;
        gh::Material& mat = shape.subset[i].material;
        mat.diffuse.r = elem[n++];
        mat.diffuse.g = elem[n++];
        mat.diffuse.b = elem[n++];
        mat.diffuse.a = elem[n++];
        mat.power = elem[n++];
        mat.ambient.r = elem[n++];
        mat.ambient.g = elem[n++];
        mat.ambient.b = elem[n++];
        mat.ambient.a = 1.0f;
        mat.specular.r = elem[n++];
        mat.specular.g = elem[n++];
        mat.specular.b = elem[n++];
        mat.specular.a = 1.0f;
        if ((int)elem.size() > n && elem[n].type == ET_Block)
        {
          tBlock& block = *(tBlock*)(elem[n++].p);
          vector<Element>& elem = block.elements;
          if (block.blocktype == "TextureFilename")
          {
            string tex = elem[0];
            if (tex.size())
            {
              if (tex[0] == '"')
              {
                mat.texture = tex.data() + 1;
                mat.texture.pop_back();
              }
              else
                mat.texture = tex;
            }
          }
        }
      }
    }

    //animation
    struct AnimationState
    {
      string name;
      AnimationBone* anim;
    };
    AnimationState animationstate;
    if (block.blocktype == "AnimationSet")
    {
      animationstate.name = block.blockname;
      prestate = &animationstate;
    }
    if (block.blocktype == "Animation")
    {
      AnimationState& state = *(AnimationState*)prestate;
      vector<Element>& elem = block.elements;
      state.anim = &_.push(animation[state.name].bone);
      for (int i = 0; i < elem.size(); i++)
        if (ET_Block == elem[i].type &&
          ((tBlock*)(elem[i].p))->blockname.size() == 0 &&
          ((tBlock*)(elem[i].p))->blocktype.size() == 0)
          state.anim->targetbone = ((tBlock*)(elem[i].p))->elements[0].str();
    }
    if (block.blocktype == "AnimationKey")
    {
      AnimationState& state = *(AnimationState*)prestate;
      vector<Element>& elem = block.elements;
      int n = 0;
      int type = elem[n++];
      int keys = elem[n++];
      float keyval[16];
      for (int i = 0; i < keys; i++)
      {
        int count = elem[n++];
        AnimationKey& key = state.anim->GetKeyFromCount(count);
        int vals = elem[n++];
        for (int j = 0; j < vals; j++)
          keyval[j] = elem[n++];
        if (type == 0 && vals == 4)//rotation
        {
          key.rotation.x = -keyval[1];
          key.rotation.y = -keyval[2];
          key.rotation.z = -keyval[3];
          key.rotation.w = keyval[0];
        }
        else if (type == 1 && vals == 3)//scale
        {
          key.scale[0] = keyval[0];
          key.scale[1] = keyval[1];
          key.scale[2] = keyval[2];
        }
        else if (type == 2 && vals == 4)
        {
          key.translation[0] = keyval[0];
          key.translation[1] = keyval[1];
          key.translation[2] = keyval[2];
        }
        else if (type == 4 && vals == 16)
        {
          key.scale.x = ((Vector3*)(&keyval[0]))->Magnitude();
          ((Vector3*)(&keyval[0]))->Normalize();
          key.scale.y = ((Vector3*)(&keyval[4]))->Magnitude();
          ((Vector3*)(&keyval[4]))->Normalize();
          key.scale.z = ((Vector3*)(&keyval[8]))->Magnitude();
          ((Vector3*)(&keyval[8]))->Normalize();
          key.translation.x = keyval[12];
          key.translation.y = keyval[13];
          key.translation.z = keyval[14];
          keyval[12] = 0;
          keyval[13] = 0;
          keyval[14] = 0;
          keyval[15] = 0;
#ifdef D3DX_DEFAULT
          D3DXQuaternionRotationMatrix((D3DXQUATERNION*)&key.rotation, (D3DXMATRIX*)keyval);
#else
#endif
        }
        else
          ;//assert
      }
    }


    for (int i = 0; i < elem.size(); i++)
      if (elem[i].type == ET_Block)
        ReadBlock(*(tBlock*)(elem[i].p), bone, animation, prestate);
  }
}

/*
namespace pmd
{
  struct Header
  {
    char ext[3];
    float version;
    char name[20];
    char description[256];
  };

  struct Vertex
  {
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
    uint16_t bone_index[2];
    uint8_t bone_weight;
    uint8_t is_edge;
  };

  struct Subset
  {
    Color diffuse;
    float power;
    Color specular;
    Color ambient;
    uint8_t toon;
    uint8_t flag;
    uint32_t face;
    char texture[20];
  };

  struct Bone
  {
    char name[20];
    uint16_t parent;
    uint16_t leaf;
    uint8_t type;
    uint16_t ik;
    Vector3 base;
  };

  struct Ik
  {
    uint16_t bone;
    uint16_t target;
    uint8_t length;
    uint16_t iteration;
    float limit_angle;
    typedef type_size_vector<uint16_t, uint8_t> _childtype;
    _childtype child;
  };

  struct Morph
  {
    struct Vertex {
      uint32_t index;
      Vector3 position;
    };
    char name[20];
    uint32_t count;
    uint8_t type;
    vector<Vertex> vertex;
  };

  struct SubName {
    char name[20];
  };

  struct BoneDispName {
    char name[50];
  };

  struct BoneDisp {
    uint16_t index;
    uint8_t frame;
  };

  struct EngHeaderName {
    uint8_t compat;
    char name[20];
    char comment[256];
  };

  struct ToonTexture {
    char filename[100];
  };

  struct RigidBody {
    char name[20];
    uint16_t bone_index; // 諸データ：関連ボーン番号 // 03 00 == 3 // 頭
    uint8_t group_index; // 諸データ：グループ // 00
    uint16_t group_target; // 諸データ：グループ：対象 // 0xFFFFとの差 // 38 FE
    uint8_t shape_type; // 形状：タイプ(0:球、1:箱、2:カプセル) // 00 // 球
    float shape_width; // 形状：半径(幅) // CD CC CC 3F // 1.6
    float shape_height; // 形状：高さ // CD CC CC 3D // 0.1
    float shape_depth; // 形状：奥行 // CD CC CC 3D // 0.1
    Vector3 pos; // 位置：位置(x, y, z)
    Vector3 rot; // 位置：回転(rad(x), rad(y), rad(z))
    float weight; // 諸データ：質量 // 00 00 80 3F // 1.0
    float pos_dim; // 諸データ：移動減 // 00 00 00 00
    float rot_dim; // 諸データ：回転減 // 00 00 00 00
    float recoil; // 諸データ：反発力 // 00 00 00 00
    float friction; // 諸データ：摩擦力 // 00 00 00 00
    uint8_t type; // 諸データ：タイプ(0:Bone追従、1:物理演算、2:物理演算(Bone位置合せ)) // 00 // Bone追従
  };

  struct Joint {
    char name[20];
    uint32_t rigidbody_a; // 諸データ：剛体A
    uint32_t rigidbody_b; // 諸データ：剛体B
    Vector3 pos; // 諸データ：位置(x, y, z) // 諸データ：位置合せでも設定可
    Vector3 rot; // 諸データ：回転(rad(x), rad(y), rad(z))
    Vector3 constrain_pos_1; // 制限：移動1(x, y, z)
    Vector3 constrain_pos_2; // 制限：移動2(x, y, z)
    Vector3 constrain_rot_1; // 制限：回転1(rad(x), rad(y), rad(z))
    Vector3 constrain_rot_2; // 制限：回転2(rad(x), rad(y), rad(z))
    Vector3 spring_pos; // ばね：移動(x, y, z)
    Vector3 spring_rot; // ばね：回転(rad(x), rad(y), rad(z))
  };

  struct Format
  {
    Header header;
    vector<Vertex> vertex;
    vector<uint16_t> face;
    vector<Subset> subset;
    type_size_vector<Bone, uint16_t> bone;
    type_size_vector<Ik, uint16_t> ik;
    type_size_vector<Morph, uint16_t> morph;
    type_size_vector<uint16_t, uint8_t> morph_disp;
    type_size_vector<BoneDispName, uint8_t> bone_disp_name;
    type_size_vector<BoneDisp, uint32_t> bone_disp;
    EngHeaderName eng_name;
    std::vector<SubName> eng_bone_name;
    std::vector<SubName> eng_morph_name;
    std::vector<BoneDispName> eng_bone_disp_name;
    std::vector<ToonTexture> toon_texture;
    type_size_vector<RigidBody, uint32_t> rigid;
    type_size_vector<Joint, uint32_t> joint;
  };
}

CONVERT_OBJECT_BEGIN(pmd::Header)
CONVERT_OBJECT_MEMBER(ext)
CONVERT_OBJECT_MEMBER(version)
CONVERT_OBJECT_MEMBER(name)
CONVERT_OBJECT_MEMBER(description)
CONVERT_OBJECT_END

CONVERT_OBJECT_BEGIN(pmd::Vertex)
CONVERT_OBJECT_MEMBER(position)
CONVERT_OBJECT_MEMBER(normal)
CONVERT_OBJECT_MEMBER(uv)
CONVERT_OBJECT_MEMBER(bone_index)
CONVERT_OBJECT_MEMBER(bone_weight)
CONVERT_OBJECT_MEMBER(is_edge)
CONVERT_OBJECT_END

CONVERT_OBJECT_BEGIN(pmd::Subset)
CONVERT_OBJECT_MEMBER(diffuse)
CONVERT_OBJECT_MEMBER(power)
CONVERT_OBJECT_MEMBER_TYPE(specular, Vector3)
CONVERT_OBJECT_MEMBER_TYPE(ambient, Vector3)
CONVERT_OBJECT_MEMBER(toon)
CONVERT_OBJECT_MEMBER(flag)
CONVERT_OBJECT_MEMBER(face)
CONVERT_OBJECT_MEMBER(texture)
CONVERT_OBJECT_END

CONVERT_OBJECT_2(pmd::Morph::Vertex, index, position)
CONVERT_OBJECT_1(pmd::SubName, name)
CONVERT_OBJECT_1(pmd::BoneDispName, name)
CONVERT_OBJECT_2(pmd::BoneDisp, index, frame)
CONVERT_OBJECT_3(pmd::EngHeaderName, compat, name, comment)
CONVERT_OBJECT_1(pmd::ToonTexture, filename)

CONVERT_OBJECT_BEGIN(pmd::RigidBody)
CONVERT_OBJECT_MEMBER(name)
CONVERT_OBJECT_MEMBER(bone_index)
CONVERT_OBJECT_MEMBER(group_index)
CONVERT_OBJECT_MEMBER(group_target)
CONVERT_OBJECT_MEMBER(shape_type)
CONVERT_OBJECT_MEMBER(shape_width)
CONVERT_OBJECT_MEMBER(shape_height)
CONVERT_OBJECT_MEMBER(shape_depth)
CONVERT_OBJECT_MEMBER(pos)
CONVERT_OBJECT_MEMBER(rot)
CONVERT_OBJECT_MEMBER(weight)
CONVERT_OBJECT_MEMBER(pos_dim)
CONVERT_OBJECT_MEMBER(rot_dim)
CONVERT_OBJECT_MEMBER(recoil)
CONVERT_OBJECT_MEMBER(friction)
CONVERT_OBJECT_MEMBER(type)
CONVERT_OBJECT_END

CONVERT_OBJECT_BEGIN(pmd::Joint)
CONVERT_OBJECT_MEMBER(name)
CONVERT_OBJECT_MEMBER(rigidbody_a)
CONVERT_OBJECT_MEMBER(rigidbody_b)
CONVERT_OBJECT_MEMBER(pos)
CONVERT_OBJECT_MEMBER(rot)
CONVERT_OBJECT_MEMBER(constrain_pos_1)
CONVERT_OBJECT_MEMBER(constrain_pos_2)
CONVERT_OBJECT_MEMBER(constrain_rot_1)
CONVERT_OBJECT_MEMBER(constrain_rot_2)
CONVERT_OBJECT_MEMBER(spring_pos)
CONVERT_OBJECT_MEMBER(spring_rot)
CONVERT_OBJECT_END

CONVERT_OBJECT_BEGIN(pmd::Bone)
CONVERT_OBJECT_MEMBER(name)
CONVERT_OBJECT_MEMBER(parent)
CONVERT_OBJECT_MEMBER(leaf)
CONVERT_OBJECT_MEMBER(type)
CONVERT_OBJECT_MEMBER(ik)
CONVERT_OBJECT_MEMBER(base)
CONVERT_OBJECT_END

namespace gh {

  bool Convert(BinaryReader& f, pmd::Ik& v, const char* info)
  {
    if (ObjectStart(f, info)
      && Convert(f, v.bone, "bone")
      && Convert(f, v.target, "target")
      && Convert(f, v.length, "length")
      && Convert(f, v.iteration, "iteration")
      && Convert(f, v.limit_angle, "limit_angle"))
    {
      v.child.resize(v.length);
      for (std::size_t i = 0; i < v.length; i++)
        if (!Convert(f, v.child[i], "child"))
          return false;
      return ObjectEnd(f);
    }
    else
      return false;
  }

  bool Convert(BinaryReader& f, pmd::Morph& v, const char* info)
  {
    if (ObjectStart(f, info)
      && Convert(f, v.name, "name")
      && Convert(f, v.count, "count")
      && Convert(f, v.type, "type")
      )
    {
      v.vertex.resize(v.count);
      for (std::size_t i = 0; i < v.count; i++)
        if (!Convert(f, v.vertex[i], "vertex"))
          return false;
      return ObjectEnd(f);
    }
    else
      return false;
  }

  template<typename T>
  bool ConvertStaticArray(BinaryReader& f, T& v, const char* info, int length) {
    v.resize(length);
    for (auto& c : v) {
      if (!Convert(f, c, info))
        return false;
    }
    return true;
  }
}


CONVERT_OBJECT_BEGIN(pmd::Format)
CONVERT_OBJECT_MEMBER(header)
CONVERT_OBJECT_MEMBER(vertex)
CONVERT_OBJECT_MEMBER(face)
CONVERT_OBJECT_MEMBER(subset)
CONVERT_OBJECT_MEMBER(bone)
CONVERT_OBJECT_MEMBER(ik)
CONVERT_OBJECT_MEMBER(morph)
CONVERT_OBJECT_MEMBER(morph_disp)
CONVERT_OBJECT_MEMBER(bone_disp_name)
CONVERT_OBJECT_MEMBER(bone_disp)
//ext1
CONVERT_OBJECT_MEMBER(eng_name)
&& ConvertStaticArray(f, value.eng_bone_name, "eng_bone_name", value.bone.size())
&& ConvertStaticArray(f, value.eng_morph_name, "eng_morph_name", value.morph.size() - 1)
&& ConvertStaticArray(f, value.eng_bone_disp_name, "eng_bone_disp_name", value.bone_disp_name.size())
//ext2
&& ConvertStaticArray(f, value.toon_texture, "toon_texture", 10)
//ext3
CONVERT_OBJECT_MEMBER(rigid)
CONVERT_OBJECT_MEMBER(joint)
CONVERT_OBJECT_END

namespace pmx
{
  struct PmxString
  {
    uint32_t length;
    std::string string;
  };

  struct GenericIndex
  {
    int v;
  };

  struct VertexIndex :public GenericIndex {};
  struct TextureIndex :public GenericIndex {};
  struct BoneIndex :public GenericIndex {};
  struct MaterialIndex :public GenericIndex {};
  struct MorphIndex :public GenericIndex {};
  struct RigidIndex :public GenericIndex {};

  struct Header
  {
    char ext[4];
    float version;
    char typesizes;
    char encode;
    char additional_uv;
    char vertex_size;
    char texture_uv_size;
    char material_size;
    char bone_size;
    char morph_size;
    char rigid_size;
  };

  struct Info {
    PmxString name;
    PmxString name_eng;
    PmxString comment;
    PmxString comment_eng;
  };

  struct Vertex
  {
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
    Vector4 additional_uv[4];
    uint8_t weight_type;
    VertexIndex weight_index[4];
    float weight[4];
    float edge;
  };

  struct Subset
  {
    PmxString name;
    PmxString name_eng;
    Vector4 diffuse;
    Vector3 specular;
    float power;
    Vector3 ambient;
    uint8_t flag;
    Vector4 edge_color;
    float edge_size;
    TextureIndex decal;
    TextureIndex sphere;
    uint8_t sphere_mode;
    uint8_t share_toon;
    TextureIndex toon;
    uint8_t share_toon_index;
    PmxString memo;
    uint32_t face;
  };


  struct IkLink {
    BoneIndex link;
    uint8_t limit;
    Vector3 limit_max;
    Vector3 limit_min;
  };
  struct Ik
  {
    BoneIndex target;
    int32_t iteration;
    float limit_angle;
    vector<IkLink> links;
  };

  struct Bone
  {
    PmxString name;
    PmxString name_eng;
    Vector3 base;
    BoneIndex parent;
    int32_t layer;
    uint16_t flag;

    Vector3 position;
    BoneIndex parent_pos;

    BoneIndex parent_rot;
    float weight;

    Vector3 fixed_axis;

    Vector3 local_axis_x;
    Vector3 local_axis_z;

    int32_t key;

    Ik ik;
  };

  struct Morph
  {
    struct Vertex {
      VertexIndex index;
      Vector3 position;
    };
    struct UV {
      VertexIndex index;
      Vector4 offset;
    };
    struct Bone {
      BoneIndex index;
      Vector3 position;
      Quaternion rotation;
    };
    struct Material {
      MaterialIndex index;
      uint8_t offset;
      Vector4 diffuse;
      Vector3 specular;
      float power;
      Vector3 ambient;
      Vector4 edge_color;
      float edge_size;
      Vector4 texture;
      Vector4 sphere;
      Vector4 toon;
    };
    struct Group {
      MorphIndex index;
      float rate;
    };
    struct Offset {
      Vertex vertex;
      UV uv;
      Bone bone;
      Material material;
      Group group;
    };
    PmxString name;
    PmxString name_eng;
    uint8_t control_panel;
    uint8_t offset_kind;
    uint32_t offset_count;
    vector<Offset> offset;
  };

  struct Disp {
    struct Index {
      uint8_t bone_or_morph;
      BoneIndex bone;
      MorphIndex morph;
    };
    PmxString name;
    PmxString name_eng;
    uint8_t special;
    vector<Index> index;
  };

  struct RigidBody {
    PmxString name;
    PmxString name_eng;
    BoneIndex bone_index;
    uint8_t group_index;
    uint16_t group_target;
    uint8_t shape_type;
    float shape_width;
    float shape_height;
    float shape_depth;
    Vector3 pos;
    Vector3 rot;
    float weight;
    float pos_dim;
    float rot_dim;
    float recoil;
    float friction;
    uint8_t type;
  };

  struct Joint {
    PmxString name;
    PmxString name_eng;
    uint8_t kind;
    RigidIndex rigidbody_a;
    RigidIndex rigidbody_b;
    Vector3 pos;
    Vector3 rot;
    Vector3 constrain_pos_1;
    Vector3 constrain_pos_2;
    Vector3 constrain_rot_1;
    Vector3 constrain_rot_2;
    Vector3 spring_pos;
    Vector3 spring_rot;
  };

  struct Format
  {
    Header header;
    Info info;
    vector<Vertex> vertex;
    vector<VertexIndex> face;
    vector<PmxString> texture;
    vector<Subset> subset;
    vector<Bone> bone;
    vector<Morph> morph;
    vector<Disp> disp;
    vector<RigidBody> rigid;
    vector<Joint> joint;
  };
}

template<typename T>inline bool Convert(BinaryReader& c, std::vector<T>& v, const char* info, pmx::Format& fmt);

template<typename C = char, typename S = int16_t, typename L = int32_t>
bool Convert(gh::BinaryReader& f, pmx::GenericIndex& value, const char* info, pmx::Format& fmt, int sizeinfo) {
  if (1 == sizeinfo) {
    C i;
    if (!Convert(f, i, "index"))return false;
    value.v = i;
  }
  else if (2 == sizeinfo) {
    S i;
    if (!Convert(f, i, "index"))return false;
    value.v = i;
  }
  else if (4 == sizeinfo) {
    L i;
    if (!Convert(f, i, "index"))return false;
    value.v = i;
  }
  return true;
}

bool Convert(gh::BinaryReader& f, pmx::VertexIndex& value, const char* info, pmx::Format& fmt) {
  return Convert<uint8_t, uint16_t, uint32_t>(f, value, info, fmt, fmt.header.vertex_size);
}

bool Convert(gh::BinaryReader& f, pmx::TextureIndex& value, const char* info, pmx::Format& fmt) {
  return Convert(f, value, info, fmt, fmt.header.texture_uv_size);
}

bool Convert(gh::BinaryReader& f, pmx::BoneIndex& value, const char* info, pmx::Format& fmt) {
  return Convert(f, value, info, fmt, fmt.header.bone_size);
}

bool Convert(gh::BinaryReader& f, pmx::MaterialIndex& value, const char* info, pmx::Format& fmt) {
  return Convert(f, value, info, fmt, fmt.header.material_size);
}

bool Convert(gh::BinaryReader& f, pmx::MorphIndex& value, const char* info, pmx::Format& fmt) {
  return Convert(f, value, info, fmt, fmt.header.morph_size);
}

bool Convert(gh::BinaryReader& f, pmx::RigidIndex& value, const char* info, pmx::Format& fmt) {
  return Convert(f, value, info, fmt, fmt.header.rigid_size);
}


//#include "jpncode.h"

bool Convert(gh::BinaryReader& f, pmx::PmxString& value, const char* info, pmx::Format& fmt)
{
  if (fmt.header.encode) {
    std::vector<char> str;
    if (Convert(f, str, "str")) {
      value.length = str.size();
      value.string.assign(str.begin(), str.end());
      return true;
    }
  }
  else {
    std::vector<uint8_t> str;
    if (Convert(f, str, "str")) {
      value.length = str.size();
      std::vector<jpncode::unicode> ws;
      ws.resize(value.length / 2+1);
      for (int i = 0; i < value.length / 2; i++) {
        ws[i] = reinterpret_cast<uint16_t*>(str.data())[i];
      }
      ws.push_back(0);

      auto len = jpncode::sjis_multibyte_charactors(ws.data());
      char *mbs = new char[len.charactors + 1];
      mbs[len.charactors] = 0;
      jpncode::sjis_encode(ws.data(), mbs);

      value.string = mbs;
      delete[]mbs;
      //LogPut(value.string.c_str());
      return true;
    }
  }
  return false;
}

CONVERT_OBJECT_BEGIN(pmx::Header)
CONVERT_OBJECT_MEMBER(ext)
CONVERT_OBJECT_MEMBER(version)
CONVERT_OBJECT_MEMBER(typesizes)
CONVERT_OBJECT_MEMBER(encode)
CONVERT_OBJECT_MEMBER(additional_uv)
CONVERT_OBJECT_MEMBER(vertex_size)
CONVERT_OBJECT_MEMBER(texture_uv_size)
CONVERT_OBJECT_MEMBER(material_size)
CONVERT_OBJECT_MEMBER(bone_size)
CONVERT_OBJECT_MEMBER(morph_size)
CONVERT_OBJECT_MEMBER(rigid_size)
CONVERT_OBJECT_END

bool Convert(gh::BinaryReader& f, pmx::Info& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.name, "name", fmt)
    && Convert(f, value.name_eng, "name_eng", fmt)
    && Convert(f, value.comment, "comment", fmt)
    && Convert(f, value.comment_eng, "comment_eng", fmt)) {
    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::Vertex& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    CONVERT_OBJECT_MEMBER(position)
    CONVERT_OBJECT_MEMBER(normal)
    CONVERT_OBJECT_MEMBER(uv))
  {
    for (std::size_t i = 0; i < fmt.header.additional_uv; i++)
      if (!Convert(f, value.additional_uv[i], "additinal_uv"))
        return false;
    if (!Convert(f, value.weight_type, "weight_type"))
      return false;
    if (0 == value.weight_type) {
      if (!Convert(f, value.weight_index[0], "weight_index", fmt))
        return false;
    }
    else if (1 == value.weight_type) {
      if (!Convert(f, value.weight_index[0], "weight_index", fmt)
        || !Convert(f, value.weight_index[1], "weight_index", fmt)
        || !Convert(f, value.weight[0], "weight"))
        return false;
    }
    else if (2 == value.weight_type) {
      if (!Convert(f, value.weight_index[0], "weight_index", fmt)
        || !Convert(f, value.weight_index[1], "weight_index", fmt)
        || !Convert(f, value.weight_index[2], "weight_index", fmt)
        || !Convert(f, value.weight_index[3], "weight_index", fmt)
        || !Convert(f, value.weight[0], "weight")
        || !Convert(f, value.weight[1], "weight")
        || !Convert(f, value.weight[2], "weight")
        || !Convert(f, value.weight[3], "weight")
        )return false;
    }
    else if (3 == value.weight_type) {
      gh::Vector3 c, r0, r1;
      if (!Convert(f, value.weight_index[0], "weight_index", fmt)
        || !Convert(f, value.weight_index[1], "weight_index", fmt)
        || !Convert(f, value.weight[0], "weight")
        || !Convert(f, c, "c")
        || !Convert(f, r0, "r0")
        || !Convert(f, r1, "r1")
        )
        return false;
      value.weight_type = 1;
    }
    if (!Convert(f, value.edge, "edge"))
      return false;

    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::Subset& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.name, "name", fmt)
    && Convert(f, value.name_eng, "name_eng", fmt)
    CONVERT_OBJECT_MEMBER(diffuse)
    CONVERT_OBJECT_MEMBER(specular)
    CONVERT_OBJECT_MEMBER(power)
    CONVERT_OBJECT_MEMBER(ambient)
    CONVERT_OBJECT_MEMBER(flag)
    CONVERT_OBJECT_MEMBER(edge_color)
    CONVERT_OBJECT_MEMBER(edge_size)
    && Convert(f, value.decal, "decal", fmt)
    && Convert(f, value.sphere, "sphere", fmt)
    CONVERT_OBJECT_MEMBER(sphere_mode)
    CONVERT_OBJECT_MEMBER(share_toon)
    && (0 == value.share_toon ?
      Convert(f, value.toon, "toon", fmt) :
      Convert(f, value.share_toon_index, "share_toon_index"))
    && Convert(f, value.memo, "memo", fmt)
    CONVERT_OBJECT_MEMBER(face)) {
    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::IkLink& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.link, "target", fmt)
    CONVERT_OBJECT_MEMBER(limit)
    && 1 == value.limit ? true
    CONVERT_OBJECT_MEMBER(limit_max)
    CONVERT_OBJECT_MEMBER(limit_min)
    : true) {
    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::Ik& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.target, "target", fmt)
    CONVERT_OBJECT_MEMBER(iteration)
    CONVERT_OBJECT_MEMBER(limit_angle)
    && Convert(f, value.links, "links", fmt)
    ) {
    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::Bone& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.name, "name", fmt)
    && Convert(f, value.name_eng, "name_eng", fmt)
    CONVERT_OBJECT_MEMBER(base)
    && Convert(f, value.parent, "parent", fmt)
    CONVERT_OBJECT_MEMBER(layer)
    CONVERT_OBJECT_MEMBER(flag)
    && ((0x0001 & value.flag) ?
      Convert(f, value.parent_pos, "parent_pos", fmt) :
      Convert(f, value.position, "position"))
    && ((0x0300 & value.flag) ?
      Convert(f, value.parent_rot, "parent_rot", fmt)
      CONVERT_OBJECT_MEMBER(weight) : true)
    && ((0x0400 & value.flag) ?
      Convert(f, value.fixed_axis, "fixed_axis") : true)
    && ((0x0800 & value.flag) ?
      Convert(f, value.local_axis_x, "loval_axis_x")
      CONVERT_OBJECT_MEMBER(local_axis_z)
      : true)
    && ((0x2000 & value.flag) ?
      Convert(f, value.key, "key")
      : true)
    && ((0x0020 & value.flag) ?
      Convert(f, value.ik, "ik", fmt) : true)
    ) {
    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::Morph::Group& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.index, "index", fmt)
    CONVERT_OBJECT_MEMBER(rate)
    ) {
    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::Morph::Vertex& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.index, "index", fmt)
    CONVERT_OBJECT_MEMBER(position)
    ) {
    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::Morph::Bone& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.index, "index", fmt)
    CONVERT_OBJECT_MEMBER(position)
    CONVERT_OBJECT_MEMBER(rotation)
    ) {
    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::Morph::UV& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.index, "index", fmt)
    CONVERT_OBJECT_MEMBER(offset)
    ) {
    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::Morph::Material& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.index, "index", fmt)
    CONVERT_OBJECT_MEMBER(offset)
    CONVERT_OBJECT_MEMBER(diffuse)
    CONVERT_OBJECT_MEMBER(specular)
    CONVERT_OBJECT_MEMBER(power)
    CONVERT_OBJECT_MEMBER(ambient)
    CONVERT_OBJECT_MEMBER(edge_color)
    CONVERT_OBJECT_MEMBER(edge_size)
    CONVERT_OBJECT_MEMBER(texture)
    CONVERT_OBJECT_MEMBER(sphere)
    CONVERT_OBJECT_MEMBER(toon)
    ) {
    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::Morph& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.name, "name", fmt)
    && Convert(f, value.name_eng, "name_eng", fmt)
    CONVERT_OBJECT_MEMBER(control_panel)
    CONVERT_OBJECT_MEMBER(offset_kind)
    CONVERT_OBJECT_MEMBER(offset_count)
    ) {
    value.offset.resize(value.offset_count);
    for (auto& i : value.offset) {
      if (0 == value.offset_kind && !Convert(f, i.group, "group", fmt)) {
        return false;
      }
      else if (1 == value.offset_kind && !Convert(f, i.vertex, "vertex", fmt)) {
        return false;
      }
      else if (2 == value.offset_kind && !Convert(f, i.bone, "bone", fmt)) {
        return false;
      }
      else if ((3 <= value.offset_kind && 7 >= value.offset_kind) && !Convert(f, i.uv, "uv", fmt)) {
        return false;
      }
      else if (8 == value.offset_kind && !Convert(f, i.material, "material", fmt)) {
        return false;
      }
    }
    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::Disp::Index& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    CONVERT_OBJECT_MEMBER(bone_or_morph)
    && value.bone_or_morph ?
    Convert(f, value.morph, "morph", fmt) :
    Convert(f, value.bone, "bone", fmt)
    ) {
    return gh::ObjectEnd(f);
  }
  return false;
}


bool Convert(gh::BinaryReader& f, pmx::Disp& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.name, "name", fmt)
    && Convert(f, value.name_eng, "name_eng", fmt)
    CONVERT_OBJECT_MEMBER(special)
    && Convert(f, value.index, "index", fmt)
    ) {
    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::RigidBody& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.name, "name", fmt)
    && Convert(f, value.name_eng, "name_eng", fmt)
    && Convert(f, value.bone_index, "bone_index", fmt)
    CONVERT_OBJECT_MEMBER(group_index)
    CONVERT_OBJECT_MEMBER(group_target)
    CONVERT_OBJECT_MEMBER(shape_type)
    CONVERT_OBJECT_MEMBER(shape_width)
    CONVERT_OBJECT_MEMBER(shape_height)
    CONVERT_OBJECT_MEMBER(shape_depth)
    CONVERT_OBJECT_MEMBER(pos)
    CONVERT_OBJECT_MEMBER(rot)
    CONVERT_OBJECT_MEMBER(weight)
    CONVERT_OBJECT_MEMBER(pos_dim)
    CONVERT_OBJECT_MEMBER(rot_dim)
    CONVERT_OBJECT_MEMBER(recoil)
    CONVERT_OBJECT_MEMBER(friction)
    CONVERT_OBJECT_MEMBER(type)
    ) {
    return gh::ObjectEnd(f);
  }
  return false;
}

bool Convert(gh::BinaryReader& f, pmx::Joint& value, const char* info, pmx::Format& fmt)
{
  if (gh::ObjectStart(f, info)
    && Convert(f, value.name, "name", fmt)
    && Convert(f, value.name_eng, "name_eng", fmt)
    CONVERT_OBJECT_MEMBER(kind)
    && Convert(f, value.rigidbody_a, "rigidbody_a", fmt)
    && Convert(f, value.rigidbody_b, "rigidbody_b", fmt)
    CONVERT_OBJECT_MEMBER(pos)
    CONVERT_OBJECT_MEMBER(rot)
    CONVERT_OBJECT_MEMBER(constrain_pos_1)
    CONVERT_OBJECT_MEMBER(constrain_pos_2)
    CONVERT_OBJECT_MEMBER(constrain_rot_1)
    CONVERT_OBJECT_MEMBER(constrain_rot_2)
    CONVERT_OBJECT_MEMBER(spring_pos)
    CONVERT_OBJECT_MEMBER(spring_rot)
    ) {
    return gh::ObjectEnd(f);
  }
  return false;
}

template<typename T>inline bool Convert(BinaryReader& c, std::vector<T>& v, const char* info, pmx::Format& fmt)
{
  uint32_t count;
  if (!Convert(c, count, info))
    return false;
  v.resize(count);
  for (uint32_t i = 0; i < count; i++)
    if (!Convert(c, v[i], info, fmt))
      return false;
  return true;
}

CONVERT_OBJECT_BEGIN(pmx::Format)
CONVERT_OBJECT_MEMBER(header)
&& ::Convert(f, value.info, "info", value)
&& ::Convert(f, value.vertex, "vertex", value)
&& ::Convert(f, value.face, "face", value)
&& ::Convert(f, value.texture, "texture", value)
&& ::Convert(f, value.subset, "subset", value)
&& ::Convert(f, value.bone, "bone", value)
&& ::Convert(f, value.morph, "morph", value)
&& ::Convert(f, value.disp, "disp", value)
&& ::Convert(f, value.rigid, "rigid", value)
&& ::Convert(f, value.joint, "joint", value)
CONVERT_OBJECT_END

*/

namespace vmd
{
  struct Header
  {
    char name[30];
    char model[20];
  };

  struct Key
  {
    char target_bone[15];
    unsigned int elapsed;
    Vector3 translation;
    Quaternion rotation;
    unsigned char interpolation[64];
  };

  struct Morph
  {
    char name[15];
    uint32_t frame;
    float weight;
  };

  int KeyCmp(const void* _a, const void* _b)
  {
    const Key* a = (const Key*)_a;
    const Key* b = (const Key*)_b;
    return a->elapsed - b->elapsed;
  }

  struct Format
  {
    Header header;
    vector<Key> key;
    vector<Morph> morph;
  };
}

CONVERT_OBJECT_BEGIN(vmd::Header)
CONVERT_OBJECT_MEMBER(name)
CONVERT_OBJECT_MEMBER(model)
CONVERT_OBJECT_END

CONVERT_OBJECT_BEGIN(vmd::Key)
CONVERT_OBJECT_MEMBER(target_bone)
CONVERT_OBJECT_MEMBER(elapsed)
CONVERT_OBJECT_MEMBER(translation)
CONVERT_OBJECT_MEMBER(rotation)
CONVERT_OBJECT_MEMBER(interpolation)
CONVERT_OBJECT_END

CONVERT_OBJECT_3(vmd::Morph, name, frame, weight)

CONVERT_OBJECT_BEGIN(vmd::Format)
CONVERT_OBJECT_MEMBER(header)
CONVERT_OBJECT_MEMBER(key)
CONVERT_OBJECT_MEMBER(morph)
CONVERT_OBJECT_END


namespace vrm {
  struct Header {
    char glTF[4];
    char type[4];
    uint32_t length;
  };
  struct Chunk {
    uint32_t length;
    char type[4];
    std::vector<uint8_t> data;
  };
  struct Format {
    Header header;
    Chunk chunk1;
    Chunk chunk2;
  };
}

CONVERT_OBJECT_BEGIN(vrm::Header)
CONVERT_OBJECT_MEMBER(glTF)
CONVERT_OBJECT_MEMBER(type)
CONVERT_OBJECT_MEMBER(length)
CONVERT_OBJECT_END

namespace gh {
  bool Convert(gh::BinaryReader& f, vrm::Chunk& v, const char* info)
  {
    gh::ObjectStart(f, info);

    if (!Convert(f, v.length, "length"))
      return false;
    if (!Convert(f, v.type, "type"))
      return false;
    v.data.resize(v.length);
    for (uint32_t i = 0; i < v.length; i++)
      if (!Convert(f, v.data[i], "data"))
        return false;
    return gh::ObjectEnd(f);
  }
}

CONVERT_OBJECT_BEGIN(vrm::Format)
CONVERT_OBJECT_MEMBER(header)
CONVERT_OBJECT_MEMBER(chunk1)
CONVERT_OBJECT_MEMBER(chunk2)
CONVERT_OBJECT_END

#include <json11.hpp>
#include <set>
#include <cstdlib>

namespace gh {

  bool ParseVRM(const char* buf, int len, Bone& root)
  {
    root.absolute.Identity();
    root.transform.Identity();
    root.name = "root";

    BinaryReader reader = { buf, buf + len };

    vrm::Format fmt;

    if (!Convert(reader, fmt, ""))
      return false;
    std::string err;
    fmt.chunk1.data.push_back('\0');

    struct Jsonp {
      struct Jp {
        const json11::Json j;
        Jp(const json11::Json i) :j(i) {
        }
        Jp(const Jp& i) :j(i.j) {
        }
        int i()const {
          return j.int_value();
        }
        float f()const {
          return j.number_value();
        }
        const std::string s()const {
          return j.string_value();
        }
        decltype(j.array_items()) a() {
          return j.array_items();
        }
        bool has(const char* key) {
          auto& o = j.object_items();
          return o.end() != o.find(key);
        }
        Jp operator[](int n) {
          return j.array_items()[n];
        }
        Jp operator[](const char* key) {
          return j.object_items().find(key)->second;
        }
      };
      Jp operator()(const json11::Json& j) {
        return j;
      }
      Jp operator()(const json11::Json* j) {
        return *j;
      }
    }J;

    struct bufferView {
      uint8_t* buf;
      uint32_t length;
      int target;
    };
    struct accessor {
      bufferView* buf;
      int offset;
      std::string type;
      int componentType;
      //#define GL_BYTE                           0x1400  5120
      //#define GL_UNSIGNED_BYTE                  0x1401  5121
      //#define GL_SHORT                          0x1402  5122
      //#define GL_UNSIGNED_SHORT                 0x1403  5123
      //#define GL_INT                            0x1404  5124
      //#define GL_UNSIGNED_INT                   0x1405  5125
      //#define GL_FLOAT                          0x1406  5126
      int count;
      bool normalized;
      bool fetch(int offset, void* out) {
        int size;
        int len;
        if (componentType == 5120 || componentType == 5121) {
          size = 1;
        }
        else if (componentType == 5122 || componentType == 5123) {
          size = 2;
        }
        else if (componentType == 5124 || componentType == 5125 || componentType == 5126) {
          size = 4;
        }
        else return false;
        if (type == "SCALAR") {
          len = 1;
        }
        else if (type == "VEC2") {
          len = 2;
        }
        else if (type == "VEC3") {
          len = 3;
        }
        else if (type == "VEC4") {
          len = 4;
        }
        else if (type == "MAT4") {
          len = 16;
        }
        else return false;
        memcpy(out, buf->buf + offset * len * size, size * len);
        return true;
      }

      int SCALAR(int n) {
        return reinterpret_cast<int*>(buf->buf)[n];
      }
      Vector3 VEC3(int n) {
        return reinterpret_cast<Vector3*>(buf->buf)[n];
      }
      Vector2 VEC2(int n) {
        return reinterpret_cast<Vector2*>(buf->buf)[n];
      }
      Vector4 VEC4(int n) {
        return reinterpret_cast<Vector4*>(buf->buf)[n];
      }
      Matrix MAT4(int n) {
        return reinterpret_cast<Matrix*>(buf->buf)[n];
      }
    };
    struct Material {
      std::string decal;
      Color color;
    };
    std::vector<Material> materials;
    std::vector<bufferView> bufferViews;
    std::vector<accessor> accessors;
    std::vector<std::string> images;
    //WriteFile("rea1.json", fmt.chunk.data);
    auto& buffer = fmt.chunk2.data;
    auto json = json11::Json::parse((char*)fmt.chunk1.data.data(), err);
    auto& top = json.object_items();
    for (auto& i : top) {
      //JSON のフィールド順によっては処理できないパターンがある
      //PNG とかテクスチャ処理時に解決できない
      if (i.first == "bufferViews") {
        //名前がついてないので参照されるときは配列のインデックス
        for (auto& j : i.second.array_items()) {
          auto& o = j.object_items();
          bufferView bv;
          bv.buf = buffer.data() + o.find("byteOffset")->second.int_value();
          bv.length = o.find("byteLength")->second.int_value();
          bv.target = o.find("target") == o.end() ? 0 : o.find("target")->second.int_value();
          bufferViews.push_back(bv);
        }
      }
      if (i.first == "images") {
        for (auto& j : i.second.array_items()) {
          auto& o = j.object_items();
          int bv = o.find("bufferView")->second.int_value();
          std::string name = o.find("name")->second.string_value();;
          name += ".png";
        }
      }
    }
    root.absolute.Identity();
    root.transform.Identity();
    root.transform.Scale(-1, 1, 1);
    root.name = "root";
    root.ik.head.clear();

    decltype(top.find("")) i;
    if (top.end() != (i = top.find("accessors"))) {
      for (auto& j : i->second.array_items()) {
        auto& o = j.object_items();
        accessor a;
        a.buf = &bufferViews[o.find("bufferView")->second.int_value()];
        a.offset = o.find("byteOffset")->second.int_value();
        a.type = o.find("type")->second.string_value();
        a.componentType = o.find("componentType")->second.int_value();
        a.count = o.find("count")->second.int_value();
        a.normalized = o.find("count")->second.bool_value();
        accessors.push_back(a);
      }
    }

    std::vector<std::string> images_index;

    if (top.end() != (i = top.find("images"))) {
      for (auto& j : i->second.array_items()) {
        auto& o = j.object_items();
        std::string name = o.find("name")->second.string_value();;
        name += ".png";
        auto& buf = bufferViews[o.find("bufferView")->second.int_value()];
        uint8_t* t = new uint8_t[buf.length];
        memcpy(t, buf.buf, buf.length);
        assert("not implement alias");
        //gh::SetFileAlias(name.c_str(), t, buf.length);
        images_index.push_back(name);
      }
    }


    if (top.end() != (i = top.find("materials"))) {
      for (auto& j : i->second.array_items()) {
        auto& o = j.object_items();
        Material m;
        if (J(o)["pbrMetallicRoughness"].has("baseColorTexture")) {
          auto n = J(o)["pbrMetallicRoughness"]["baseColorTexture"]["index"].i();
          m.decal = images_index[n];
        }
        if (J(o)["pbrMetallicRoughness"].has("baseColorFactor")) {
          auto n = J(o)["pbrMetallicRoughness"]["baseColorFactor"];
          m.color.r = n[0].f();
          m.color.g = n[1].f();
          m.color.b = n[2].f();
          m.color.a = n[3].f();
        }
        if (J(o)["pbrMetallicRoughness"].has("metallicFactor")) {
          auto n = J(o)["pbrMetallicRoughness"]["metallicFactor"];
        }
        if (J(o)["pbrMetallicRoughness"].has("roughnessFactor")) {
          auto n = J(o)["pbrMetallicRoughness"]["roughnessFactor"];
        }
        materials.push_back(m);
      }

    }

    if (top.end() != (i = top.find("nodes"))) {
      auto& nodes = i->second.array_items();
      std::vector<pair<int, string>> node_stack;
      node_stack.push_back({ 0,"root" });
      while (node_stack.size()) {
        auto n = node_stack[0];
        node_stack.erase(node_stack.begin());
        auto parent = root.GetChildFromName(n.second);
        auto& bn = _.push(parent->child);
        bn.name = J(nodes[n.first])["name"].s();
        bn.transform.Identity();
        bn.inverse.Identity();
        bn.transform.Translate(
          J(nodes[n.first])["translation"][0].f(),
          J(nodes[n.first])["translation"][1].f(),
          J(nodes[n.first])["translation"][2].f()
        );
        if (J(nodes[n.first]).has("children")) {
          for (auto& c : J(nodes[n.first])["children"].j.array_items()) {
            node_stack.push_back({ c.int_value() ,bn.name });
          }
        }


      }
    }

    std::vector<std::string> mesh_name_index;
    if (top.end() != (i = top.find("meshes"))) {
      mesh_name_index.resize(i->second.array_items().size());
    }
    if (top.end() != (i = top.find("nodes"))) {
      for (auto& j : i->second.array_items()) {
        auto o = j.object_items();
        auto m = o.find("mesh");
        if (o.end() != m) {
          auto name = J(j)["name"].s();
          mesh_name_index[m->second.int_value()] = name;
          Bone* bt = root.GetChildFromName(name);
          _.push(bt->shape);
        }
      }
    }

    std::vector<std::string> skin_name_index;
    if (top.end() != (i = top.find("skins"))) {
      skin_name_index.resize(i->second.array_items().size());
    }
    if (top.end() != (i = top.find("nodes"))) {
      for (auto& j : i->second.array_items()) {
        auto o = j.object_items();
        auto m = o.find("skin");
        if (o.end() != m) {
          skin_name_index[m->second.int_value()] = J(j)["name"].s();
        }
      }
    }

    std::vector<std::string> nodes_name_index;
    if (top.end() != (i = top.find("nodes"))) {
      for (auto& j : i->second.array_items()) {
        nodes_name_index.push_back(J(j)["name"].s());
      }
    }

    if (top.end() != (i = top.find("skins"))) {
      int skin_num = 0;
      for (auto& j : i->second.array_items()) {
        auto o = j.object_items();
        int bind = J(o)["inverseBindMatrices"].i();
        int skeleton = J(o)["skeleton"].i();
        std::vector<Matrix> imat;
        auto& bind_acc = accessors[bind];
        imat.resize(bind_acc.count);
        Bone* t = root.GetChildFromName(skin_name_index[skin_num]);
        if (t) {
          int b = 0;
          auto& sh = t->shape[0];//TODO 複数 Mesh 対応
          for (auto& j : J(o)["joints"].a()) {
            //このへんの参照がインデックスなのでつらい
            auto& name = nodes_name_index[j.int_value()];
            Bone* bt = root.GetChildFromName(name);
            bind_acc.fetch(b, &bt->inverse);
            auto& w = _.push(sh.weight);
            w.bonename = name;
            //w.offset.Identity();
            w.offset = bt->inverse;
            b++;
          }
        }
        else assert(0);
        skin_num++;
      }
    }

    std::map<int, bool> is_weight_loaded;

    if (top.end() != (i = top.find("meshes"))) {
      int mesh_num = 0;
      for (auto& j : i->second.array_items()) {
        auto o = j.object_items();
        auto pr = o.find("primitives");
        auto name = J(o)["name"].s();
        Bone* bt = root.GetChildFromName(mesh_name_index[mesh_num]);
        Shape& sh = bt->shape[0];
        for (auto& pa : pr->second.array_items()) {
          auto& p = pa.object_items();
          Subset& subset = _.push(sh.subset);

          auto vtx = J(p)["attributes"];
          if (vtx.has("POSITION")) {
            auto pos_acc = accessors[vtx["POSITION"].i()];
            int b = 0;
            sh.position.resize(pos_acc.count);
            for (auto& v : sh.position) {
              v = pos_acc.VEC3(b++);
            }
          }
          if (vtx.has("NORMAL")) {
            auto& pos_acc = accessors[vtx["NORMAL"].i()];
            int b = 0;
            sh.normal.resize(pos_acc.count);
            for (auto& v : sh.normal) {
              v = pos_acc.VEC3(b++);
            }
          }
          bool has_texcoord = false;
          if (vtx.has("TEXCOORD_0")) {
            has_texcoord = true;
            auto& pos_acc = accessors[vtx["TEXCOORD_0"].i()];
            int b = 0;
            sh.texture.resize(pos_acc.count);
            for (auto& v : sh.texture) {
              v = pos_acc.VEC2(b++);
            }
          }
          bool has_color = false;
          if (vtx.has("COLOR_0")) {
            has_color = true;
            auto pos_acc = accessors[vtx["COLOR_0"].i()];
            int b = 0;
            sh.color.resize(pos_acc.count);
            for (auto& v : sh.color) {
              v = pos_acc.VEC4(b++);
            }
          }
          bool has_joint = false;
          if (vtx.has("JOINTS_0") && vtx.has("WEIGHTS_0")) {
            has_joint = true;
            if (is_weight_loaded.end() == is_weight_loaded.find(vtx["WEIGHTS_0"].i())) {
              auto& joint_acc = accessors[vtx["JOINTS_0"].i()];
              auto& weight_acc = accessors[vtx["WEIGHTS_0"].i()];
              for (int b = 0; b < joint_acc.count; b++) {
                uint16_t j[4];
                float w[4];
                joint_acc.fetch(b, j);
                weight_acc.fetch(b, w);
                float f = w[0] + w[1];
                for (int l = 0; l < 2; l++) {
                  if (w[l] > EPSILON)
                  {
                    sh.weight[j[l]].position.push_back({ (IndexType)b,w[l] / f });
                    sh.weight[j[l]].normal.push_back({ (IndexType)b,w[l] / f });
                  }
                }
              }
              is_weight_loaded[vtx["WEIGHTS_0"].i()] = true;
            }
            //sh.weight.resize(joint_acc.count);
            //for (auto& v : sh.weight) {
            //  v.position = joint_acc.VEC4(b);
            //}
          }

          int indices = J(p)["indices"].i();
          auto& idx_acc = accessors[indices];
          subset.indices.resize(idx_acc.count);
          subset.material.texture = materials[J(p)["material"].i()].decal;
          subset.material.diffuse = materials[J(p)["material"].i()].color;
          int b = 0;
          int offsets[] = { 2,-1,2 };
          int ofn = 0;
          for (auto& v : subset.indices) {
            //index が属性で一緒
            auto i = idx_acc.SCALAR(b);
            v.position = i;
            v.normal = i;
            v.texture = i;
            v.color = i;
            b += offsets[ofn % 3];
            ofn++;
          }
        }
        mesh_num++;
      }
    }
    if (top.end() != top.find("extensions")) {
      for (auto j : top.find("extensions")->second.object_items()) {
        if (j.first == "VRM") {
          for (auto& k : j.second.object_items()) {
            if (k.first == "humanoid") {
              for (auto& l : k.second.object_items()) {
                if (l.first == "humanBones") {
                  for (auto& h : l.second.array_items()) {
                    string name = J(h)["bone"].s();
                    int node = J(h)["node"].i();
                    Bone* bn = root.GetChildFromName(nodes_name_index[node]);
                    bn->humanoid_name = name;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  /*
  bool ParsePMX(const char* buf, int len, Bone& root)
  {
    root.absolute.Identity();
    root.transform.Identity();
    root.name = "root";

    BinaryReader reader = { buf, buf + len };

    pmx::Format fmt;

    if (!Convert(reader, fmt, ""))
      return false;

    root.name = fmt.info.name.string;
    root.transform.Identity();
    root.ik.head.clear();
    Shape& sh = _.push(root.shape);
    int vertices = fmt.vertex.size();

    // 頂点は非共有
    sh.position.resize(vertices);
    sh.normal.resize(vertices);
    sh.texture.resize(vertices);

    // 重みをボーンに設定 
    sh.weight.resize(fmt.bone.size());

    // 頂点組み立て
    for (int i = 0; i < vertices; i++)
    {
      const auto& v = fmt.vertex[i];
      sh.position[i] = v.position;
      sh.normal[i] = v.normal;
      sh.texture[i] = v.uv;

      if (0 == v.weight_type) {
        WeightIndex& p = _.push(sh.weight[v.weight_index[0].v].position);
        p.weight = 1.0f;
        p.index = i;
        WeightIndex& n = _.push(sh.weight[v.weight_index[0].v].normal);
        n.weight = 1.0f;
        n.index = i;
      }
      else if (1 == v.weight_type || 3 == v.weight_type) {
        WeightIndex& p0 = _.push(sh.weight[v.weight_index[0].v].position);
        p0.weight = v.weight[0];
        p0.index = i;
        WeightIndex& n0 = _.push(sh.weight[v.weight_index[0].v].normal);
        n0.weight = v.weight[0];
        n0.index = i;
        WeightIndex& p1 = _.push(sh.weight[v.weight_index[1].v].position);
        p1.weight = 1 - v.weight[0];
        p1.index = i;
        WeightIndex& n1 = _.push(sh.weight[v.weight_index[1].v].normal);
        n1.weight = 1 - v.weight[0];
        n1.index = i;
      }
      else if (2 == v.weight_type) {
        for (int j = 0; j < 4; j++) {
          WeightIndex& p = _.push(sh.weight[v.weight_index[j].v].position);
          p.weight = v.weight[j];
          p.index = i;
          WeightIndex& n = _.push(sh.weight[v.weight_index[j].v].normal);
          n.weight = v.weight[j];
          n.index = i;
        }
      }
    }

    // 位置モーフィング組み立て

    std::set<int> base;
    for (auto& m : fmt.morph) {
      if (1 == m.offset_kind) {
        for (auto& j : m.offset) {
          base.insert(j.vertex.index.v);
        }
      }
    }
    {
      Morphing br;
      br.name = "base";
      br.target.resize(base.size());
      int n = 0;
      for (auto& i : base) {
        br.target[n].index = i;
        br.target[n].position = sh.position[i];
        n++;
      }
      sh.morphing.push_back(br);
    }
    for (int i = 0; i < fmt.morph.size(); i++) {
      const auto& m = fmt.morph[i];
      if (1 == m.offset_kind) {
        Morphing r;
        r.name = m.name.string;
        r.target.resize(m.offset.size());
        for (int j = 0; j < m.offset.size(); j++) {
          r.target[j].index = m.offset[j].vertex.index.v;
          r.target[j].position = m.offset[j].vertex.position;
        }
        sh.morphing.push_back(r);

      }
    }

    // 面リスト組み立て

    int subsets = fmt.subset.size();
    sh.subset.resize(subsets);
    int faceoffset = 0;
    for (int i = 0; i < subsets; i++)
    {
      int faces = fmt.subset[i].face;
      sh.subset[i].indices.resize(faces);
      IndexVertex* iv = sh.subset[i].indices.data();
      for (int j = 0; j < faces / 3; j++)
      {
        IndexType idx;
        idx = fmt.face[faceoffset + j * 3 + 0].v;
        iv->position = idx;
        iv->normal = idx;
        iv->texture = idx;
        iv->color = 0;
        iv++;
        idx = fmt.face[faceoffset + j * 3 + 1].v;
        iv->position = idx;
        iv->normal = idx;
        iv->texture = idx;
        iv->color = 0;
        iv++;
        idx = fmt.face[faceoffset + j * 3 + 2].v;
        iv->position = idx;
        iv->normal = idx;
        iv->texture = idx;
        iv->color = 0;
        iv++;
      }
      faceoffset += faces;

      //fmt.subset[i].specular.a = 1.0f;
      //fmt.subset[i].ambient.a = 0.0f;

      Material& mtr = sh.subset[i].material;
      if (-1 != fmt.subset[i].decal.v) {
        mtr.texture = fmt.texture[fmt.subset[i].decal.v].string;
        if (mtr.texture.find(".bmp") == mtr.texture.size() - 4)
        {
          mtr.texture.replace(mtr.texture.size() - 3, 3, "png");
        }
        if (std::string::npos != mtr.texture.find('*')) {
          mtr.texture.erase(mtr.texture.begin() + mtr.texture.find('*'), mtr.texture.end());
        }
      }
      mtr.ambient.fromRGBf(fmt.subset[i].ambient);
      mtr.diffuse.fromRGBf(fmt.subset[i].diffuse);
      mtr.specular.fromRGBf(fmt.subset[i].specular);
      mtr.power = fmt.subset[i].power;
    }

    // ボーン位置設定

    vector<string> list;
    for (auto& b : fmt.bone)
    {
      list.push_back(b.name.string);
    }

    for (uint16_t i = 0; i < fmt.bone.size(); i++)
    {
      auto& b = fmt.bone[i];

      Bone* bone, * parent = 0;
      if (-1 == b.parent.v)
      {
        bone = &_.push(root.child);
      }
      else
      {
        parent = root.GetChildFromName(list[b.parent.v]);
        bone = &_.push(parent->child);
      }
      bone->name = b.name.string;
      bone->absolute.Identity();
      bone->absolute.Translate(b.base);// 絶対座標 
    }

    // アニメーション時のオフセット設定 
    for (int i = 0; i < sh.weight.size(); i++)
    {
      Bone* bone = root.GetChildFromName(list[i]);
      sh.weight[i].offset = bone->absolute;
      sh.weight[i].offset.Inverse();
      sh.weight[i].bonename = bone->name;
    }

    // ボーン位置を相対座標に変換 
    //for (int i = fmt.bone.size() - 1; i >= 0; i--)
    for (int i = 0; i < fmt.bone.size(); i++)
    {
      const auto& src = fmt.bone[i];

      Bone* bone = root.GetChildFromName(list[i]), * parent = 0;
      bone->ik.head.clear();
      if (-1 != src.parent.v)
      {
        parent = root.GetChildFromName(list[src.parent.v]);
      }
      else
      {
        parent = &root;
      }
      Matrix mat;
      mat = parent->absolute;
      mat.Inverse();
      bone->inverse = mat;
      bone->transform = mat * bone->absolute;

      if (0x0020 & src.flag) {
        auto& ik = src.ik;
        //Bone *bone = root.GetChildFromName(list[ik.bone]);
        bone->ik.head = root.GetChildFromName(list[ik.target.v])->name;
        for (int j = 0; j < ik.links.size(); j++)
        {
          bone->ik.effects.push_back(root.GetChildFromName(list[ik.links[j].link.v])->name);
        }

        bone->ik.iteration = ik.iteration;
        bone->ik.limit_angle = ik.limit_angle;
      }
    }

    for (int i = 0; i < fmt.rigid.size(); i++) {
      const auto& r = fmt.rigid[i];
      PMDRigid d;
      if (-1 == r.bone_index.v)continue;
      auto bone = root.GetChildFromName(list[r.bone_index.v]);
      d.name = r.name.string;
      d.bone_name = -1 == r.bone_index.v ? "" : list[r.bone_index.v];
      d.collision_group = 1 << r.group_index;
      d.collision_mask = r.group_target;
      d.shape_type = r.shape_type;
      d.shape_width = r.shape_width;
      d.shape_height = r.shape_height;
      d.shape_depth = r.shape_depth;
      d.pos = bone ? r.pos - bone->absolute.W : r.pos;
      d.rot = r.rot;
      d.weight = r.weight;
      d.pos_dim = r.pos_dim;
      d.rot_dim = r.rot_dim;
      d.friction = r.friction;
      d.type = r.type;
      //if (0xffff != r.bone_index) {
      //  Bone *bone = root.GetChildFromName(fmt.bone[r.bone_index].name);
      //  if (bone) {
      //    bone->rigid.push_back(d);
      //  }
      //}
      //else 
      {
        root.rigid.push_back(d);
      }
    }

    for (int i = 0; i < fmt.joint.size(); i++) {
      const auto& j = fmt.joint[i];
      PMDJoint d;
      d.name = j.name.string;
      d.parent = fmt.rigid[j.rigidbody_a.v].name.string;
      d.current = fmt.rigid[j.rigidbody_b.v].name.string;
      d.pos = j.pos;
      d.rot = j.rot;
      d.pos_min = j.constrain_pos_1;
      d.pos_max = j.constrain_pos_2;
      d.rot_min = j.constrain_rot_1;
      d.rot_max = j.constrain_rot_2;
      d.spring_pos = j.spring_pos;
      d.spring_rot = j.spring_rot;
      root.joint.push_back(d);
    }

    return true;
  }
  bool ParsePMD(const char* buf, int len, Bone& root)
  {
    root.absolute.Identity();
    root.transform.Identity();
    root.name = "root";

    BinaryReader reader = { buf, buf + len };

    pmd::Format fmt;

    if (!Convert(reader, fmt, ""))
      return false;

    root.name = fmt.header.name;
    root.transform.Identity();
    root.ik.head.clear();
    Shape& sh = _.push(root.shape);
    int vertices = fmt.vertex.size();

    // 頂点は非共有
    sh.position.resize(vertices);
    sh.normal.resize(vertices);
    sh.texture.resize(vertices);

    // 重みをボーンに設定 
    sh.weight.resize(fmt.bone.size());

    // 頂点組み立て
    for (int i = 0; i < vertices; i++)
    {
      sh.position[i] = fmt.vertex[i].position;
      sh.normal[i] = fmt.vertex[i].normal;
      sh.texture[i] = fmt.vertex[i].uv;

      {
        WeightIndex& w = _.push(sh.weight[fmt.vertex[i].bone_index[0]].position);
        w.weight = fmt.vertex[i].bone_weight / 100.0f;
        w.index = i;
      }
      {
        WeightIndex& w = _.push(sh.weight[fmt.vertex[i].bone_index[0]].normal);
        w.weight = fmt.vertex[i].bone_weight / 100.0f;
        w.index = i;
      }
      {
        WeightIndex& w = _.push(sh.weight[fmt.vertex[i].bone_index[1]].position);
        w.weight = 1.0f - (fmt.vertex[i].bone_weight / 100.0f);
        w.index = i;
      }
      {
        WeightIndex& w = _.push(sh.weight[fmt.vertex[i].bone_index[1]].normal);
        w.weight = 1.0f - (fmt.vertex[i].bone_weight / 100.0f);
        w.index = i;
      }
    }

    // 位置モーフィング組み立て
    const pmd::Morph* base;
    for (int i = 0; i < fmt.morph.size(); i++) {
      const auto& m = fmt.morph[i];
      if (0 == m.type) {
        base = &m;
        Morphing r;
        r.name = m.name;
        r.target.resize(m.vertex.size());
        for (int j = 0; j < m.vertex.size(); j++) {
          r.target[j].index = m.vertex[j].index;
          r.target[j].position = m.vertex[j].position;
        }
        sh.morphing.push_back(r);
      }
      else {
        Morphing r;
        r.name = m.name;
        r.target.resize(m.vertex.size());
        for (int j = 0; j < m.vertex.size(); j++) {
          r.target[j].index = base->vertex[m.vertex[j].index].index;
          r.target[j].position = m.vertex[j].position;
        }
        sh.morphing.push_back(r);
      }
    }

    // 面リスト組み立て

    int subsets = fmt.subset.size();
    sh.subset.resize(subsets);
    int faceoffset = 0;
    for (int i = 0; i < subsets; i++)
    {
      int faces = fmt.subset[i].face;
      sh.subset[i].indices.resize(faces);
      IndexVertex* iv = sh.subset[i].indices.data();
      for (int j = 0; j < faces / 3; j++)
      {
        IndexType idx;
        idx = fmt.face[faceoffset + j * 3 + 0];
        iv->position = idx;
        iv->normal = idx;
        iv->texture = idx;
        iv->color = 0;
        iv++;
        idx = fmt.face[faceoffset + j * 3 + 1];
        iv->position = idx;
        iv->normal = idx;
        iv->texture = idx;
        iv->color = 0;
        iv++;
        idx = fmt.face[faceoffset + j * 3 + 2];
        iv->position = idx;
        iv->normal = idx;
        iv->texture = idx;
        iv->color = 0;
        iv++;
      }
      faceoffset += faces;

      fmt.subset[i].specular.a = 1.0f;
      fmt.subset[i].ambient.a = 0.0f;

      Material& mtr = sh.subset[i].material;
      mtr.texture = fmt.subset[i].texture;
      if (mtr.texture.find(".bmp") == mtr.texture.size() - 4)
      {
        mtr.texture.replace(mtr.texture.size() - 3, 3, "png");
      }
      if (std::string::npos != mtr.texture.find('*')) {
        mtr.texture.erase(mtr.texture.begin() + mtr.texture.find('*'), mtr.texture.end());
      }
      mtr.ambient = fmt.subset[i].ambient;
      mtr.diffuse = fmt.subset[i].diffuse;
      mtr.specular = fmt.subset[i].specular;
      mtr.power = fmt.subset[i].power;
    }

    // ボーン位置設定

    vector<string> list;
    for (uint16_t i = 0; i < fmt.bone.size(); i++)
    {
      pmd::Bone& src = fmt.bone[i];

      Bone* bone, * parent = 0;
      if (0xffff == src.parent)
      {
        bone = &_.push(root.child);
      }
      else
      {
        parent = root.GetChildFromName(list[src.parent]);
        bone = &_.push(parent->child);
      }
      bone->name = src.name;
      bone->absolute.Identity();
      bone->absolute.Translate(src.base);// 絶対座標 
      list.push_back(bone->name);
    }

    // アニメーション時のオフセット設定 
    for (int i = 0; i < sh.weight.size(); i++)
    {
      Bone* bone = root.GetChildFromName(list[i]);
      sh.weight[i].offset = bone->absolute;
      sh.weight[i].offset.Inverse();
      sh.weight[i].bonename = bone->name;
    }

    // ボーン位置を相対座標に変換 
    //for (int i = fmt.bone.size() - 1; i >= 0; i--)
    for (int i = 0; i < fmt.bone.size(); i++)
    {
      pmd::Bone& src = fmt.bone[i];

      Bone* bone = root.GetChildFromName(list[i]), * parent = 0;
      bone->ik.head.clear();
      if (0xffff != src.parent)
      {
        parent = root.GetChildFromName(list[src.parent]);
      }
      else
      {
        parent = &root;
      }
      Matrix mat;
      mat = parent->absolute;
      mat.Inverse();
      bone->inverse = mat;
      bone->transform = mat * bone->absolute;
    }

    for (int i = 0; i < fmt.ik.size(); i++)
    {
      pmd::Ik& ik = fmt.ik[i];
      Bone* bone = root.GetChildFromName(list[ik.bone]);
      bone->ik.head = root.GetChildFromName(list[ik.target])->name;
      for (int j = 0; j < ik.child.size(); j++)
      {
        bone->ik.effects.push_back(root.GetChildFromName(list[ik.child[j]])->name);
      }

      bone->ik.iteration = ik.iteration;
      bone->ik.limit_angle = ik.limit_angle;
    }

    for (int i = 0; i < fmt.rigid.size(); i++) {
      const auto& r = fmt.rigid[i];
      PMDRigid d;
      d.name = r.name;
      d.bone_name = 0xffff == r.bone_index ? "" : list[r.bone_index];
      d.collision_group = 1 << r.group_index;
      d.collision_mask = r.group_target;
      d.shape_type = r.shape_type;
      d.shape_width = r.shape_width;
      d.shape_height = r.shape_height;
      d.shape_depth = r.shape_depth;
      d.pos = r.pos;
      d.rot = r.rot;
      d.weight = r.weight;
      d.pos_dim = r.pos_dim;
      d.rot_dim = r.rot_dim;
      d.friction = r.friction;
      d.type = r.type;
      //if (0xffff != r.bone_index) {
      //  Bone *bone = root.GetChildFromName(fmt.bone[r.bone_index].name);
      //  if (bone) {
      //    bone->rigid.push_back(d);
      //  }
      //}
      //else 
      {
        root.rigid.push_back(d);
      }
    }

    for (int i = 0; i < fmt.joint.size(); i++) {
      const auto& j = fmt.joint[i];
      PMDJoint d;
      d.name = j.name;
      d.parent = fmt.rigid[j.rigidbody_a].name;
      d.current = fmt.rigid[j.rigidbody_b].name;
      d.pos = j.pos;
      d.rot = j.rot;
      d.pos_min = j.constrain_pos_1;
      d.pos_max = j.constrain_pos_2;
      d.rot_min = j.constrain_rot_1;
      d.rot_max = j.constrain_rot_2;
      d.spring_pos = j.spring_pos;
      d.spring_rot = j.spring_rot;
      root.joint.push_back(d);
    }

    return true;
  }
  */

  bool ParseVMD(const char* buf, int len, Animation& anim)
  {
    BinaryReader reader = { buf, buf + len };

    vmd::Format fmt;

    // モーション読み込み 
    if (!Convert(reader, fmt, ""))
      return false;

    // キーフレーム順に整列 
    qsort(fmt.key.data(), fmt.key.size(), sizeof(vmd::Key), vmd::KeyCmp);

    // ボーンに適応 
    for (int i = 0; i < fmt.key.size(); i++)
    {
      vmd::Key& src = fmt.key[i];
      string target = src.target_bone;
      AnimationBone* bone = 0;
      for (int j = 0; j < anim.bone.size(); j++)
      {
        if (anim.bone[j].targetbone == target)
        {
          bone = &anim.bone[j];
          break;
        }
      }
      if (!bone)
      {
        bone = &_.push(anim.bone);
        bone->targetbone = target;
      }
      if (bone->length < (int)(src.elapsed * 33.33f))
      {
        bone->length = (int)(src.elapsed * 33.33f);
      }
      AnimationKey& key = _.push(bone->key);
      key.interval = (int)(src.elapsed * 33.33f);
      key.rotation = src.rotation;
      key.scale = Vec3(1.0f, 1.0f, 1.0f);
      key.translation = src.translation;
    }

    // 絶対時間を相対時間に変更 
    for (int j = 0; j < anim.bone.size(); j++)
    {
      for (int i = anim.bone[j].key.size() - 1; i > 0; i--)
      {
        int delta = anim.bone[j].key[i].interval - anim.bone[j].key[i - 1].interval;
        anim.bone[j].key[i].interval = delta;
      }
    }

    qsort(fmt.morph.data(), fmt.morph.size(), sizeof(vmd::Morph), [](const void* a, const void* b)->int {
      return reinterpret_cast<const vmd::Morph*>(a)->frame - reinterpret_cast<const vmd::Morph*>(b)->frame;
      });

    for (const auto& f : fmt.morph) {
      Morph* m = nullptr;
      for (auto& a : anim.morph) {
        if (a.name == f.name) {
          m = &a;
          break;
        }
      }
      if (!m) {
        m = &_.push(anim.morph);
        m->name = f.name;
      }
      if (m->length < (int)(f.frame * 33.33f))
      {
        m->length = (int)(f.frame * 33.33f);
      }
      MorphKey k;
      k.interval = (int)(f.frame * 33.33f);
      k.weight = f.weight;
      m->key.push_back(k);
    }
    // 絶対時間を相対時間に変更 
    for (int j = 0; j < anim.morph.size(); j++)
    {
      for (int i = anim.morph[j].key.size() - 1; i > 0; i--)
      {
        int delta = anim.morph[j].key[i].interval - anim.morph[j].key[i - 1].interval;
        anim.morph[j].key[i].interval = delta;
      }
    }

    return true;
  }

  bool ParseX(const char* buf, int len, Bone& root, AnimationSet& animation)
  {
    std::vector<char> data;
    _.push(data, buf, len);

    data.push_back(0);

    xfile::tFormat format;
    const char* text = data.data();
    format.Parse(text);
    if (text < (&data.back() - 1))
    {
      ;
    }

    root.transform.Identity();
    root.name = "root";
    for (int i = 0; i < format.elements.size(); i++)
    {
      if (format.elements[i].type == xfile::ET_Block)
      {
        xfile::ReadBlock(*(xfile::tBlock*)(format.elements[i].p), root, animation);
      }
    }
    return true;
  }


  AnimationBone::AnimationBone()
  {
    length = 0;
  }

  AnimationKey& AnimationBone::GetKeyFromCount(int count)
  {
    int total = 0;
    for (int i = 0; i < key.size(); i++)
    {
      total += key[i].interval;
      if (total == count)
        return key[i];
      else if (total > count)
      {
        AnimationKey ak;
        ak.interval = total - count;
        key[i].interval -= ak.interval;
        ak.rotation = key[i - 1].rotation;
        ak.scale = key[i - 1].scale;
        ak.translation = key[i - 1].translation;
        key.insert(key.begin() + i, ak);
        return key[i];
      }
    }
    if (total - count || key.size() == 0)
    {
      AnimationKey ak;
      ak.interval = count - total;
      length += ak.interval;
      ak.rotation.x = 1;
      ak.rotation.y = 0;
      ak.rotation.z = 0;
      ak.rotation.w = 0;
      ak.scale = Vec3(1, 1, 1);
      ak.translation = Vec3(0, 0, 0);
      key.push_back(ak);
    }
    return key.back();
  }

  static void InitializeAnimatedPose(AnimatedPose& ap, const Bone& bone, int parent)
  {
    int tp = (int)ap.pose.size();
    Pose& pose = _.push(ap.pose);
    pose.bonename = bone.name;
    pose.parent = parent;
    pose.base = bone.transform;
    pose.ik = bone.ik;


    for (int i = 0; i < bone.child.size(); i++)
    {
      InitializeAnimatedPose(ap, bone.child[i], tp);
    }
  }

  void CreateAnimatedPose(AnimatedPose& pose, const Bone& bone, const Animation& animation)
  {
    pose.pose.clear();
    pose.anim = &animation;
    InitializeAnimatedPose(pose, bone, -1);
    for (int i = 0; i < pose.pose.size(); i++) {
      if (-1 != pose.pose[i].parent) {
        pose.pose[pose.pose[i].parent].child.push_back(i);
      }
    }
    for (const auto& m : animation.morph) {
      pose.morph_weight[m.name] = 0;
    }
  }

  Pose* AnimatedPose::GetBoneNc(const string& bonename)
  {
    for (auto& p : pose) {
      if (p.bonename == bonename) {
        return &p;
      }
    }
    return 0;
  }

  const Pose* AnimatedPose::GetBone(const string& bonename)const
  {
    for (const auto& p : pose) {
      if (p.bonename == bonename) {
        return &p;
      }
    }
    return 0;
  }

  void gh::AnimatedPose::CalcPoseFromAnim(float count)
  {
    //update local
#define USE_BONENAME_MAP
#ifdef USE_BONENAME_MAP
    map<string, const AnimationBone*> bonename_map;
    for (int j = 0; j < anim->bone.size(); j++) {
      bonename_map[anim->bone[j].targetbone] = &(anim->bone[j]);
    }
#endif
    for (int i = 0; i < pose.size(); i++)
    {
      Pose* tp = &pose[i];
      tp->relative.Identity();
      const AnimationBone* bone = 0;
#ifdef USE_BONENAME_MAP
      auto hit = bonename_map.find(tp->bonename);
      if (hit != bonename_map.end())
        bone = hit->second;
#else
      for (int j = 0; j < anim->size(); j++)
      {
        if ((*anim)[j].targetbone == tp->bonename)
        {
          bone = &((*anim)[j]);
          break;
        }
      }
#endif
      if (bone && 0 != bone->length)
      {
        const AnimationKey* last = bone->key.data();
        const auto pkey = last;
        int key_len = bone->key.size();
        int n = (int)count / bone->length;
        float cc = count - bone->length * n;
        if (cc < 0)
          cc = 0;
        for (int t = 0;; t++)
        {
          if (t >= key_len)
            t = 0;
          if (cc == pkey[t].interval)
          {
            const AnimationKey& key = pkey[t];
            tp->relative.Scale(key.scale.x, key.scale.y, key.scale.z);
            tp->relative.Rotate(key.rotation);
            tp->local_rotation = key.rotation;
            tp->relative.Translate(key.translation.x, key.translation.y, key.translation.z);
            break;
          }
          else if (cc < pkey[t].interval)
          {
            const AnimationKey& key = pkey[t];
            float f = 1.0f / key.interval * cc;
            Vector3 t, s;
            Quaternion r;
            r = Slerp(last->rotation, key.rotation, f);
            t = Lerp(last->translation, key.translation, f);
            s = Lerp(last->scale, key.scale, f);
            tp->relative.Scale(s.x, s.y, s.z);
            tp->relative.Rotate(r);
            tp->local_rotation = r;
            tp->relative.Translate(t.x, t.y, t.z);
            break;
          }
          cc -= pkey[t].interval;
          last = pkey + t;
        }
      }
    }
  }

  void AnimatedPose::SetTime(float count)
  {
    CalcPoseFromAnim(count);
    //update world
    for (int i = 0; i < pose.size(); i++)
    {
      Pose* tp = &pose[i];
      if (tp->parent != -1)
        tp->absolute = tp->relative * tp->base * pose[tp->parent].absolute;
      else
        tp->absolute = tp->relative * tp->base;
    }
    //update ik
    for (int i = 0; i < pose.size(); i++)
    {
      Pose* tp = &pose[i];
      if (tp->ik.head.size())
      {
        VMDIK& ik = tp->ik;
        const Vector3 target = tp->absolute.W;
        Pose* head = const_cast<Pose*>(GetBone(ik.head));
        head->relative.Identity();
#define CACHE_IK_EFFECTS
#ifdef CACHE_IK_EFFECTS
        vector<Pose*> cache_ik_effects;
        cache_ik_effects.resize(ik.effects.size());
        for (int j = 0; j < ik.effects.size(); j++) {
          cache_ik_effects[j] = const_cast<Pose*>(GetBone(ik.effects[j]));
          cache_ik_effects[j]->relative.Identity();
          //cache_ik_effects[j]->local_rotation.Identity();
          //if (cache_ik_effects[j]->bonename.find("ひざ") != -1) {
          //  cache_ik_effects[j]->relative.RotateX(-PI / 2);
          //}
        }
#endif
        bool rotated = true;
        for (int k = 0; k < ik.iteration && rotated; k++)
        {
          rotated = false;
          for (int j = 0; j < ik.effects.size(); j++)
          {
#ifdef CACHE_IK_EFFECTS
            Pose* eff = cache_ik_effects[j];
#else
            Pose* eff = const_cast<Pose*>(GetBone(ik.effects[j]));
#endif
            Vector3 vt, vh;

            vt = target - eff->absolute.W;
            vh = (Vector3)head->absolute.W - eff->absolute.W;
            Quaternion inv;
            inv.FromMatrix(eff->absolute);
            inv.Inverse();

            inv.Transform(vt);
            inv.Transform(vh);

            //float distance = (target - head->absolute.W).Sqlen();
            //if (distance < 0.001f) {
            //  k = ik.iteration;
            //  break;
            //}
            //bool is_inner = vt.Sqlen() < vh.Sqlen();
            vt.Normalize();
            vh.Normalize();

            //if (eff->bonename.find("ひざ") != -1)
            //{
            //  vt.x = 0.0f;
            //  vh.x = 0.0f;
            //}

            float dot = Dot(vt, vh);
            if (dot >= 0.99999f)
              continue;
            float angle = acosf(dot);

            if (angle > ik.limit_angle) {
              angle = ik.limit_angle;
            }
            if (angle < 0.0001f)
              continue;

            Vector3 axis;
            axis = Cross(vh, vt);
            axis.Normalize();
            Quaternion q;
            q.FromAxisAngle(axis, angle);
            eff->local_rotation *= q;
            if (eff->bonename.find("ひざ") != -1)
            {
              float c = eff->local_rotation.w;
              if (c > 1)c = 1;
              float c2 = -sqrtf(1 - c * c);
              eff->local_rotation.x = 1.0f * c2;
              eff->local_rotation.y = 0.0f * c2;
              eff->local_rotation.z = 0.0f * c2;
              eff->local_rotation.w = c;
            }
            eff->relative.Identity();
            eff->relative.Rotate(eff->local_rotation);
            rotated = true;
            this->Update(eff);
          }
        }
      }
    }
    //update morph
    for (const auto& m : anim->morph) {
      if (0 == m.length) {
        continue;
      }
      const auto* last = m.key.data();
      auto pkey = last;
      auto key_len = m.key.size();
      int n = (int)count / m.length;
      float cc = count - m.length * n;
      if (cc < 0)
        cc = 0;
      for (int t = 0;; t++)
      {
        if (t >= key_len)
          t = 0;
        if (cc == pkey[t].interval)
        {
          auto& key = pkey[t];
          morph_weight[m.name] = key.weight;
          break;
        }
        else if (cc < pkey[t].interval)
        {
          auto& key = pkey[t];
          float f = 1.0f / key.interval * cc;
          morph_weight[m.name] = last->weight * (1 - f) + key.weight * f;
          break;
        }
        cc -= pkey[t].interval;
        last = pkey + t;
      }
    }
  }
  void gh::AnimatedPose::Update(Pose* root)
  {
    if (-1 != root->parent) {
      root->absolute = root->relative * root->base * pose[root->parent].absolute;
    }
    else {
      root->absolute = root->relative * root->base;
    }
    for (auto i : root->child)
    {
      Update(&pose[i]);
    }
  }

  void Shape::Normalize(float scale)
  {
    Vector3 vv = Vec3(0, 0, 0);
    for (size_t i = 0; i < position.size(); i++)
      vv += position[i];
    vv /= (float)position.size();
    float ave = 0;
    for (size_t i = 0; i < position.size(); i++)
    {
      position[i] -= vv;
      ave += position[i].Magnitude();
    }
    ave /= position.size();
    ave /= scale;
    for (size_t i = 0; i < position.size(); i++)
      position[i] /= ave;
  }

  void Shape::Centering()
  {
    Vector3 vv = Vec3(0, 0, 0);
    for (const auto p : position)
      vv += p;
    vv /= (float)position.size();
    for (auto& p : position)
      p -= vv;
  }

  float Shape::Magnitude()const
  {
    float ave = 0;
    for (size_t i = 0; i < position.size(); i++)
    {
      ave += position[i].Magnitude();
    }
    ave /= position.size();
    return ave;
  }

  void Shape::SetNormal()
  {
    normal.resize(position.size());
    for (size_t i = 0; i < normal.size(); i++)
      normal[i] = Vec3(0, 0, 0);
    Vector3 v1, v2, vn;
    for (size_t i = 0; i < subset.size(); i++)
    {
      std::vector<IndexVertex>& idx = subset[i].indices;
      for (size_t j = 0; j < idx.size(); j++)
        idx[j].normal = idx[j].position;
      for (size_t j = 0; j < idx.size(); j += 3)
      {
        v1 = position[idx[j + 1].position] - position[idx[j].position];
        v2 = position[idx[j + 2].position] - position[idx[j].position];
        vn = Cross(v1, v2);
        normal[idx[j].normal] += vn;
        normal[idx[j + 1].normal] += vn;
        normal[idx[j + 2].normal] += vn;
      }
    }
    for (size_t i = 0; i < normal.size(); i++)
      normal[i].Normalize();
  }

  void Shape::ReducePosition(float threshold)
  {
    //search
    float t = threshold * threshold;
    for (int i = 0; i < position.size(); i++)
    {
      for (int j = i + 1; j < position.size(); j++)
      {
        if ((position[i] - position[j]).Sqlen() < t)
        {
          //replace
          for (int k = 0; k < subset.size(); k++)
          {
            std::vector<IndexVertex>& idx = subset[k].indices;
            for (size_t l = 0; l < idx.size(); l++)
            {
              if (j == idx[l].position)
                idx[l].position = i;
              else if (j < idx[l].position)
                idx[l].position--;
            }
          }
          //reduce
          position.erase(position.begin() + j);
          --j;
        }
      }
    }
    //replace
  }

  void Shape::ReverseCulling()
  {
    for (int i = 0; i < subset.size(); i++)
    {
      for (int j = 0; j < subset[i].indices.size(); j += 3)
      {
        IndexVertex temp = subset[i].indices[j + 1];
        subset[i].indices[j + 1] = subset[i].indices[j + 2];
        subset[i].indices[j + 2] = temp;
      }
    }
  }

  void Shape::UVScale(float scale)
  {
    for (size_t i = 0; i < texture.size(); i++)
    {
      texture[i].x *= scale;
      texture[i].y *= scale;
    }
  }
  void Shape::CenteringX()
  {
    float min = this->position[0].x, max = this->position[0].x;
    for (auto p : this->position)
    {
      min = min < p.x ? min : p.x;
      max = max < p.x ? p.x : max;
    }
    float c = min + max / 2;
    for (auto& p : this->position)
      p.x -= c;
  }
  void Shape::Scale(float x)
  {
    for (auto& p : this->position)
      p *= x;
  }

  void Shape::MakeTangent()
  {
    tangent.resize(normal.size());
    for (size_t i = 0; i < tangent.size(); i++)
      tangent[i] = Vec3(0, 0, 0);
    for (size_t i = 0; i < subset.size(); i++)
    {
      std::vector<IndexVertex>& idx = subset[i].indices;
      for (size_t j = 0; j < idx.size(); j += 3)
      {
        Vector3 puv[3][3] =
        {
          {
            Vec3(position[idx[j].position].x, texture[idx[j].texture].x, texture[idx[j].texture].y),
            Vec3(position[idx[j].position].y, texture[idx[j].texture].x, texture[idx[j].texture].y),
            Vec3(position[idx[j].position].z, texture[idx[j].texture].x, texture[idx[j].texture].y)
          },
          {
            Vec3(position[idx[j + 1].position].x, texture[idx[j + 1].texture].x, texture[idx[j + 1].texture].y),
            Vec3(position[idx[j + 1].position].y, texture[idx[j + 1].texture].x, texture[idx[j + 1].texture].y),
            Vec3(position[idx[j + 1].position].z, texture[idx[j + 1].texture].x, texture[idx[j + 1].texture].y)
          },
          {
            Vec3(position[idx[j + 2].position].x, texture[idx[j + 2].texture].x, texture[idx[j + 2].texture].y),
            Vec3(position[idx[j + 2].position].y, texture[idx[j + 2].texture].x, texture[idx[j + 2].texture].y),
            Vec3(position[idx[j + 2].position].z, texture[idx[j + 2].texture].x, texture[idx[j + 2].texture].y)
          }
        };
        Vector3 vn;
        for (int k = 0; k < 3; k++)
        {
          vn = Cross(puv[1][k] - puv[0][k], puv[2][k] - puv[1][k]);
          if (vn.Magnitude() <= EPSILON)
          {
            tangent[idx[j + k].normal].v[k] += 0.0f;
          }
          else
          {
            vn.Normalize();
            tangent[idx[j + k].normal].v[k] += -vn.y / vn.x;
          }
        }
      }
    }
    for (size_t i = 0; i < tangent.size(); i++)
    {
      tangent[i].Normalize();
      float dot = Dot(normal[i], tangent[i]);
      tangent[i] -= normal[i] * dot;
      tangent[i].Normalize();
    }
  }

  void Bone::ReverseCulling()
  {
    for (int i = 0; i < shape.size(); i++)
    {
      shape[i].ReverseCulling();
    }
    for (int i = 0; i < child.size(); i++)
    {
      child[i].ReverseCulling();
    }
  }
}

