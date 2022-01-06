
#ifndef GH_POLYGON_H_
#define GH_POLYGON_H_

#include "geometory.h"
#include "../main/converter.h"

#include "graphics.h"
#include "shape.h"

namespace gh {


  struct VMDIK
  {
    std::string head;
    std::vector<std::string> effects;
    int iteration;
    float limit_angle;
  };

  struct PMDRigid {
    std::string name;
    std::string bone_name;
    int collision_group;
    int collision_mask;
    int shape_type;
    float shape_width;
    float shape_height;
    float shape_depth;
    Vector3 pos;
    Vector3 rot;
    float weight;
    float pos_dim;
    float rot_dim;
    float friction;
    int type;
  };

  struct PMDJoint {
    std::string name;
    std::string parent;
    std::string current;
    Vector3 pos; // 諸データ：位置(x, y, z) // 諸データ：位置合せでも設定可
    Vector3 rot; // 諸データ：回転(rad(x), rad(y), rad(z))
    Vector3 pos_min; // 制限：移動1(x, y, z)
    Vector3 pos_max; // 制限：移動2(x, y, z)
    Vector3 rot_min; // 制限：回転1(rad(x), rad(y), rad(z))
    Vector3 rot_max; // 制限：回転2(rad(x), rad(y), rad(z))
    Vector3 spring_pos; // ばね：移動(x, y, z)
    Vector3 spring_rot; // ばね：回転(rad(x), rad(y), rad(z))

  };

  struct Bone
  {
    Matrix transform;
    Matrix absolute;
    Matrix inverse;
    std::string name;
    std::vector<Shape> shape;
    std::vector<Bone> child;
    std::vector<PMDRigid> rigid;
    std::vector<PMDJoint> joint;

    VMDIK ik;
    std::string humanoid_name;

    inline Bone* GetChildFromName(const std::string& name) {
      if (name == this->name)
        return this;
      for (int i = 0; i < child.size(); i++)
        if (auto hit = child[i].GetChildFromName(name))
          return hit;
      return NULL;
    }
    inline Bone* HumanoidName(const std::string& name) {
      if (name == this->humanoid_name)
        return this;
      for (int i = 0; i < child.size(); i++)
        if (auto hit = child[i].HumanoidName(name))
          return hit;
      return NULL;
    }
    void ReverseCulling();
  };

  void Freeze(Shape& out, const Shape& in, const Matrix& mat);
  void Freeze(Shape& out, const Bone& bone, Matrix mat);

  struct AnimationKey
  {
    int interval;
    Quaternion rotation;
    Vector3 scale;
    Vector3 translation;
  };

  struct AnimationBone
  {
    std::string targetbone;
    std::vector<AnimationKey> key;
    int length;
    AnimationKey& GetKeyFromCount(int count);
    AnimationBone();
  };

  struct MorphKey {
    int interval;
    float weight;
  };

  struct Morph {
    std::string name;
    std::vector<MorphKey> key;
    int length;
  };

  struct Animation {
    std::vector<AnimationBone> bone;
    std::vector<Morph> morph;
  };

  typedef std::map<std::string, Animation> AnimationSet;

  struct Pose
  {
    Matrix absolute;
    Matrix base;
    Matrix relative;
    Quaternion local_rotation;
    std::string bonename;
    VMDIK ik;
    int parent;
    std::vector<int> child;
  };

  struct AnimatedPose
  {
    std::vector<Pose> pose;
    const Animation* anim;
    std::map<std::string, float> morph_weight;

    void CalcPoseFromAnim(float count);
    void SetTime(float count);
    void Update(Pose* root);
    const Pose* GetBone(const std::string& bonename)const;
    Pose* GetBoneNc(const std::string& bonename);
  };
  void CreateAnimatedPose(AnimatedPose& pose, const Bone& bone, const Animation& animation);

  bool ParseVRM(const char* buf, int len, Bone& root);
  bool ParsePMX(const char* buf, int len, Bone& root);
  bool ParsePMD(const char* buf, int len, Bone& root);
  bool ParseVMD(const char* buf, int len, Animation& root);
  bool ParseX(const char* buf, int len, Bone& root, AnimationSet& animation);

}

CONVERT_OBJECT_1(gh::Matrix, m)
CONVERT_OBJECT_4(gh::Color, r, g, b, a)
CONVERT_OBJECT_4(gh::Vector4, x, y, z, w)
CONVERT_OBJECT_4(gh::Quaternion, x, y, z, w)
CONVERT_OBJECT_3(gh::Vector3, x, y, z)
CONVERT_OBJECT_2(gh::Vector2, x, y)

CONVERT_OBJECT_4(gh::IndexVertex, position, normal, color, texture)
CONVERT_OBJECT_5(gh::Material, ambient, diffuse, specular, power, texture)
CONVERT_OBJECT_2(gh::Subset, indices, material)
CONVERT_OBJECT_2(gh::WeightIndex, index, weight)
CONVERT_OBJECT_4(gh::Weight, bonename, offset, position, normal)
CONVERT_OBJECT_6(gh::Shape, position, normal, color, texture, subset, weight)
CONVERT_OBJECT_4(gh::VMDIK, head, effects, iteration, limit_angle)
CONVERT_OBJECT_5(gh::Bone, transform, name, shape, child, ik)

#endif
