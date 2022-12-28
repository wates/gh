#include "scene.h"
#include "sys/fs.h"
#include "main/underscore.h"
#include "gfx/fertex.h"

using namespace gh;

class MeshImpl
  :public Mesh
{
  struct PntRefs
  {
    IndexType position;
    IndexType normal;
    IndexType color;
    IndexType tex1;
  };

  struct PntRefsPred {
    bool operator()(const PntRefs& a, const PntRefs& b)const {
      return *reinterpret_cast<const uint64_t*>(&a) < *reinterpret_cast<const uint64_t*>(&b);
    }
  };

  struct ReferencedCounter
  {
    int position;
    int normal;
    int color;
    int tex1;

    ReferencedCounter()
      :position(0), normal(0), color(0), tex1(0)
    {}
  };
  // the ARM align probrems
  //typedef std::vector<WeightIndex,char,4> BlendIndex;
  typedef std::vector<WeightIndex> BlendIndex;
  struct BlendCounter {
    uint32_t count;
    WeightIndex value[2];
  };

  struct MeshSubset
  {
    int faces;
    bool wire;
    Material material;
  };
  struct BoneIndex
  {
    Matrix offset;
    std::string index;
  };


  int* multi_form_offset_;
  Matrix* multi_form_;
  int using_forms_;
  int max_forms_;
  bool is_multi_form_;
  bool use_camera_;
  bool enable_add_alpha_;
  std::string prefix_;

  VertexBuffer* vb_;
  IndexBuffer* ib_;
  std::vector<MeshSubset> subset_;
  std::vector<BoneIndex> boneindex_;
  std::vector<Morphing> morphing_;
  std::vector<char> vtx;
  std::vector<Vector3> position_;

  gh::Graphics* gf;

  void Setup(gh::Graphics* g) {
    gf = g;
  }

  void Draw()
  {
    int offset = 0;
    for (int j = 0; j < subset_.size(); j++)
    {
      const MeshSubset& face = subset_[j];

      gf->DrawIndexedPrimitive(vb_, ib_, offset, face.faces);
      switch (ib_->Type()) {
      case PRIMITIVE_TRIANGLELIST:
        offset += face.faces * 3;
        break;
      case PRIMITIVE_LINELIST:
        offset += face.faces * 2;
        break;
      default:
        assert(false);
      }
    }
  }

  void Draw2(std::function<void(int)> f)
  {
    int offset = 0;
    for (int j = 0; j < subset_.size(); j++)
    {
      f(j);
      const MeshSubset& face = subset_[j];
      gf->DrawIndexedPrimitive(vb_, ib_, offset, face.faces);
      offset += face.faces * 3;
    }
  }

  void InitOneSkin(const Shape& shape, PrimitiveType primitive_type)
  {
    int format = 0;
    if (shape.position.size())
      format |= FTX_POSITION;
    if (shape.normal.size())
      format |= FTX_NORMAL;
    if (shape.color.size())
      format |= FTX_DIFFUSE;
    if (shape.texture.size())
      format |= FTX_TEX1;
    if (shape.tangent.size())
      format |= FTX_TANGENT;


    // 同じインデックスを参照している数をカウント
    std::vector<ReferencedCounter> referenced_count;
    referenced_count.resize(shape.position.size());
    for (int j = 0; j < shape.subset.size(); j++)
    {
      const Subset& subset = shape.subset[j];
      for (int i = 0; i < subset.indices.size(); i++)
      {
        referenced_count[subset.indices[i].position].position++;
        if (format & FTX_NORMAL)
          referenced_count[subset.indices[i].normal].normal++;
        if (format & FTX_DIFFUSE)
          referenced_count[subset.indices[i].color].color++;
        // TODO material???
        if (format & FTX_TEXTUREMASK)
          referenced_count[subset.indices[i].texture].tex1++;
      }
    }

    //ボーン毎に重みの参照先があるところを、頂点毎に重みへの参照先に置き換える
    //std::map<IndexType, BlendIndex> wic;
    std::vector<BlendCounter> wic;
    wic.resize(shape.position.size());
    for (auto& i : wic) {
      i.count = 0;
    }
    for (int j = 0; j < shape.weight.size(); j++)
    {
      const Weight& weight = shape.weight[j];
      for (int k = 0; k < weight.position.size(); k++)
      {
        //if (referenced_count.end() == referenced_count.find(weight.position[k].index))
        //  continue;
        if (weight.position[k].weight > 0)
        {
          auto& w = wic[weight.position[k].index];
          WeightIndex& wi = w.value[w.count++];
          wi.index = static_cast<IndexType>(j);
          wi.weight = weight.position[k].weight;
          if (w.count == 4) {
            break;
          }
        }
      }
    }
    //頂点毎に与えられた重みの数の最大値を出す
    int maxblends = 0;
    for (auto& i : wic) {
      if (i.count > maxblends)
        maxblends = i.count;
    }
    if (maxblends == 1)
      format |= FTX_WEIGHT1;
    else if (maxblends == 2)
      format |= FTX_WEIGHT2;
    else if (maxblends == 3)
      format |= FTX_WEIGHT2;
    else if (maxblends >= 4)
      format |= FTX_WEIGHT2;

    std::vector<int> bind_weight;
    bind_weight.resize(shape.weight.size());
    for (auto& i : bind_weight) {
      i = 0;
    }
    for (auto& n : wic) {
      for (int i = 0; i < n.count; i++) {
        if (n.value[i].weight > 0) {
          bind_weight[n.value[i].index]++;
        }
      }
    }

    //for(int i=0;i<bone_bind_count.bind_weight->size();i++){
    //    printf("%d- %d\n",i,bone_bind_count.bind_weight->operator[](i));            
    //}

    std::vector<int> shrinked_bone_index;
    int used_bone = 0;
    for (int i = 0; i < bind_weight.size(); i++)
    {
      if (bind_weight[i] > 0)
      {
        shrinked_bone_index.push_back(used_bone);
        used_bone++;
      }
      else
      {
        shrinked_bone_index.push_back(-1);
      }
    }

    boneindex_.resize(used_bone);
    for (int j = 0, i = 0; i < bind_weight.size(); i++)
    {
      if (bind_weight[i] > 0)
      {
        boneindex_[j].index = shape.weight[i].bonename;
        boneindex_[j].offset = shape.weight[i].offset;
        j++;
      }
    }

    //printf("used_bones %d\n",used_bone);

    //if(used_bone==1)
    //    format&=~FTX_WEIGHTMASK;

    std::vector<IndexType> idx;
    int vtxex = 0;
    //std::vector<PntRefs> iviv;
    std::map<PntRefs, int, PntRefsPred> indexed;
    std::map<int, int> position_to_vertex;//for morphing
    for (int j = 0; j < shape.subset.size(); j++)
    {
      const Subset& subset = shape.subset[j];
      for (int k = 0; k < subset.indices.size(); k++)
      {
        const IndexVertex& iv = subset.indices[k];
        PntRefs ref;
        //const PntRefs* piviv = iviv.data();

        ref.position =
          (format & FTX_POSITION) ? iv.position : -1;

        ref.normal =
          (format & FTX_NORMAL) ? iv.normal : -1;

        ref.color =
          (format & FTX_DIFFUSE) ? iv.color : -1;

        ref.tex1 =
          (format & FTX_TEX1) ? iv.texture : -1;

        //bool hit = false;
        //auto ivivsize = iviv.size();
        auto hit = indexed.find(ref);
        if (hit != indexed.end()) {
          idx.push_back(hit->second);
        }
        //for (IndexType index = 0; index < ivivsize; index++)
        //{
        //  if (((format&FTX_POSITION) ? ref.position == piviv[index].position : true)
        //    && ((format&FTX_NORMAL) ? ref.normal == piviv[index].normal : true)
        //    && ((format&FTX_DIFFUSE) ? ref.color == piviv[index].color : true)
        //    && ((format&FTX_TEX1) ? ref.tex1 == piviv[index].tex1 : true)
        //    )
        //  {
        //    hit = true;
        //    idx.push_back(index);
        //    break;
        //  }
        //}
        //if (!hit)
        else
        {
          idx.push_back((vtx.size() / GetFertexSize(format)));
          indexed[ref] = vtxex;
          position_to_vertex[ref.position] = vtxex;
          vtxex++;
          vtx.resize(vtx.size() + GetFertexSize(format));
          void* v = &*(&vtx.back() - GetFertexSize(format) + 1);
          if (format & FTX_POSITION) {
            GetFertexOffsetPosition(v, format) =
              Vec3(shape.position[ref.position].x, shape.position[ref.position].y, shape.position[ref.position].z);
          }
          if (format & FTX_NORMAL)
            GetFertexOffsetNormal(v, format) =
            Vec3(shape.normal[ref.normal].x, shape.normal[ref.normal].y, shape.normal[ref.normal].z);
          if (format & FTX_TANGENT)
            GetFertexOffsetTangent(v, format) =
            Vec3(shape.tangent[ref.normal].x, shape.tangent[ref.normal].y, shape.tangent[ref.normal].z);
          if (format & FTX_DIFFUSE)
            GetFertexOffsetDiffuse(v, format) =
            Color(shape.color[ref.color].r, shape.color[ref.color].g, shape.color[ref.color].b);
          if (FTX_WEIGHT1 == (format & FTX_WEIGHTMASK))
          {
            GetFertexOffsetWeight1(v, format).weightindex =
              static_cast<float>(shrinked_bone_index[wic[ref.position].value[0].index]);
          }
          if (FTX_WEIGHT2 == (format & FTX_WEIGHTMASK))
          {
            FTX_Type<FTX_WEIGHT2>::type& fv = GetFertexOffsetWeight2(v, format);
            BlendCounter& bi = wic[ref.position];
            fv.weightindex[0] =
              static_cast<float>(shrinked_bone_index[bi.value[0].index]);
            fv.weight[0] = bi.value[0].weight;
            if (1 < bi.count)
            {
              fv.weightindex[1] =
                static_cast<float>(shrinked_bone_index[bi.value[1].index]);
              fv.weight[1] = bi.value[1].weight;
            }
            else
            {
              fv.weightindex[1] = static_cast<float>(0);
              fv.weight[1] = 0;
            }
          }
          if (format & FTX_TEX1)
          {
            if (shape.texture.size())
            {
              GetFertexOffsetTex1(v, format).u = shape.texture[ref.tex1].x;
              GetFertexOffsetTex1(v, format).v = shape.texture[ref.tex1].y;
            }
            else
            {
              GetFertexOffsetTex1(v, format).u = 0;
              GetFertexOffsetTex1(v, format).v = 0;
            }
          }
          //iviv.push_back(ref);
        }
      }
    }

    // 初期値の保持とインデックスの置き換え
    this->morphing_ = shape.morphing;
    if (this->morphing_.size()) {
      char* head = reinterpret_cast<char*>(&GetFertexOffsetPosition(vtx.data(), format));
      int align = GetFertexSize(format);
      this->position_.resize(vtx.size() / align);
      for (auto& pos : this->position_) {
        pos = *reinterpret_cast<Vector3*>(head);
        head += align;
      }
      for (auto& m : this->morphing_) {
        for (int i = 0; i < m.target.size(); i++) {
          m.target[i].index = position_to_vertex[m.target[i].index];
        }
      }
    }

    gf->CreateVertexBuffer(&vb_, format, vtxex);
    gf->CreateIndexBuffer(&ib_, idx.size(), primitive_type);

    //static int polys=0;
    //polys+=idx.size()/3;
    //printf("tris=%d\n",polys);

    vb_->WriteVertex(&vtx.front());
    ib_->WriteIndex(&idx.front());
    subset_.resize(shape.subset.size());
    for (int j = 0; j < shape.subset.size(); j++)
    {
      const Subset& sub = shape.subset[j];
      switch (ib_->Type()) {
      case PRIMITIVE_TRIANGLELIST:
        subset_[j].faces = sub.indices.size() / 3;
        break;
      case PRIMITIVE_LINELIST:
        subset_[j].faces = sub.indices.size() / 2;
        break;
      default:
        assert(false);
      }
      subset_[j].material = sub.material;
      //subset_[j].decal = mg_->GetTexture((prefix_ + sub.material.texture).c_str());

      //subset_[j].center_specular = false;
      //subset_[j].edge_specular = false;
      //subset_[j].blow_normal = false;
      //subset_[j].wire = false;
      //subset_[j].set_rgba = false;
      //subset_[j].set_angle_rgba = false;
      //subset_[j].enable_normalmap = false;
      //subset_[j].enable_tangentmap = false;
      //subset_[j].enable_colored_normalmap = false;

#if 0
      int min = 100000;
      int max = 0;
      for (auto i : sub.indices) {
        for (auto j : wic[i.position]) {
          if (min > j.index)min = j.index;
          if (max < j.index)max = j.index;
        }
      }
      printf("%d - %d , %d\n", min, max, max - min);
#endif
    }
  }
};

Mesh* CreateMesh() {
  return new MeshImpl;
}

class SceneImpl :public Scene {
  std::map<std::string, Bone> bone_map_;
  gh::Fs* fs;
public:
  SceneImpl();
  Bone* GetBone(const std::string& name);


};

void CreateScene(Scene** a) {
  *a = new SceneImpl;
}

SceneImpl::SceneImpl()
{
  fs = gh::Fs::Create();
}

Bone* SceneImpl::GetBone(const std::string& name)
{
  if (_.has(bone_map_, name))
  {
    return &bone_map_[name];
  }
  else
  {
    fs->FetchFromFile(name);
    if (name.find(".x") == name.size() - 2)
    {
      if (fs->Status(name) == Fs::Complete)
      {
        AnimationSet as;
        ParseX((char*)fs->Data(name), fs->Length(name), bone_map_[name], as);
        //animation_map_[name] = as.begin()->second_.value; //TODO cant replace type
        return &bone_map_[name];
      }
      else
        return 0;
    }
    else if (name.find(".glb") == name.size() - 4 || name.find(".vrm") == name.size() - 4)
    {
      if (fs->Status(name) == Fs::Complete)
      {
        ParseVRM((char*)fs->Data(name), fs->Length(name), bone_map_[name]);
        return &bone_map_[name];
      }
      else
        return 0;
    }
    else if (name.find(".pmx") == name.size() - 4)
    {
      if (fs->Status(name) == Fs::Complete)
      {
        ParsePMX((char*)fs->Data(name), fs->Length(name), bone_map_[name]);
        return &bone_map_[name];
      }
      else
        return 0;
    }
    else if (name.find(".mdl") == name.size() - 4)
    {
      if (fs->Status(name) == Fs::Complete)
      {
        BinaryReader bread;
        bread.position = reinterpret_cast<const char*>(fs->Data(name));
        bread.end = bread.position + fs->Length(name);
        Convert(bread, bone_map_[name], "");
        return &bone_map_[name];
      }
      else
        return 0;
    }
    else
      return 0;

  }
}
