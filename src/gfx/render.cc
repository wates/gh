
#include "render.h"
#include "../sys/fs.h"
#include <assert.h>
#include "png.h"
#include "fertex.h"
#include <stdio.h>
#include <set>
#include "../main/underscore.h"
#include "../sys/fs.h"

using namespace gh;

namespace
{
  struct PntRefs
  {
    IndexType position;
    IndexType normal;
    IndexType color;
    IndexType tex1;
  };

  struct PntRefsPred {
    bool operator()(const PntRefs &a, const PntRefs &b)const {
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
  //struct SearchOfMaxblends
  //{
  //    int maxblends;
  //    SearchOfMaxblends():maxblends(0){}
  //    void operator()(const std::map<IndexType,BlendIndex>::value_type &in)
  //    {
  //        if(in.second.size()>maxblends)
  //            maxblends=in.second.size();
  //    }
  //};

  //struct BoneBindCount
  //{
  //  std::vector<int> *bind_weight;
  //  BoneBindCount(std::vector<int> *counter) :bind_weight(counter){}
  //  void operator()(const std::map<IndexType, BlendIndex>::value_type &in)
  //  {
  //    for (int i = 0; i < in.second.size(); i++)
  //      if (in.second[i].weight>0)
  //        (*bind_weight)[in.second[i].index]++;
  //  }
  //};

  //struct AllRelease
  //{
  //  template<typename T>
  //  void operator()(const T &kv)
  //  {
  //    kv.value->Release();
  //  }
  //};

  struct Edge {
    IndexType a, b;
    bool operator <(const Edge &t)const {
      if (a == t.a)
        return b < t.b;
      return a < t.a;
    }
    Edge Swap()const {
      Edge e = { b, a };
      return e;
    }
  };
}



class MeshImpl
  :public Mesh
{
  struct MeshSubset
  {
    int faces;
    bool wire;
    Material material;
    Texture *decal;
    bool center_specular;
    Color center_specular_color;
    bool edge_specular;
    Color edge_specular_color;
    float edge_specular_power;
    bool blow_normal;
    float blow_normal_scale;
    bool set_rgba;
    Color set_rgba_color;
    bool set_angle_rgba;
    Color set_angle_rgba_center;
    Color set_angle_rgba_edge;
    bool enable_normalmap;
    bool enable_tangentmap;
    Color tangentmap_color;
    float tangentmap_power;
    bool enable_colored_normalmap;
    Color colored_normalmap_color;
    Color colored_normalmap_specular;
    float colored_normalmap_power;
    Texture *screenspace_diffuse;
  };
public:
  void SetPathPrefix(const std::string prefix) {
    prefix_ = prefix;
  }
  void InitOneSkin(const Shape &shape, PrimitiveType primitive_type)
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
      const Subset &subset = shape.subset[j];
      for (int i = 0; i < subset.indices.size(); i++)
      {
        referenced_count[subset.indices[i].position].position++;
        if (format&FTX_NORMAL)
          referenced_count[subset.indices[i].normal].normal++;
        if (format&FTX_DIFFUSE)
          referenced_count[subset.indices[i].color].color++;
        // TODO material???
        if (format&FTX_TEXTUREMASK)
          referenced_count[subset.indices[i].texture].tex1++;
      }
    }

    //ボーン毎に重みの参照先があるところを、頂点毎に重みへの参照先に置き換える
    //std::map<IndexType, BlendIndex> wic;
    std::vector<BlendCounter> wic;
    wic.resize(shape.position.size());
    for (auto &i : wic) {
      i.count = 0;
    }
    for (int j = 0; j < shape.weight.size(); j++)
    {
      const Weight &weight = shape.weight[j];
      for (int k = 0; k < weight.position.size(); k++)
      {
        //if (referenced_count.end() == referenced_count.find(weight.position[k].index))
        //  continue;
        if (weight.position[k].weight > 0)
        {
          auto &w = wic[weight.position[k].index];
          WeightIndex &wi = w.value[w.count++];
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
    for (auto &i : wic) {
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
    for (auto &i : bind_weight) {
      i = 0;
    }
    for (auto &n : wic) {
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
      const Subset &subset = shape.subset[j];
      for (int k = 0; k < subset.indices.size(); k++)
      {
        const IndexVertex &iv = subset.indices[k];
        PntRefs ref;
        //const PntRefs* piviv = iviv.data();

        ref.position =
          (format&FTX_POSITION) ? iv.position : -1;

        ref.normal =
          (format&FTX_NORMAL) ? iv.normal : -1;

        ref.color =
          (format&FTX_DIFFUSE) ? iv.color : -1;

        ref.tex1 =
          (format&FTX_TEX1) ? iv.texture : -1;

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
          void *v = &*(&vtx.back() - GetFertexSize(format) + 1);
          if (format&FTX_POSITION) {
            GetFertexOffsetPosition(v, format) =
              Vec3(shape.position[ref.position].x, shape.position[ref.position].y, shape.position[ref.position].z);
          }
          if (format&FTX_NORMAL)
            GetFertexOffsetNormal(v, format) =
            Vec3(shape.normal[ref.normal].x, shape.normal[ref.normal].y, shape.normal[ref.normal].z);
          if (format&FTX_TANGENT)
            GetFertexOffsetTangent(v, format) =
            Vec3(shape.tangent[ref.normal].x, shape.tangent[ref.normal].y, shape.tangent[ref.normal].z);
          if (format&FTX_DIFFUSE)
            GetFertexOffsetDiffuse(v, format) =
            Color(shape.color[ref.color].r, shape.color[ref.color].g, shape.color[ref.color].b);
          if (FTX_WEIGHT1 == (format&FTX_WEIGHTMASK))
          {
            GetFertexOffsetWeight1(v, format).weightindex =
              static_cast<float>(shrinked_bone_index[wic[ref.position].value[0].index]);
          }
          if (FTX_WEIGHT2 == (format&FTX_WEIGHTMASK))
          {
            FTX_Type<FTX_WEIGHT2>::type &fv = GetFertexOffsetWeight2(v, format);
            BlendCounter &bi = wic[ref.position];
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
          if (format&FTX_TEX1)
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
      char *head = reinterpret_cast<char*>(&GetFertexOffsetPosition(vtx.data(), format));
      int align = GetFertexSize(format);
      this->position_.resize(vtx.size() / align);
      for (auto &pos : this->position_) {
        pos = *reinterpret_cast<Vector3*>(head);
        head += align;
      }
      for (auto &m : this->morphing_) {
        for (int i = 0; i < m.target.size(); i++) {
          m.target[i].index = position_to_vertex[m.target[i].index];
        }
      }
    }
    // make device objects

    Graphics *raw = mg_->Raw();

    raw->CreateVertexBuffer(&vb_, format, vtxex);
    raw->CreateIndexBuffer(&ib_, idx.size(), primitive_type);

    //static int polys=0;
    //polys+=idx.size()/3;
    //printf("tris=%d\n",polys);

    vb_->WriteVertex(&vtx.front());
    ib_->WriteIndex(&idx.front());
    subset_.resize(shape.subset.size());
    for (int j = 0; j < shape.subset.size(); j++)
    {
      const Subset &sub = shape.subset[j];
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
      subset_[j].decal = mg_->GetTexture((prefix_ + sub.material.texture).c_str());

      subset_[j].center_specular = false;
      subset_[j].edge_specular = false;
      subset_[j].blow_normal = false;
      subset_[j].wire = false;
      subset_[j].set_rgba = false;
      subset_[j].set_angle_rgba = false;
      subset_[j].enable_normalmap = false;
      subset_[j].enable_tangentmap = false;
      subset_[j].enable_colored_normalmap = false;

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

  void InitOneSkinMultiForm(const Shape &in, int max_form)
  {
    this->max_forms_ = max_form;
    this->multi_form_offset_ = new int[max_form];
    this->multi_form_ = new Matrix[max_form];
    //マージ用
    Shape shape;
    int faces = 0;
    if (0x10000 < (int)in.position.size()*max_forms_ ||
      0x10000 < (int)in.normal.size()*max_forms_)
    {
      assert(false);
    }
    for (int i = 0; i < max_forms_; i++)
    {
      int begin_pos = shape.position.size();
      int begin_norm = shape.normal.size();

      //頂点マージ
      _.push(shape.position, in.position.data(), in.position.size());
      _.push(shape.normal, in.normal.data(), in.normal.size());

      //面マージ
      Subset &sb = _.push(shape.subset);
      sb.indices = in.subset[0].indices;
      for (int n = 0; n < sb.indices.size(); n++) {
        sb.indices[n].position += begin_pos;
        sb.indices[n].normal += begin_norm;
      }

      faces += shape.subset.back().indices.size() / 3;
      this->multi_form_offset_[i] = faces;
      int end_pos = shape.position.size();
      int end_norm = shape.normal.size();

      //重み追加
      Weight &w = _.push(shape.weight);
      w.offset.Identity();
      w.position.resize(end_pos - begin_pos);
      for (int n = 0; n < end_pos - begin_pos; n++) {
        w.position[n].index = begin_pos + n;
        w.position[n].weight = 1.0f;
      }
      w.normal.resize(end_norm - begin_norm);
      for (int n = 0; n < end_norm - begin_norm; n++) {
        w.normal[n].index = begin_norm + n;
        w.normal[n].weight = 1.0f;
      }
    }
    shape.subset[0].material = in.subset[0].material;
    InitOneSkin(shape, PRIMITIVE_TRIANGLELIST);
    is_multi_form_ = true;
  }


  void InitWireframe(const Shape &shape)
  {
    int format = 0;
    format = FTX_POSITION;


    // 同じインデックスを参照している数をカウント
    std::map<IndexType, int> referenced_count;
    for (int j = 0; j < shape.subset.size(); j++)
    {
      const Subset &subset = shape.subset[j];
      for (int i = 0; i < subset.indices.size(); i++)
      {
        referenced_count[subset.indices[i].position]++;
      }
    }

    //ボーン毎に重みの参照先があるところを、頂点毎に重みへの参照先に置き換える
    std::map<IndexType, BlendIndex> wic;
    for (int j = 0; j < shape.weight.size(); j++)
    {
      const Weight &weight = shape.weight[j];
      for (int k = 0; k < weight.position.size(); k++)
      {
        if (referenced_count.end() == referenced_count.find(weight.position[k].index))
          continue;
        if (weight.position[k].weight > 0)
        {
          WeightIndex &wi = _.push(wic[weight.position[k].index]);
          wi.index = static_cast<IndexType>(j);
          wi.weight = weight.position[k].weight;
        }
      }
    }
    //頂点毎に与えられた重みの数の最大値を出す
    int maxblends = 0;
    for (auto &i : wic) {
      if (i.second.size() > maxblends)
        maxblends = i.second.size();
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
    for (auto &i : bind_weight) {
      i = 0;
    }
    for (auto &n : wic) {
      for (int i = 0; i < n.second.size(); i++) {
        if (n.second[i].weight > 0) {
          bind_weight[n.second[i].index]++;
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
    std::vector<IndexType> iviv;
    std::set<Edge> edge;
    for (int j = 0; j < shape.subset.size(); j++)
    {
      const Subset &subset = shape.subset[j];
      Edge tri[3];
      for (int k = 0; k < subset.indices.size(); k++)
      {
        const IndexVertex &iv = subset.indices[k];
        IndexType ref_pos;

        ref_pos = iv.position;

        bool hit = false;
        IndexType target;
        for (IndexType index = 0; index < iviv.size(); index++)
        {
          if (ref_pos == iviv[index])
          {
            hit = true;
            target = index;
            break;
          }
        }
        if (!hit)
        {
          target = (IndexType)(vtx.size() / GetFertexSize(format));
          vtxex++;
          vtx.resize(vtx.size() + GetFertexSize(format));
          void *v = &*(&vtx.back() - GetFertexSize(format) + 1);
          if (format&FTX_POSITION)
            GetFertexOffsetPosition(v, format) =
            Vec3(shape.position[ref_pos].x, shape.position[ref_pos].y, shape.position[ref_pos].z);
          if (FTX_WEIGHT1 == (format&FTX_WEIGHTMASK))
          {
            GetFertexOffsetWeight1(v, format).weightindex =
              static_cast<float>(shrinked_bone_index[wic[ref_pos][0].index]);
          }
          iviv.push_back(ref_pos);
        }
        tri[(k) % 3].a = target;
        tri[(k + 1) % 3].b = target;
        if (k % 3 == 2) {
          for (int i = 0; i < 3; i++) {
            if (edge.end() == edge.find(tri[i]) &&
              edge.end() == edge.find(tri[i].Swap())) {
              edge.insert(tri[i]);
              idx.push_back(tri[i].a);
              idx.push_back(tri[i].b);
            }
          }
        }
      }
    }

    // make device objects

    Graphics *raw = mg_->Raw();

    raw->CreateVertexBuffer(&vb_, format, vtxex);
    raw->CreateIndexBuffer(&ib_, idx.size(), PRIMITIVE_LINELIST);

    //static int polys=0;
    //polys+=idx.size()/3;
    //printf("tris=%d\n",polys);

    vb_->WriteVertex(&vtx.front());
    ib_->WriteIndex(&idx.front());
    subset_.resize(shape.subset.size());
    for (int j = 0; j < shape.subset.size(); j++)
    {
      const Subset &sub = shape.subset[j];
      subset_[j].faces = sub.indices.size() / 2;
      subset_[j].material = sub.material;
      subset_[j].decal = NULL;

      subset_[j].center_specular = false;
      subset_[j].edge_specular = false;
      subset_[j].blow_normal = false;
      subset_[j].wire = true;
      subset_[j].set_rgba = false;
      subset_[j].set_angle_rgba = false;
      subset_[j].enable_normalmap = false;
      subset_[j].enable_tangentmap = false;
      subset_[j].enable_colored_normalmap = false;
    }
  }

  void SetCenterSpecular(int subset, const Color &color) {
    if (subset_.size() > subset) {
      subset_[subset].center_specular = 0 < color.a;
      subset_[subset].center_specular_color = color;
    }
  }

  void SetEdgeSpecular(int subset, const Color &color, float power) {
    if (subset_.size() > subset) {
      subset_[subset].edge_specular = 0 < color.a;
      subset_[subset].edge_specular_color = color;
      subset_[subset].edge_specular_power = power;
    }
  }

  void SetBlowNormal(int subset, float scale) {
    if (subset_.size() > subset) {
      subset_[subset].blow_normal = true;
      subset_[subset].blow_normal_scale = scale;
    }
  }

  void SetRGBA(int subset, const Color &col) {
    if (subset_.size() > subset) {
      subset_[subset].set_rgba = true;
      subset_[subset].set_rgba_color = col;
    }
  }

  void SetAngleRGBA(int subset, const Color &center, const Color &edge) {
    if (subset_.size() > subset) {
      subset_[subset].set_angle_rgba = true;
      subset_[subset].set_angle_rgba_center = center;
      subset_[subset].set_angle_rgba_edge = edge;
    }
  }

  void UseCamera(bool use) {
    use_camera_ = use;
  }

  void EnableAddAlpha() {
    enable_add_alpha_ = true;
  }

  void EnableNormalmap(int subset)
  {
    subset_[subset].enable_normalmap = true;
  }

  void EnableTangentmap(int subset, const Color &color, float power)
  {
    subset_[subset].enable_tangentmap = true;
    subset_[subset].tangentmap_color = color;
    subset_[subset].tangentmap_power = power;
  }

  void EnableColoredNormalmap(const Color &color, const Color &specular, float power)
  {
    int subset = 0;
    subset_[subset].enable_colored_normalmap = true;
    subset_[subset].colored_normalmap_color = color;
    subset_[subset].colored_normalmap_specular = specular;
    subset_[subset].colored_normalmap_power = power;
  }
  void ApplyMorph(const std::vector<float> &weight)
  {
    char *head = reinterpret_cast<char*>(&GetFertexOffsetPosition(vtx.data(), vb_->Format()));
    int align = GetFertexSize(vb_->Format());
    const Morphing &base = morphing_[0];
    for (int i = 0; i < base.target.size(); i++) {
      Vector3 *pos = reinterpret_cast<Vector3*>(head + align * base.target[i].index);
      *pos = position_[base.target[i].index];
    }
    for (int i = 0; i < weight.size(); i++) {
      const float w = weight[i];
      if (0 < w) {
        const auto &m = morphing_[i + 1].target;
        for (int j = 0; j < m.size(); j++) {
          Vector3 *pos = reinterpret_cast<Vector3*>(head + align * m[j].index);
          *pos += m[j].position*w;
        }
      }
    }
    vb_->WriteVertex(&vtx.front());
  }
  void ApplyNamedMorph(const std::map<std::string, float> &weight) {
    if (this->morphing_.size()) {
      std::vector<float> w;
      w.resize(this->morphing_.size() - 1);
      for (int i = 1; i < this->morphing_.size(); i++) {
        auto h = weight.find(this->morphing_[i].name);
        if (h != weight.end()) {
          w[i - 1] = h->second;
        }
        else {
          w[i - 1] = 0;
        }
      }
      ApplyMorph(w);
    }
  }

  int AppendEffectShaders(const MeshSubset &face, bool is_blend) {
    Graphics *raw = mg_->Raw();
    int slot = 0;

    if (face.blow_normal)
    {
      mg_->blow_normal_->Set(face.blow_normal_scale);
      raw->SetShader(mg_->blow_normal_, slot++);
    }
    if (is_blend) {
      raw->SetShader(mg_->blend_transform_, slot++);
    }
    else {
      raw->SetShader(mg_->world_transform_, slot++);
    }
    return AppendEffect(face, slot);
  }

  void SetScreenSpaceDiffuse(int subset, Texture * diffuse)
  {
    subset_[subset].screenspace_diffuse = diffuse;
  }

  int AppendEffect(const MeshSubset &face, int slot) {
    Graphics *raw = mg_->Raw();

    if (mg_->directional_light_enable_ && !face.wire &&
      (face.material.diffuse.HasColor() || 0 != face.material.power))
    {
      raw->SetShader(mg_->directional_light_, slot++);
    }
    if (face.set_rgba)
    {
      mg_->set_rgba_->Set(face.set_rgba_color);
      raw->SetShader(mg_->set_rgba_, slot++);
    }
    if (face.set_angle_rgba)
    {
      mg_->set_angle_rgba_->Set(face.set_angle_rgba_center, face.set_angle_rgba_edge);
      raw->SetShader(mg_->set_angle_rgba_, slot++);
    }
    if (face.enable_normalmap)
    {
      mg_->normal_map_->SetTexture(face.decal);
      raw->SetShader(mg_->normal_map_, slot++);
    }
    else if (face.enable_tangentmap)
    {
      mg_->tangent_map_->SetTexture(face.decal);
      mg_->tangent_map_->Specular(face.tangentmap_color);
      mg_->tangent_map_->Power(face.tangentmap_power);
      raw->SetShader(mg_->tangent_map_, slot++);
    }
    else if (face.decal && !face.wire)
    {
      mg_->decal_texture_->SetTexture(face.decal);
      raw->SetShader(mg_->decal_texture_, slot++);
    }
    if (face.enable_colored_normalmap)
    {
      mg_->colored_normalmap_->BaseColor(face.colored_normalmap_color);
      mg_->colored_normalmap_->CenterSpecular(face.colored_normalmap_specular);
      mg_->colored_normalmap_->CenterPower(face.colored_normalmap_power);
      raw->SetShader(mg_->colored_normalmap_, slot++);
    }
    if (face.screenspace_diffuse && !face.wire) {
      mg_->ssdiffuse_->SetTexture(face.screenspace_diffuse);
      raw->SetShader(mg_->ssdiffuse_, slot++);
    }
    else if ((face.material.diffuse.HasColor()) && !face.wire) {
      mg_->diffuse_light_->Diffuse(face.material.diffuse);
      raw->SetShader(mg_->diffuse_light_, slot++);
      mg_->ambient_light_->Ambient(face.material.ambient);
      raw->SetShader(mg_->ambient_light_, slot++);
    }
    if (face.material.power && !face.wire)
    {
      mg_->specular_light_->Specular(face.material.specular);
      mg_->specular_light_->Power(face.material.power);
      raw->SetShader(mg_->specular_light_, slot++);
    }
    if (face.center_specular && !face.wire)
    {
      mg_->center_specular_->Specular(face.center_specular_color);
      raw->SetShader(mg_->center_specular_, slot++);
    }
    if (face.edge_specular && !face.wire)
    {
      mg_->edge_specular_->Specular(face.edge_specular_color);
      mg_->edge_specular_->Power(face.edge_specular_power);
      raw->SetShader(mg_->edge_specular_, slot++);
    }
    if (use_camera_) {
      raw->SetShader(mg_->camera_, slot++);
    }
    raw->SetShader(shader::ShaderEnd(), slot++);
    return slot;
  }

  void DrawMultiForm(const Matrix *mats, int count, int faces)
  {
    Graphics *raw = mg_->Raw();

    if (enable_add_alpha_)
    {
      raw->SetRenderState(RENDER_ZWRITE, VALUE_DISABLE);
      raw->SetAlphaBlendMode(ALPHABLEND_ADD);
    }
    for (int i = 0; i < count; i++)
      mg_->blend_transform_->Transform(mats[i], i);

    AppendEffectShaders(subset_[0], true);

    raw->DrawIndexedPrimitive(vb_, ib_, 0, faces);
    if (enable_add_alpha_)
    {
      raw->SetAlphaBlendMode(ALPHABLEND_NONE);
      raw->SetRenderState(RENDER_ZWRITE, VALUE_ENABLE);
    }
  }

  void FlushMultiform() {
    if (0 < using_forms_) {
      DrawMultiForm(this->multi_form_,
        this->using_forms_,
        this->multi_form_offset_[this->using_forms_ - 1]);
      this->using_forms_ = 0;
    }
  }

  void Draw(const Matrix &mat)
  {
    if (is_multi_form_) {
      this->multi_form_[this->using_forms_++] = mat;
      if (max_forms_ == this->using_forms_) {
        FlushMultiform();
      }
    }
    else {
      int offset = 0;
      Graphics *raw = mg_->Raw();

      mg_->world_transform_->Transform(mat);
      if (enable_add_alpha_)
      {
        mg_->Raw()->SetRenderState(RENDER_ZWRITE, VALUE_DISABLE);
        mg_->Raw()->SetAlphaBlendMode(ALPHABLEND_ADD);
      }
      for (int j = 0; j < subset_.size(); j++)
      {
        const MeshSubset &face = subset_[j];
        AppendEffectShaders(face, false);

        raw->DrawIndexedPrimitive(vb_, ib_, offset, face.faces);
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
      if (enable_add_alpha_)
      {
        mg_->Raw()->SetAlphaBlendMode(ALPHABLEND_NONE);
        mg_->Raw()->SetRenderState(RENDER_ZWRITE, VALUE_ENABLE);
      }
    }
  }
  void DrawFaces(const Matrix &mat, int faces)
  {
    Graphics *raw = mg_->Raw();

    mg_->world_transform_->Transform(mat);
    const MeshSubset &face = subset_[0];
    AppendEffectShaders(face, false);

    raw->DrawIndexedPrimitive(vb_, ib_, 0, faces);
  }
  void DrawPose(const Matrix &mat, AnimatedPose * pose)
  {
    mg_->world_transform_->Transform(mat);

    shader::BlendTransform *bt = mg_->huge_blend_transform_;
    int slot = 0;
    bt->ClearTransform();
    for (int j = 0; j < boneindex_.size(); j++)
    {
      Matrix world = boneindex_[j].offset;
      const Pose *p = pose->GetBone(boneindex_[j].index);
      if (p)
        world *= p->absolute;
      else
        assert(0);
      //world.Identity();
      bt->Transform(world, j);
    }
    mg_->Raw()->SetShader(bt, slot++);
    mg_->Raw()->SetShader(mg_->world_transform_, slot++);
    int offset = 0;
    for (int j = 0; j < subset_.size(); j++)
    {
      const auto &face = subset_[j];
      if (face.decal && kHasAlpha[face.decal->Desc().format]) {
        offset += face.faces * 3;
        continue;
      }
      AppendEffect(face, slot);
      mg_->Raw()->DrawIndexedPrimitive(vb_, ib_, offset, face.faces);
      offset += face.faces * 3;
    }
    offset = 0;
    mg_->Raw()->SetAlphaBlendMode(ALPHABLEND_MODULATE);
    for (int j = 0; j < subset_.size(); j++)
    {
      const auto &face = subset_[j];
      if (face.decal && kHasAlpha[face.decal->Desc().format]) {
        AppendEffect(face, slot);
        mg_->Raw()->DrawIndexedPrimitive(vb_, ib_, offset, face.faces);
      }
      offset += face.faces * 3;
    }
    mg_->Raw()->SetAlphaBlendMode(ALPHABLEND_NONE);
  }

  void DrawPoseLighting(const Matrix &mat, AnimatedPose * pose)
  {
    mg_->world_transform_->Transform(mat);

    shader::BlendTransform *bt = mg_->huge_blend_transform_;
    int slot = 0;
    bt->ClearTransform();
    for (int j = 0; j < boneindex_.size(); j++)
    {
      Matrix world = boneindex_[j].offset;
      const Pose *p = pose->GetBone(boneindex_[j].index);
      if (p)
        world *= p->absolute;
      //world.Identity();
      bt->Transform(world, j);
    }
    mg_->Raw()->SetShader(bt, slot++);
    mg_->Raw()->SetShader(mg_->world_transform_, slot++);
    int offset = 0;
    auto raw = mg_->Raw();
    const auto bslot = slot;
    for (int j = 0; j < subset_.size(); j++)
    {
      const auto &face = subset_[j];
      slot = bslot;
      if (mg_->directional_light_enable_ && !face.wire &&
        (face.material.diffuse.HasColor() || 0 != face.material.power))
      {
        raw->SetShader(mg_->directional_light_, slot++);
      }
      //if (face.set_angle_rgba)
      //{
      //  mg_->set_angle_rgba_->Set(face.set_angle_rgba_center, face.set_angle_rgba_edge);
      //  raw->SetShader(mg_->set_angle_rgba_, slot++);
      //}
      if ((face.material.diffuse.HasColor()) && !face.wire) {
        mg_->diffuse_light_->Diffuse(face.material.diffuse);
        raw->SetShader(mg_->diffuse_light_, slot++);
        mg_->ambient_light_->Ambient(face.material.ambient);
        raw->SetShader(mg_->ambient_light_, slot++);
      }
      //if (face.material.power && !face.wire)
      //{
      //  mg_->specular_light_->Specular(face.material.specular);
      //  mg_->specular_light_->Power(face.material.power);
      //  raw->SetShader(mg_->specular_light_, slot++);
      //}
      //if (face.center_specular && !face.wire)
      //{
      //  mg_->center_specular_->Specular(face.center_specular_color);
      //  raw->SetShader(mg_->center_specular_, slot++);
      //}
      //if (face.edge_specular && !face.wire)
      //{
      //  mg_->edge_specular_->Specular(face.edge_specular_color);
      //  mg_->edge_specular_->Power(face.edge_specular_power);
      //  raw->SetShader(mg_->edge_specular_, slot++);
      //}
      if (use_camera_) {
        raw->SetShader(mg_->camera_, slot++);
      }
      raw->SetShader(shader::ShaderEnd(), slot++);
      mg_->Raw()->DrawIndexedPrimitive(vb_, ib_, offset, face.faces);
      offset += face.faces * 3;
    }
  }
  MeshImpl(ManagedGraphics *mg)
    :multi_form_offset_(0)
    , multi_form_(NULL)
    , using_forms_(0)
    , max_forms_(0)
    , is_multi_form_(false)
    , use_camera_(true)
    , enable_add_alpha_(false)
    , mg_(mg)
    , vb_(NULL)
    , ib_(NULL)
  {
  }
  void Release()
  {
    if (is_multi_form_)
    {
      delete[]multi_form_offset_;
      delete[]multi_form_;
    }
    Graphics *raw = mg_->Raw();
    for (int j = 0; j < subset_.size(); j++)
    {
      const MeshSubset &subset = subset_[j];
    }
    ib_->Release();
    vb_->Release();

    delete this;
  }
private:
  ~MeshImpl() {
  }
private:
  int *multi_form_offset_;
  Matrix *multi_form_;
  int using_forms_;
  int max_forms_;
  bool is_multi_form_;
  bool use_camera_;
  bool enable_add_alpha_;
  std::string prefix_;

  ManagedGraphics *mg_;
  VertexBuffer *vb_;
  IndexBuffer *ib_;
  std::vector<MeshSubset> subset_;
  std::vector<BoneIndex> boneindex_;
  std::vector<Morphing> morphing_;
  std::vector<char> vtx;
  std::vector<Vector3> position_;
};

Mesh* ManagedGraphics::CreateMesh()
{
  return new MeshImpl(this);
}

class GridImpl
  :public Grid
{
  typedef Fertex<
    FTX_POSITION |
    FTX_TEX1> FtxFormat;

  static const int kTip = 8;

public:
  GridImpl(ManagedGraphics *mg)
    :mg_(mg)
    , width_(0)
    , height_(0)
    , vb(NULL)
    , ib(NULL)
  {

  }
  ~GridImpl()
  {
    if (vb)
      vb->Release();
    if (ib)
      ib->Release();
  }
  bool Build(int width, int height, const std::string &texture)
  {
    if (vb)
      vb->Release();
    if (ib)
      ib->Release();

    width_ = width;
    height_ = height;
    texture_ = mg_->GetTexture(texture.data());
    if (NULL == texture_)
      return false;
    std::vector<IndexType> idx;

    idx.resize(width*height * 6);
    IndexType *i = &idx.front();
    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        *i++ = 0xffff & ((y*width + x) * 4 + 0);
        *i++ = 0xffff & ((y*width + x) * 4 + 2);
        *i++ = 0xffff & ((y*width + x) * 4 + 1);

        *i++ = 0xffff & ((y*width + x) * 4 + 1);
        *i++ = 0xffff & ((y*width + x) * 4 + 2);
        *i++ = 0xffff & ((y*width + x) * 4 + 3);
      }
    }

    mg_->Raw()->CreateVertexBuffer(&vb, FtxFormat::format, width*height * 4);
    mg_->Raw()->CreateIndexBuffer(&ib, idx.size());
    ib->WriteIndex(&idx.front());

    tile_.resize(width*height);
    for (int y = 0; y < height_; y++)
      for (int x = 0; x < width; x++)
        tile_[width*y + x] = 0;

    UpdateVertexBuffer();
    return true;
  }
  void UpdateTile(const int *tile)
  {
    tile_.clear();
    tile_.insert(tile_.begin(), tile, tile + width_ * height_);
    UpdateVertexBuffer();
  }
  void UpdateVertexBuffer()
  {
    std::vector<FtxFormat> v;
    v.resize(width_*height_ * 4);

    float utip = 1.0f / kTip;
    float vtip = 1.0f / kTip;

    float texel = 1.0f / texture_->Desc().width;

    for (int y = 0; y < height_; y++)
    {
      for (int x = 0; x < width_; x++)
      {
        const int tnum = tile_[width_*(height_ - y - 1) + x];
        const int tu = tnum % kTip;
        const int tv = tnum / kTip;
        const int n = (y*width_ + x) * 4;
        v[n + 0].position = Vec3(x, y, 0.0f);
        v[n + 0].u = (tu + 0)*utip + texel;
        v[n + 0].v = (tv + 1)*vtip - texel;
        v[n + 1].position = Vec3(x + 1, y, 0.0f);
        v[n + 1].u = (tu + 1)*utip - texel;
        v[n + 1].v = (tv + 1)*vtip - texel;
        v[n + 2].position = Vec3(x, y + 1, 0.0f);
        v[n + 2].u = (tu + 0)*utip + texel;
        v[n + 2].v = (tv + 0)*vtip + texel;
        v[n + 3].position = Vec3(x + 1, y + 1, 0.0f);
        v[n + 3].u = (tu + 1)*utip - texel;
        v[n + 3].v = (tv + 0)*vtip + texel;
      }
    }
    vb->WriteVertex(v.data());
  }
  void Draw(const Matrix &mat)
  {
    mg_->world_transform_->Transform(mat);
    mg_->decal_texture_->SetTexture(texture_);

    mg_->Raw()->SetShader(mg_->world_transform_, 0);
    mg_->Raw()->SetShader(mg_->decal_texture_, 1);
    mg_->Raw()->SetShader(shader::ShaderEnd(), 2);

    mg_->Raw()->DrawIndexedPrimitive(vb, ib, 0, width_*height_ * 2);
  }
private:
  ManagedGraphics * mg_;
  int width_;
  int height_;
  VertexBuffer *vb;
  IndexBuffer *ib;
  Texture *texture_;
  float angle_;
  float tilt_;
  std::vector<int> tile_;
};

class ParticleImpl
  :public Particle
{
  typedef Fertex<
    FTX_POSITION |
    FTX_NORMAL |
    FTX_TEX1> FtxFormat;

public:
  ParticleImpl(ManagedGraphics *mg)
    :mg_(mg)
    , vb(NULL)
    , ib(NULL)
  {

  }
  ~ParticleImpl()
  {
    if (vb)
      vb->Release();
    if (ib)
      ib->Release();
  }
  bool Init(const std::string &texture, std::vector<Chip > motion)
  {
    if (vb)
      vb->Release();
    if (ib)
      ib->Release();

    texture_ = mg_->GetTexture(texture.data());

    num_ = motion.size();

    std::vector<IndexType> idx;

    idx.resize(num_ * 6);
    IndexType *i = &idx.front();
    for (int x = 0; x < num_; x++)
    {
      *i++ = 0xffff & (x * 4 + 0);
      *i++ = 0xffff & (x * 4 + 2);
      *i++ = 0xffff & (x * 4 + 1);

      *i++ = 0xffff & (x * 4 + 1);
      *i++ = 0xffff & (x * 4 + 2);
      *i++ = 0xffff & (x * 4 + 3);
    }

    mg_->Raw()->CreateIndexBuffer(&ib, num_ * 6);
    ib->WriteIndex(idx.data());

    std::vector<FtxFormat> v;
    v.resize(num_ * 4);

    for (int x = 0; x < num_; x++)
    {
      const int n = x * 4;
      v[n + 0].position = motion[x].velocity;
      v[n + 0].normal = motion[x].acceleration;
      v[n + 0].u = 0;
      v[n + 0].v = 0;
      v[n + 1].position = motion[x].velocity;
      v[n + 1].normal = motion[x].acceleration;
      v[n + 1].u = 1;
      v[n + 1].v = 0;
      v[n + 2].position = motion[x].velocity;
      v[n + 2].normal = motion[x].acceleration;
      v[n + 2].u = 0;
      v[n + 2].v = 1;
      v[n + 3].position = motion[x].velocity;
      v[n + 3].normal = motion[x].acceleration;
      v[n + 3].u = 1;
      v[n + 3].v = 1;
    }
    mg_->Raw()->CreateVertexBuffer(&vb, FtxFormat::format, num_ * 4);
    vb->WriteVertex(v.data());

    return true;
  }
  void Draw(const Vector3 &position, const Color &color, float count, float size)
  {
    mg_->particle_->SetCount(count);
    mg_->particle_->SetSize(size);
    mg_->particle_->SetCamera(mg_->camera_);
    mg_->set_rgba_->Set(color);
    mg_->particle_->SetPosition(position);

    mg_->Raw()->SetRenderState(RENDER_ZWRITE, VALUE_DISABLE);
    //mg_->Raw()->SetAlphaBlendMode(ALPHABLEND_MODULATE);
    mg_->Raw()->SetAlphaBlendMode(ALPHABLEND_ADD);

    int slot = 0;
    mg_->Raw()->SetShader(mg_->particle_, slot++);
    mg_->Raw()->SetShader(mg_->set_rgba_, slot++);
    if (texture_)
    {
      mg_->decal_texture_->SetTexture(texture_);
      mg_->Raw()->SetShader(mg_->decal_texture_, slot++);
    }
    mg_->Raw()->SetShader(shader::ShaderEnd(), slot++);

    mg_->Raw()->DrawIndexedPrimitive(vb, ib, 0, num_ * 2);
    mg_->Raw()->SetAlphaBlendMode(ALPHABLEND_NONE);
    mg_->Raw()->SetRenderState(RENDER_ZWRITE, VALUE_ENABLE);
  }
private:
  int num_;
  ManagedGraphics *mg_;
  VertexBuffer *vb;
  IndexBuffer *ib;
  Texture *texture_;
};

class PointicleImpl
  :public Pointicle
{
  typedef Fertex<
    FTX_POSITION |
    FTX_DIFFUSE |
    FTX_TEX1> FtxFormat;

public:
  PointicleImpl(ManagedGraphics *mg)
    :mg_(mg)
    , vb(NULL)
    , ib(NULL)
  {

  }
  ~PointicleImpl()
  {
    if (vb)
      vb->Release();
    if (ib)
      ib->Release();
  }
  bool Init(std::vector<Chip > motion)
  {
    if (vb)
      vb->Release();
    if (ib)
      ib->Release();

    num_ = motion.size();

    std::vector<IndexType> idx;

    idx.resize(num_ * 6);
    IndexType *i = &idx.front();
    for (int x = 0; x < num_; x++)
    {
      *i++ = 0xffff & (x * 4 + 0);
      *i++ = 0xffff & (x * 4 + 2);
      *i++ = 0xffff & (x * 4 + 1);

      *i++ = 0xffff & (x * 4 + 1);
      *i++ = 0xffff & (x * 4 + 2);
      *i++ = 0xffff & (x * 4 + 3);
    }

    mg_->Raw()->CreateIndexBuffer(&ib, num_ * 6);
    ib->WriteIndex(idx.data());

    std::vector<FtxFormat> v;
    v.resize(num_ * 4);

    for (int x = 0; x < num_; x++)
    {
      const int n = x * 4;
      v[n + 0].position = motion[x].velocity;
      v[n + 0].diffuse = motion[x].color;
      v[n + 0].u = 1;
      v[n + 0].v = 1;
      v[n + 1].position = motion[x].velocity;
      v[n + 1].diffuse = motion[x].color;
      v[n + 1].u = -1;
      v[n + 1].v = 1;
      v[n + 2].position = motion[x].velocity;
      v[n + 2].diffuse = motion[x].color;
      v[n + 2].u = 1;
      v[n + 2].v = -1;
      v[n + 3].position = motion[x].velocity;
      v[n + 3].diffuse = motion[x].color;
      v[n + 3].u = -1;
      v[n + 3].v = -1;
    }
    mg_->Raw()->CreateVertexBuffer(&vb, FtxFormat::format, num_ * 4);
    vb->WriteVertex(v.data());

    num_ *= 2;
    return true;
  }
  bool InitTri(std::vector<Chip > motion)
  {
    if (vb)
      vb->Release();
    if (ib)
      ib->Release();

    num_ = motion.size();

    std::vector<IndexType> idx;

    idx.resize(num_ * 3);
    IndexType *i = &idx.front();
    for (int x = 0; x < num_; x++)
    {
      *i++ = 0xffff & (x * 3 + 0);
      *i++ = 0xffff & (x * 3 + 1);
      *i++ = 0xffff & (x * 3 + 2);
    }

    mg_->Raw()->CreateIndexBuffer(&ib, num_ * 3);
    ib->WriteIndex(idx.data());

    std::vector<FtxFormat> v;
    v.resize(num_ * 3);

    for (int x = 0; x < num_; x++)
    {
      const int n = x * 3;
      v[n + 0].position = motion[x].velocity;
      v[n + 0].diffuse = motion[x].color;
      v[n + 0].u = 0;
      v[n + 0].v = -1.7f;
      v[n + 1].position = motion[x].velocity;
      v[n + 1].diffuse = motion[x].color;
      v[n + 1].u = -1.472f;
      v[n + 1].v = 0.85;
      v[n + 2].position = motion[x].velocity;
      v[n + 2].diffuse = motion[x].color;
      v[n + 2].u = 1.472f;
      v[n + 2].v = 0.85;
    }
    mg_->Raw()->CreateVertexBuffer(&vb, FtxFormat::format, num_ * 3);
    vb->WriteVertex(v.data());

    return true;
  }
  void Draw(const Vector3 &position, float alpha, float count, float size)
  {
    mg_->pointicle_->SetAlpha(alpha);
    mg_->pointicle_->SetCount(count);
    mg_->pointicle_->SetSize(size);
    mg_->pointicle_->SetCamera(mg_->camera_);
    mg_->pointicle_->SetPosition(position);

    mg_->Raw()->SetRenderState(RENDER_ZWRITE, VALUE_DISABLE);
    mg_->Raw()->SetAlphaBlendMode(ALPHABLEND_ADD);

    int slot = 0;
    mg_->Raw()->SetShader(mg_->pointicle_, slot++);
    mg_->Raw()->SetShader(shader::ShaderEnd(), slot++);

    mg_->Raw()->DrawIndexedPrimitive(vb, ib, 0, num_);
    mg_->Raw()->SetAlphaBlendMode(ALPHABLEND_NONE);
    mg_->Raw()->SetRenderState(RENDER_ZWRITE, VALUE_ENABLE);
  }
private:
  int num_;
  ManagedGraphics *mg_;
  VertexBuffer *vb;
  IndexBuffer *ib;
};

class LinecleImpl
  :public Linecle
{
  typedef Fertex<
    FTX_POSITION |
    FTX_DIFFUSE |
    FTX_NORMAL |
    FTX_TANGENT |
    FTX_TEX1> FtxFormat;

public:
  LinecleImpl(ManagedGraphics *mg)
    :mg_(mg)
    , vb(NULL)
    , ib(NULL)
  {

  }
  ~LinecleImpl()
  {
    if (vb)
      vb->Release();
    if (ib)
      ib->Release();
  }
  bool Init(std::vector<Chip > motion)
  {
    if (vb)
      vb->Release();
    if (ib)
      ib->Release();

    num_ = motion.size();

    std::vector<IndexType> idx;

    idx.resize(num_ * 18);
    IndexType *i = &idx.front();
    for (int x = 0; x < num_; x++)
    {
      *i++ = 0xffff & (x * 8 + 0);
      *i++ = 0xffff & (x * 8 + 2);
      *i++ = 0xffff & (x * 8 + 1);

      *i++ = 0xffff & (x * 8 + 1);
      *i++ = 0xffff & (x * 8 + 2);
      *i++ = 0xffff & (x * 8 + 3);

      *i++ = 0xffff & (x * 8 + 2);
      *i++ = 0xffff & (x * 8 + 3);
      *i++ = 0xffff & (x * 8 + 4);

      *i++ = 0xffff & (x * 8 + 4);
      *i++ = 0xffff & (x * 8 + 3);
      *i++ = 0xffff & (x * 8 + 5);

      *i++ = 0xffff & (x * 8 + 4);
      *i++ = 0xffff & (x * 8 + 5);
      *i++ = 0xffff & (x * 8 + 6);

      *i++ = 0xffff & (x * 8 + 6);
      *i++ = 0xffff & (x * 8 + 5);
      *i++ = 0xffff & (x * 8 + 7);

    }

    mg_->Raw()->CreateIndexBuffer(&ib, num_ * 18);
    ib->WriteIndex(idx.data());

    std::vector<FtxFormat> v;
    v.resize(num_ * 8);
    FtxFormat *n = v.data();

    for (int x = 0; x < num_; x++)
    {
      Vector3 r = motion[x].velocity;
      Vector3 c = Cross(mg_->camera_->Direction(), r).Normalize();
      r.Normalize();
      n->position = motion[x].velocity;
      n->diffuse = motion[x].color;
      n->normal = r + c;
      n->tangent = r;
      n->u = 1;
      n->v = 1;
      n++;
      n->position = motion[x].velocity;
      n->diffuse = motion[x].color;
      n->normal = r - c;
      n->tangent = r;
      n->u = -1;
      n->v = 1;
      n++;
      n->position = motion[x].velocity;
      n->diffuse = motion[x].color;
      n->normal = c;
      n->tangent = r;
      n->u = 1;
      n->v = 0;
      n++;
      n->position = motion[x].velocity;
      n->diffuse = motion[x].color;
      n->normal = -c;
      n->tangent = r;
      n->u = -1;
      n->v = 0;
      n++;
      n->position = motion[x].velocity;
      n->diffuse = motion[x].color;
      n->normal = c;
      n->tangent = Vec3(0, 0, 0);
      n->u = 1;
      n->v = 0;
      n++;
      n->position = motion[x].velocity;
      n->diffuse = motion[x].color;
      n->normal = -c;
      n->tangent = Vec3(0, 0, 0);
      n->u = -1;
      n->v = 0;
      n++;
      n->position = motion[x].velocity;
      n->diffuse = motion[x].color;
      n->normal = -r + c;
      n->tangent = Vec3(0, 0, 0);
      n->u = 1;
      n->v = -1;
      n++;
      n->position = motion[x].velocity;
      n->diffuse = motion[x].color;
      n->normal = -r - c;
      n->tangent = Vec3(0, 0, 0);
      n->u = -1;
      n->v = -1;
      n++;
    }
    mg_->Raw()->CreateVertexBuffer(&vb, FtxFormat::format, num_ * 8);
    vb->WriteVertex(v.data());

    num_ *= 2;
    return true;
  }
  void Draw(const Vector3 &position, float alpha, float count, float length, float size)
  {
    mg_->pointicle_->SetAlpha(alpha);
    mg_->pointicle_->SetCount(count, length);
    mg_->pointicle_->SetSize(size);
    mg_->pointicle_->SetCamera(mg_->camera_);
    mg_->pointicle_->SetPosition(position);

    mg_->Raw()->SetRenderState(RENDER_ZWRITE, VALUE_DISABLE);
    mg_->Raw()->SetRenderState(RENDER_CULLMODE, CULL_NONE);
    mg_->Raw()->SetAlphaBlendMode(ALPHABLEND_ADD);

    int slot = 0;
    mg_->Raw()->SetShader(mg_->pointicle_, slot++);
    mg_->Raw()->SetShader(shader::ShaderEnd(), slot++);

    mg_->Raw()->DrawIndexedPrimitive(vb, ib, 0, num_);
    mg_->Raw()->SetAlphaBlendMode(ALPHABLEND_NONE);
    mg_->Raw()->SetRenderState(RENDER_ZWRITE, VALUE_ENABLE);
    mg_->Raw()->SetRenderState(RENDER_CULLMODE, CULL_CCW);
  }
private:
  int num_;
  ManagedGraphics *mg_;
  VertexBuffer *vb;
  IndexBuffer *ib;
};

class LoopJetImpl
  :public LoopJet
{
  typedef Fertex<
    FTX_POSITION |
    FTX_DIFFUSE |
    FTX_TEX1> FtxFormat;

public:
  LoopJetImpl(ManagedGraphics *mg)
    :mg_(mg)
    , vb(NULL)
    , ib(NULL)
  {

  }
  ~LoopJetImpl()
  {
    if (vb)
      vb->Release();
    if (ib)
      ib->Release();
  }
  bool Init(std::vector<Chip > motion)
  {
    if (vb)
      vb->Release();
    if (ib)
      ib->Release();

    num_ = motion.size();

    std::vector<IndexType> idx;

    idx.resize(num_ * 6);
    IndexType *i = &idx.front();
    for (int x = 0; x < num_; x++)
    {
      *i++ = 0xffff & (x * 4 + 0);
      *i++ = 0xffff & (x * 4 + 2);
      *i++ = 0xffff & (x * 4 + 1);

      *i++ = 0xffff & (x * 4 + 1);
      *i++ = 0xffff & (x * 4 + 2);
      *i++ = 0xffff & (x * 4 + 3);
    }

    mg_->Raw()->CreateIndexBuffer(&ib, num_ * 6);
    ib->WriteIndex(idx.data());

    std::vector<FtxFormat> v;
    v.resize(num_ * 4);

    for (int x = 0; x < num_; x++)
    {
      const int n = x * 4;
      v[n + 0].position = motion[x].velocity;
      v[n + 0].diffuse = motion[x].color;
      v[n + 0].u = 1;
      v[n + 0].v = 1;
      v[n + 1].position = motion[x].velocity;
      v[n + 1].diffuse = motion[x].color;
      v[n + 1].u = -1;
      v[n + 1].v = 1;
      v[n + 2].position = motion[x].velocity;
      v[n + 2].diffuse = motion[x].color;
      v[n + 2].u = 1;
      v[n + 2].v = -1;
      v[n + 3].position = motion[x].velocity;
      v[n + 3].diffuse = motion[x].color;
      v[n + 3].u = -1;
      v[n + 3].v = -1;
    }
    mg_->Raw()->CreateVertexBuffer(&vb, FtxFormat::format, num_ * 4);
    vb->WriteVertex(v.data());

    num_ *= 2;
    return true;
  }
  bool InitTri(std::vector<Chip > motion)
  {
    if (vb)
      vb->Release();
    if (ib)
      ib->Release();

    num_ = motion.size();

    std::vector<IndexType> idx;

    idx.resize(num_ * 3);
    IndexType *i = &idx.front();
    for (int x = 0; x < num_; x++)
    {
      *i++ = 0xffff & (x * 3 + 0);
      *i++ = 0xffff & (x * 3 + 1);
      *i++ = 0xffff & (x * 3 + 2);
    }

    mg_->Raw()->CreateIndexBuffer(&ib, num_ * 3);
    ib->WriteIndex(idx.data());

    std::vector<FtxFormat> v;
    v.resize(num_ * 3);

    for (int x = 0; x < num_; x++)
    {
      const int n = x * 3;
      v[n + 0].position = motion[x].velocity;
      v[n + 0].diffuse = motion[x].color;
      v[n + 0].u = 0;
      v[n + 0].v = -1.7f;
      v[n + 1].position = motion[x].velocity;
      v[n + 1].diffuse = motion[x].color;
      v[n + 1].u = -1.472f;
      v[n + 1].v = 0.85;
      v[n + 2].position = motion[x].velocity;
      v[n + 2].diffuse = motion[x].color;
      v[n + 2].u = 1.472f;
      v[n + 2].v = 0.85;
    }
    mg_->Raw()->CreateVertexBuffer(&vb, FtxFormat::format, num_ * 3);
    vb->WriteVertex(v.data());

    return true;
  }
  void Draw(const Matrix &pose, float count, float size)
  {
    mg_->loop_jet_->SetCount(count);
    mg_->world_transform_->Transform(pose);
    mg_->billboard_->SetSize(size);

    mg_->Raw()->SetRenderState(RENDER_ZWRITE, VALUE_DISABLE);
    mg_->Raw()->SetAlphaBlendMode(ALPHABLEND_ADD);


    int slot = 0;
    mg_->Raw()->SetShader(mg_->loop_jet_, slot++);
    mg_->Raw()->SetShader(mg_->world_transform_, slot++);
    mg_->Raw()->SetShader(mg_->camera_, slot++);
    mg_->Raw()->SetShader(mg_->billboard_, slot++);
    mg_->Raw()->SetShader(shader::ShaderEnd(), slot++);

    mg_->Raw()->DrawIndexedPrimitive(vb, ib, 0, num_);
    mg_->Raw()->SetAlphaBlendMode(ALPHABLEND_NONE);
    mg_->Raw()->SetRenderState(RENDER_ZWRITE, VALUE_ENABLE);
  }
private:
  int num_;
  ManagedGraphics *mg_;
  VertexBuffer *vb;
  IndexBuffer *ib;
};

void ManagedGraphics::CreateLoopJet(LoopJet **obj)
{
  *obj = new LoopJetImpl(this);
}


bool gh::CreateManagedGraphics(ManagedGraphics **mg)
{
  *mg = new ManagedGraphics();
  return true;
}

ManagedGraphics::ManagedGraphics()
  :raw_(0)
  , camera_(0)
  , directional_light_(0)
  , specular_light_(0)
  , ambient_light_(0)
  , world_transform_(0)
  , blend_transform_(0)
  , diffuse_light_(0)
  , decal_texture_(0)
  , normal_map_(0)
  , particle_(0)
  , set_rgba_(0)
  , pointicle_(0)
  , center_specular_(0)
  , edge_specular_(0)
  , blow_normal_(0)
  , set_angle_rgba_(0)
{
}

bool ManagedGraphics::Initialize(Graphics *raw)
{
  if (raw_)
    return false;
  this->raw_ = raw;

  diffuse_light_ = raw_->CreateShader<shader::ClassID_DiffuseLight>();
  decal_texture_ = raw_->CreateShader<shader::ClassID_DecalTexture>();
  world_transform_ = raw_->CreateShader<shader::ClassID_WorldTransform>();
  camera_ = raw_->CreateShader<shader::ClassID_Camera>();
  blend_transform_ = raw_->CreateShader<shader::ClassID_BlendTransform>();
  huge_blend_transform_ = raw_->CreateShader<shader::ClassID_HugeBlendTransform>();
  directional_light_ = raw_->CreateShader<shader::ClassID_DirectionalLight>();
  ambient_light_ = raw_->CreateShader<shader::ClassID_AmbientLight>();
  specular_light_ = raw_->CreateShader<shader::ClassID_SpecularLight>();
  particle_ = raw_->CreateShader<shader::ClassID_Particle>();
  set_rgba_ = raw_->CreateShader<shader::ClassID_SetRGBA>();
  pointicle_ = raw_->CreateShader<shader::ClassID_Pointicle>();
  center_specular_ = raw_->CreateShader<shader::ClassID_CenterSpecular>();
  edge_specular_ = raw_->CreateShader<shader::ClassID_EdgeSpecular>();
  blow_normal_ = raw_->CreateShader<shader::ClassID_BlowNormal>();
  set_angle_rgba_ = raw_->CreateShader<shader::ClassID_SetAngleRGBA>();
  normal_map_ = raw_->CreateShader<shader::ClassID_NormalMap>();
  tangent_map_ = raw_->CreateShader<shader::ClassID_TangentMap>();
  colored_normalmap_ = raw->CreateShader<shader::ClassID_ColoredNormalMap>();
  loop_jet_ = raw->CreateShader<shader::ClassID_LoopJet>();
  billboard_ = raw_->CreateShader<shader::ClassID_Billboard>();
  ssdiffuse_ = raw_->CreateShader<shader::ClassID_ScreenSpaceDiffuse>();
  gaussian_filter_ = raw_->CreateShader<shader::ClassID_GaussianFilter>();

  camera_->LookAt(Vec3(0, 0, 200), Vec3(0, 0, 0), Vec3(0, 1, 0));
  camera_->PerspectiveFov(0.1f, 1000.0f, Deg(30), 16 / 9.0f);
  billboard_->SetAspect(9.0f / 16.0f);
  Vector3 light_dir = Vec3(-1, 1, -1);
  light_dir.Normalize();
  directional_light_->Direction(light_dir);
  directional_light_->Diffuse(Color(0xffffffff));
  directional_light_enable_ = false;
  specular_light_->Power(5);
  specular_light_->Specular(0xffffffff);
  ambient_light_->Ambient(Color(0xff808080));
  return true;
}

bool ManagedGraphics::Finalize()
{
  for (auto &i : textures_) {
    i.second->Release();
  }

  while (model_map_.size())
  {
    DeleteModel(model_map_.begin()->first);
  }

  colored_normalmap_&&raw_->DeleteShader(colored_normalmap_);
  set_angle_rgba_&&raw_->DeleteShader(set_angle_rgba_);
  blow_normal_&&raw_->DeleteShader(blow_normal_);
  edge_specular_&&raw_->DeleteShader(edge_specular_);
  center_specular_&&raw_->DeleteShader(center_specular_);
  pointicle_&&raw_->DeleteShader(pointicle_);
  particle_&&raw_->DeleteShader(particle_);
  decal_texture_&&raw_->DeleteShader(decal_texture_);
  normal_map_&&raw_->DeleteShader(normal_map_);
  diffuse_light_&&raw_->DeleteShader(diffuse_light_);
  blend_transform_&&raw_->DeleteShader(blend_transform_);
  world_transform_&&raw_->DeleteShader(world_transform_);
  ambient_light_&&raw_->DeleteShader(ambient_light_);
  specular_light_&&raw_->DeleteShader(specular_light_);
  directional_light_&&raw_->DeleteShader(directional_light_);
  loop_jet_&&raw_->DeleteShader(loop_jet_);
  camera_&&raw_->DeleteShader(camera_);

  return true;
}

void ManagedGraphics::BuildPreset()
{
  GetModel("");
}

Texture* ManagedGraphics::CreateColorTexture(int width, int height, const Color *bitmap)
{
  int offs[TEXTURE_FORMAT_END][4];
  offs[TEXTURE_ABGR8][0] = 0;
  offs[TEXTURE_ABGR8][1] = 1;
  offs[TEXTURE_ABGR8][2] = 2;
  offs[TEXTURE_ABGR8][3] = 3;
  offs[TEXTURE_ARGB8][0] = 2;
  offs[TEXTURE_ARGB8][1] = 1;
  offs[TEXTURE_ARGB8][2] = 0;
  offs[TEXTURE_ARGB8][3] = 3;
  Texture *tex;
  raw_->CreateTexture(&tex, width, height, TEXTURE_ABGR8) ||
    raw_->CreateTexture(&tex, width, height, TEXTURE_ARGB8);
  LockInfo li;
  tex->Lock(li, LOCK_WRITE);
  TextureDescription td = tex->Desc();
  for (int y = 0; y < height; y++)
  {
    for (int x = 0; x < width; x++)
    {
      unsigned char *dst = (li.Bits + li.Pitch*y) + x * 4;
      dst[offs[td.format][0]] = (int)(bitmap[y*width + x].r * 255);
      dst[offs[td.format][1]] = (int)(bitmap[y*width + x].g * 255);
      dst[offs[td.format][2]] = (int)(bitmap[y*width + x].b * 255);
      dst[offs[td.format][3]] = (int)(bitmap[y*width + x].a * 255);
    }
  }
  tex->Unlock();
  return tex;
}

void ManagedGraphics::AppendTexture(const char *path, Texture *t)
{
  textures_[path] = t;
}


Texture* ManagedGraphics::GetTexture(const char *path)
{
  if (_.has(textures_, (std::string)path))
  {
    return textures_[path];
  }
  else
  {
    if (Fs::Complete == fs->Status(path))
    {
      gh::Raw raw;
      raw.data = (uint8_t*)fs->Data(path);
      raw.size = fs->Length(path);

      Png png;
      if (ReadFromRaw(&png, &raw))
      {
        if (8 == png.image_header.color_depth &&
          COLOR_TYPE_RGB == png.image_header.color_type)
        {
          Texture *tex;
          LockInfo li;
          if (raw_->CreateTexture(&tex, png.image_header.width, png.image_header.height, TEXTURE_XBGR8))
          {
            tex->Lock(li, LOCK_WRITE);
            for (uint32_t y = 0; y < png.image_header.height; y++)
            {
              for (uint32_t x = 0; x < png.image_header.width; x++)
              {
                unsigned char *src = png.image_data + (y*png.image_header.width + x) * 3;
                unsigned char *dst = (li.Bits + li.Pitch*y) + x * 4;
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
                dst[3] = 0xff;
              }
            }
            tex->Unlock();
          }
          else if (raw_->CreateTexture(&tex, png.image_header.width, png.image_header.height, TEXTURE_XRGB8))
          {
            tex->Lock(li, LOCK_WRITE);
            for (uint32_t y = 0; y < png.image_header.height; y++)
            {
              for (uint32_t x = 0; x < png.image_header.width; x++)
              {
                unsigned char *src = png.image_data + (y*png.image_header.width + x) * 3;
                unsigned char *dst = (li.Bits + li.Pitch*y) + x * 4;
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
                dst[3] = 0xff;
              }
            }
            tex->Unlock();
          }

          textures_[path] = tex;
        }
        else if (8 == png.image_header.color_depth &&
          COLOR_TYPE_RGB_ALPHA == png.image_header.color_type)
        {
          Texture *tex;
          LockInfo li;
          if (raw_->CreateTexture(&tex, png.image_header.width, png.image_header.height, TEXTURE_ABGR8))
          {
            tex->Lock(li, LOCK_WRITE);
            for (uint32_t y = 0; y < png.image_header.height; y++)
            {
              for (uint32_t x = 0; x < png.image_header.width; x++)
              {
                unsigned char *src = png.image_data + (y*png.image_header.width + x) * 4;
                unsigned char *dst = (li.Bits + li.Pitch*y) + x * 4;
                dst[0] = src[3];
                dst[1] = src[2];
                dst[2] = src[1];
                dst[3] = src[0];
              }
            }
            tex->Unlock();
          }
          else if (raw_->CreateTexture(&tex, png.image_header.width, png.image_header.height, TEXTURE_ARGB8))
          {
            tex->Lock(li, LOCK_WRITE);
            for (uint32_t y = 0; y < png.image_header.height; y++)
            {
              for (uint32_t x = 0; x < png.image_header.width; x++)
              {
                unsigned char *src = png.image_data + (y*png.image_header.width + x) * 4;
                unsigned char *dst = (li.Bits + li.Pitch*y) + x * 4;
                dst[0] = src[1];
                dst[1] = src[2];
                dst[2] = src[3];
                dst[3] = src[0];
              }
            }
            tex->Unlock();
          }

          textures_[path] = tex;
        }
        else if (8 == png.image_header.color_depth &&
          COLOR_TYPE_GRAY_ALPHA == png.image_header.color_type)
        {
          Texture *tex;
          LockInfo li;
          if (raw_->CreateTexture(&tex, png.image_header.width, png.image_header.height, TEXTURE_ABGR8))
          {
            tex->Lock(li, LOCK_WRITE);
            for (uint32_t y = 0; y < png.image_header.height; y++)
            {
              for (uint32_t x = 0; x < png.image_header.width; x++)
              {
                unsigned char *src = png.image_data + (y*png.image_header.width + x) * 4;
                unsigned char *dst = (li.Bits + li.Pitch*y) + x * 4;
                dst[0] = src[1];
                dst[1] = src[1];
                dst[2] = src[1];
                dst[3] = src[0];
              }
            }
            tex->Unlock();
          }
          else if (raw_->CreateTexture(&tex, png.image_header.width, png.image_header.height, TEXTURE_ARGB8))
          {
            tex->Lock(li, LOCK_WRITE);
            for (uint32_t y = 0; y < png.image_header.height; y++)
            {
              for (uint32_t x = 0; x < png.image_header.width; x++)
              {
                unsigned char *src = png.image_data + (y*png.image_header.width + x) * 4;
                unsigned char *dst = (li.Bits + li.Pitch*y) + x * 4;
                dst[0] = src[1];
                dst[1] = src[1];
                dst[2] = src[1];
                dst[3] = src[0];
              }
            }
            tex->Unlock();
          }

          textures_[path] = tex;
        }
        else if (8 == png.image_header.color_depth &&
          COLOR_TYPE_INDEX == png.image_header.color_type)
        {
          Texture *tex;
          LockInfo li;
          if (raw_->CreateTexture(&tex, png.image_header.width, png.image_header.height, TEXTURE_ABGR8))
          {
            tex->Lock(li, LOCK_WRITE);
            for (uint32_t y = 0; y < png.image_header.height; y++)
            {
              for (uint32_t x = 0; x < png.image_header.width; x++)
              {
                unsigned char *src = png.image_data + (y*png.image_header.width + x);
                unsigned char *dst = (li.Bits + li.Pitch*y) + x * 4;
                dst[0] = png.image_header.pltd[src[0]][0];
                dst[1] = png.image_header.pltd[src[0]][1];
                dst[2] = png.image_header.pltd[src[0]][2];
                dst[3] = png.image_header.trns[src[0]];
              }
            }
            tex->Unlock();
          }
          else if (raw_->CreateTexture(&tex, png.image_header.width, png.image_header.height, TEXTURE_ARGB8))
          {
            tex->Lock(li, LOCK_WRITE);
            for (uint32_t y = 0; y < png.image_header.height; y++)
            {
              for (uint32_t x = 0; x < png.image_header.width; x++)
              {
                unsigned char *src = png.image_data + (y*png.image_header.width + x);
                unsigned char *dst = (li.Bits + li.Pitch*y) + x * 4;
                dst[0] = png.image_header.pltd[src[0]][2];
                dst[1] = png.image_header.pltd[src[0]][1];
                dst[2] = png.image_header.pltd[src[0]][0];
                dst[3] = png.image_header.trns[src[0]];
              }
            }
            tex->Unlock();
          }

          textures_[path] = tex;
        }
        else
        {
          return 0;
        }


        FreePng(&png);
      }
      else
      {
        return 0;
      }
      return textures_[path];
    }
  }
  return 0;
}

void ManagedGraphics::DrawSprite(const Matrix &mat, Texture *tex, Frect fr, uint32_t color)
{
  int slot = 0;
  world_transform_->Transform(mat);
  raw_->SetShader(world_transform_, slot++);
  if (color) {
    set_rgba_->Set(color);
    raw_->SetShader(set_rgba_, slot++);
  }
  raw_->SetAlphaBlendMode(ALPHABLEND_MODULATE);
  raw_->SetRenderState(RENDER_CULLMODE, CULL_NONE);
  if (tex) {
    const float w = tex->Desc().width;
    const float h = tex->Desc().height;
    if (fr.x1 == 0 && fr.x2 == 0 && fr.y1 == 0 && fr.y2 == 0)fr.x2 = w - 1, fr.y2 = h - 1;
    Fertex<FTX_POSITION | FTX_TEX1> v[4];
    v[0].position = Vec3(-0.5f, -0.5f, 0.0f);
    v[0].u = (fr.x1 + 0.5f) / w;
    v[0].v = (fr.y2 + 0.5f) / h;
    v[1].position = Vec3(-0.5f, 0.5f, 0.0f);
    v[1].u = (fr.x1 + 0.5f) / w;
    v[1].v = (fr.y1 + 0.5f) / h;
    v[2].position = Vec3(0.5f, -0.5f, 0.0f);
    v[2].u = (fr.x2 + 0.5f) / w;
    v[2].v = (fr.y2 + 0.5f) / h;
    v[3].position = Vec3(0.5f, 0.5f, 0.0f);
    v[3].u = (fr.x2 + 0.5f) / w;
    v[3].v = (fr.y1 + 0.5f) / h;

    decal_texture_->SetTexture(tex);
    raw_->SetShader(decal_texture_, slot++);

    raw_->SetShader(shader::ShaderEnd(), slot);
    raw_->DrawPrimitiveUP(PRIMITIVE_TRIANGLESTRIP, 2, v, v[0].format);
  }
  else {
    Fertex<FTX_POSITION> v[4];
    v[0].position = Vec3(-0.5f, -0.5f, 0.0f);
    v[1].position = Vec3(-0.5f, 0.5f, 0.0f);
    v[2].position = Vec3(0.5f, -0.5f, 0.0f);
    v[3].position = Vec3(0.5f, 0.5f, 0.0f);

    raw_->SetShader(shader::ShaderEnd(), slot);
    raw_->DrawPrimitiveUP(PRIMITIVE_TRIANGLESTRIP, 2, v, v[0].format);
  }
  raw_->SetRenderState(RENDER_CULLMODE, CULL_CCW);
  raw_->SetAlphaBlendMode(ALPHABLEND_NONE);
}

void ManagedGraphics::DrawAspectedSprite(Centering c, const Matrix &mat, Texture *tex, Frect fr, uint32_t color)
{
  const float aspect = (float)raw_->Height() / raw_->Width();
  static const float layout[9][2] = {
    { -1, 1 },
    { 0, 1 },
    { 1, 1 },
    { -1, 0 },
    { 0, 0 },
    { 1, 0 },
    { -1, -1 },
    { 0, -1 },
    { 1, -1 } };
  Matrix m;
  m.Identity();
  m.Scale(aspect, 1, 1);
  m.Translate(layout[c][0], layout[c][1], 0);
  m = mat * m;
  DrawSprite(m, tex, fr, color);
}

void ManagedGraphics::DrawGaussian(Texture * from, Texture *to)
{
  raw_->SetRenderTarget(to);
  int slot = 0;
  Fertex<FTX_POSITION | FTX_TEX1> v[6];
  v[0].position = Vec3(-1.0f, -1.0f, 0.0f);
  v[0].u = 0;
  v[1].position = Vec3(-1.0f, 1.0f, 0.0f);
  v[1].u = 0;
  v[2].position = Vec3(1.0f, -1.0f, 0.0f);
  v[2].u = 1;
  v[3].position = Vec3(1.0f, 1.0f, 0.0f);
  v[3].u = 1;
  if (to->isInverse()) {
    v[0].v = 0;
    v[1].v = 1;
    v[2].v = 0;
    v[3].v = 1;
  }
  else {
    v[0].v = 1;
    v[1].v = 0;
    v[2].v = 1;
    v[3].v = 0;
  }

  raw_->BeginScene();

  gaussian_filter_->SetTexture(from);
  raw_->SetShader(gaussian_filter_, slot++);

  raw_->SetShader(shader::ShaderEnd(), slot);
  raw_->SetRenderState(RENDER_Z, false);
  raw_->DrawPrimitiveUP(PRIMITIVE_TRIANGLESTRIP, 2, v, v[0].format);
  raw_->SetRenderState(RENDER_Z, true);
  raw_->EndScene();
}


Bone* ManagedGraphics::GetBone(const std::string &name)
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
    //else if (name.find(".pmd") == name.size() - 4)
    //{
    //  if (fs->Status(name) == Fs::Complete)
    //  {
    //    ParsePMD((char*)fs->Data(name), fs->Length(name), bone_map_[name]);
    //    return &bone_map_[name];
    //  }
    //  else
    //    return 0;
    //}
    //else if (name.find(".pmx") == name.size() - 4)
    //{
    //  if (fs->Status(name) == Fs::Complete)
    //  {
    //    ParsePMX((char*)fs->Data(name), fs->Length(name), bone_map_[name]);
    //    return &bone_map_[name];
    //  }
    //  else
    //    return 0;
    //}
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

Animation* ManagedGraphics::GetAnimation(const std::string &name)
{
  if (_.has(animation_map_, name))
  {
    return &animation_map_[name];
  }
  else
  {
    if (name.find(".vmd") == name.size() - 4)
    {
      if (fs->Status(name) == Fs::Complete)
      {
        ParseVMD((char*)fs->Data(name), fs->Length(name), animation_map_[name]);
        return &animation_map_[name];
      }
      else
        return 0;
    }
    else
      return 0;
    //if(name.Find(".x")==name.size()-2)
    //{
    //	File *f;
    //	f=Fetch(name.data());
    //	if(f->State()==File::Complete)
    //	{
    //		AnimationSet as;
    //		ParseX((char*)f->data(),f->Length(),bones[name],as);
    //                    anims[name]=as.Root()->content.value;
    //		return &bones[name];
    //	}
    //	else
    //		return 0;
    //}
    //else
    //	return 0;

  }
}

void ManagedGraphics::CreatePrimitiveFromSubset(Primitive *primitive, const Shape &shape, int subset_number)
{
  const Subset &subset = shape.subset[subset_number];
  int offset = 0;//head of subset as index
  for (int i = 0; i < subset_number; i++)
    offset += shape.subset[i].indices.size();


  int format = 0;
  if (shape.position.size())
    format |= FTX_POSITION;
  if (shape.normal.size())
    format |= FTX_NORMAL;
  if (subset.material.texture.size())
    format |= FTX_TEX1;

  std::map<IndexType, ReferencedCounter> referenced_count;
  for (int i = 0; i < subset.indices.size(); i++)
  {
    referenced_count[subset.indices[i].position].position++;
    referenced_count[subset.indices[i].normal].normal++;
    if (subset.material.texture.size())
      referenced_count[subset.indices[i].texture].tex1++;
  }

  std::map<IndexType, BlendIndex> wic;
  for (int j = 0; j < shape.weight.size(); j++)
  {
    const Weight &weight = shape.weight[j];
    for (int k = 0; k < weight.position.size(); k++)
    {
      if (referenced_count.end() == referenced_count.find(weight.position[k].index))
        continue;
      if (weight.position[k].weight > 0)
      {
        WeightIndex &wi = _.push(wic[weight.position[k].index]);
        wi.index = static_cast<IndexType>(j);
        wi.weight = weight.position[k].weight;
      }
    }
  }
  int maxblends = 0;
  for (auto &i : wic) {
    if (i.second.size() > maxblends)
      maxblends = i.second.size();
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
  for (auto &i : bind_weight) {
    i = 0;
  }
  for (auto &n : wic) {
    for (int i = 0; i < n.second.size(); i++) {
      if (n.second[i].weight > 0) {
        bind_weight[n.second[i].index]++;
      }
    }
  }

  std::vector<int> shrinked_bone_index;
  int used_bone = 0;
  for (int i = 0; i < bind_weight.size(); i++)
  {
    //printf("%d- %d\n",i,bind_weight[i]);
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
  primitive->boneindex.resize(used_bone);
  for (int j = 0, i = 0; i < bind_weight.size(); i++)
  {
    if (bind_weight[i] > 0)
    {
      primitive->boneindex[j].index = shape.weight[i].bonename;
      primitive->boneindex[j].offset = shape.weight[i].offset;
      j++;
    }
  }

  //printf("subset %d used_bones %d\n",subset_number,used_bone);

  //if(used_bone==1)
  //    format&=~FTX_WEIGHTMASK;

  std::vector<char> &vtx = primitive->raw_vertex;
  std::vector<IndexType> idx;
  int vtxex = 0;
  std::vector<PntRefs> iviv;
  for (int k = 0; k < subset.indices.size(); k++)
  {
    const IndexVertex &iv = subset.indices[k];
    PntRefs ref;

    ref.position =
      (format&FTX_POSITION) ? iv.position : 0;

    ref.normal =
      (format&FTX_NORMAL) ? iv.normal : 0;

    ref.tex1 =
      (format&FTX_TEX1) ? iv.texture : 0;

    bool hit = false;
    for (IndexType index = 0; index < iviv.size(); index++)
    {
      if (((format&FTX_POSITION) ? ref.position == iviv[index].position : true)
        && ((format&FTX_NORMAL) ? ref.normal == iviv[index].normal : true)
        && ((format&FTX_TEX1) ? ref.tex1 == iviv[index].tex1 : true)
        )
      {
        hit = true;
        idx.push_back(index);
        break;
      }
    }
    if (!hit)
    {
      idx.push_back((IndexType)(vtx.size() / GetFertexSize(format)));
      vtxex++;
      vtx.resize(vtx.size() + GetFertexSize(format));
      void *v = &*(&vtx.back() - GetFertexSize(format) + 1);
      if (format&FTX_POSITION)
        GetFertexOffsetPosition(v, format) =
        Vec3(shape.position[ref.position].x, shape.position[ref.position].y, shape.position[ref.position].z);
      if (format&FTX_NORMAL)
        GetFertexOffsetNormal(v, format) =
        Vec3(shape.normal[ref.normal].x, shape.normal[ref.normal].y, shape.normal[ref.normal].z);
      if (FTX_WEIGHT1 == (format&FTX_WEIGHTMASK))
      {
        GetFertexOffsetWeight1(v, format).weightindex =
          static_cast<float>(shrinked_bone_index[wic[ref.position][0].index]);
      }
      if (FTX_WEIGHT2 == (format&FTX_WEIGHTMASK))
      {
        FTX_Type<FTX_WEIGHT2>::type &fv = GetFertexOffsetWeight2(v, format);
        BlendIndex &bi = wic[ref.position];
        fv.weightindex[0] =
          static_cast<float>(shrinked_bone_index[bi[0].index]);
        fv.weight[0] = wic[ref.position][0].weight;
        if (1 < bi.size())
        {
          fv.weightindex[1] =
            static_cast<float>(shrinked_bone_index[bi[1].index]);
          fv.weight[1] = wic[ref.position][1].weight;
        }
        else
        {
          fv.weightindex[1] = static_cast<float>(0);
          fv.weight[1] = 0;
        }
      }
      if (format&FTX_TEX1)
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
      iviv.push_back(ref);
    }
  }

  raw_->CreateVertexBuffer(&primitive->vb, format, vtxex);
  raw_->CreateIndexBuffer(&primitive->ib, idx.size());

  primitive->morph = shape.morphing;

  //static int polys=0;
  //polys+=idx.size()/3;
  //printf("tris=%d\n",polys);

  primitive->vb->WriteVertex(&vtx.front());
  primitive->ib->WriteIndex(&idx.front());
  primitive->subset.resize(1);
  primitive->subset[0].material = subset.material;
  primitive->subset[0].faces = subset.indices.size() / 3;
}

void ManagedGraphics::CreateVBIBFromShape(VertexBuffer **vb, IndexBuffer **ib, const Shape &shape)
{
  int format = 0;
  if (shape.position.size())
    format |= FTX_POSITION;
  if (shape.normal.size())
    format |= FTX_NORMAL;
  if (shape.texture.size())
    format |= FTX_TEX1;
  int vertices = 0;
  int mx = 0;
  mx = (int)shape.position.size();
  if (mx < (int)shape.normal.size())
    mx = (int)shape.normal.size();
  vertices += mx;

  int indices = 0;
  for (int j = 0; j < shape.subset.size(); j++)
    indices += (int)shape.subset[j].indices.size();


  std::vector<std::vector<WeightIndex> > wic;
  wic.resize(shape.position.size());
  for (int j = 0; j < shape.weight.size(); j++)
  {
    for (int k = 0; k < shape.weight[j].position.size(); k++)
    {
      WeightIndex &wi = _.push(wic[shape.weight[j].position[k].index]);
      wi.index = static_cast<IndexType>(j);
      wi.weight = shape.weight[j].position[k].weight;
    }
  }
  int maxblends = 0;
  for (int j = 0; j < wic.size(); j++)
  {
    if ((int)wic[j].size() > maxblends)
      maxblends = (int)wic[j].size();
  }
  if (maxblends == 1)
    format |= FTX_WEIGHT1;
  else if (maxblends == 2)
    format |= FTX_WEIGHT2;
  else if (maxblends == 3)
    format |= FTX_WEIGHT2;
  else if (maxblends >= 4)
    format |= FTX_WEIGHT2;

  std::vector<char> vtx;
  std::vector<IndexType> idx;
  int vtxex = 0;
  std::vector<PntRefs> iviv;
  for (int j = 0; j < shape.subset.size(); j++)
  {
    for (int k = 0; k < shape.subset[j].indices.size(); k++)
    {
      const IndexVertex &iv = shape.subset[j].indices[k];
      PntRefs ref;

      ref.position =
        (format&FTX_POSITION) ? iv.position : 0;

      ref.normal =
        (format&FTX_NORMAL) ? iv.normal : 0;

      ref.tex1 =
        (format&FTX_TEX1) ? iv.texture : 0;

      bool hit = false;
      //for(IndexType index=0;index<iviv.size();index++)
      //{
      //	if(((format&FTX_POSITION)?ref.position==iviv[index].position:true)
      //		&&((format&FTX_NORMAL)?ref.normal==iviv[index].normal:true)
      //		&&((format&FTX_TEX1)?ref.tex1==iviv[index].tex1:true)
      //		)
      //	{
      //		hit=true;
      //		idx.push_back(index);
      //		break;
      //	}
      //}
      if (!hit)
      {
        idx.push_back((IndexType)(vtx.size() / GetFertexSize(format)));
        vtxex++;
        vtx.resize(vtx.size() + GetFertexSize(format));
        void *v = &*(&vtx.back() - GetFertexSize(format) + 1);
        if (format&FTX_POSITION)
          GetFertexOffsetPosition(v, format) =
          Vec3(shape.position[ref.position].x, shape.position[ref.position].y, shape.position[ref.position].z);
        if (format&FTX_NORMAL)
          GetFertexOffsetNormal(v, format) =
          Vec3(shape.normal[ref.normal].x, shape.normal[ref.normal].y, shape.normal[ref.normal].z);
        if (FTX_WEIGHT1 == (format&FTX_WEIGHTMASK))
        {
          GetFertexOffsetWeight1(v, format).weightindex = wic[ref.position][0].index;
        }
        if (FTX_WEIGHT2 == (format&FTX_WEIGHTMASK))
        {
          FTX_Type<FTX_WEIGHT2>::type &fv = GetFertexOffsetWeight2(v, format);
          fv.weightindex[0] = wic[ref.position][0].index;
          fv.weightindex[1] = wic[ref.position][1].index;
          fv.weight[0] = wic[ref.position][0].weight;
          fv.weight[1] = wic[ref.position][1].weight;
        }
        if (format&FTX_TEX1)
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
        iviv.push_back(ref);
      }
    }
  }

  raw_->CreateVertexBuffer(vb, format, vtxex);
  raw_->CreateIndexBuffer(ib, idx.size());

  (*vb)->WriteVertex(&vtx.front());
  (*ib)->WriteIndex(&idx.front());
}

bool ManagedGraphics::CreateGrid(Grid **iface)
{
  *iface = new GridImpl(this);
  return true;
}

void ManagedGraphics::CreateParticle(Particle **iface)
{
  *iface = new ParticleImpl(this);
}

void ManagedGraphics::CreatePointicle(Pointicle **obj)
{
  *obj = new PointicleImpl(this);
}

void ManagedGraphics::CreateLinecle(Linecle **obj)
{
  *obj = new LinecleImpl(this);
}

void ManagedGraphics::CreateModelFromBone(Model *model, const Bone &bone)
{
  model->name = bone.name;
  model->transform = bone.transform;

  for (int i = 0; i < bone.shape.size(); i++)
  {
    const Shape &shape = bone.shape[i];
#if 0
    model->primitive.resize(shape.subset.size());
    for (int j = 0; j < shape.subset.size(); j++)
    {
      CreatePrimitiveFromSubset(&model->primitive[j], shape, j);
}
#else
    Primitive &primitive = _.push(model->primitive);
    CreateVBIBFromShape(&primitive.vb, &primitive.ib, shape);
    primitive.boneindex.resize(shape.weight.size());
    for (int j = 0; j < shape.weight.size(); j++)
    {
      primitive.boneindex[j].index = shape.weight[j].bonename;
      primitive.boneindex[j].offset = shape.weight[j].offset;
    }
    primitive.subset.resize(shape.subset.size());
    for (int j = 0; j < shape.subset.size(); j++)
    {
      primitive.subset[j].material = shape.subset[j].material;
      primitive.subset[j].faces = (int)shape.subset[j].indices.size() / 3;
    }
#endif
  }
  model->child.resize(bone.child.size());
  for (int i = 0; i < bone.child.size(); i++)
  {
    CreateModelFromBone(&model->child[i], bone.child[i]);
  }
}

void ManagedGraphics::DeleteModel(const std::string &name)
{
  if (_.has(model_map_, name))
  {
    Model *model = &model_map_[name];
    for (int i = 0; i < model->primitive.size(); i++)
    {
      model->primitive[i].vb->Release();
      model->primitive[i].ib->Release();
    }
    model_map_.erase(name);
  }
}

static void LoadModelTexture(ManagedGraphics *dev, const Model &mdl)
{
  for (int i = 0; i < mdl.primitive.size(); i++)
  {
    for (int j = 0; j < mdl.primitive[i].subset.size(); j++)
    {
      dev->GetTexture(mdl.primitive[i].subset[j].material.texture.data());
    }
  }
  for (int i = 0; i < mdl.child.size(); i++)
  {
    LoadModelTexture(dev, mdl.child[i]);
  }
}

Model* ManagedGraphics::GetModel(const std::string &name)
{
  if (_.has(model_map_, name))
  {
    return &model_map_[name];
  }
  else
  {
    if (Bone *bone = GetBone(name))
    {
      CreateModelFromBone(&model_map_[name], *bone);
      LoadModelTexture(this, model_map_[name]);
      return &model_map_[name];
    }
    else
      return 0;
  }
}

AnimatedPose* ManagedGraphics::GetAnimatedPose(const std::string &bonename, const std::string &animname)
{
  std::pair<std::string, std::string> name(bonename, animname);
  if (_.has(pose_map_, name))
  {
    return &pose_map_[name];
  }
  else
  {
    Bone *bone = GetBone(bonename);
    Animation *anim = GetAnimation(animname);
    if (bone&&anim)
    {
      CreateAnimatedPose(pose_map_[name], *bone, *anim);
      return &pose_map_[name];
    }
  }
  return 0;
}

void ManagedGraphics::DrawAnimatedModel(Matrix mat, const Model &model, const AnimatedPose &pose)
{
  world_transform_->Transform(mat);
  for (int i = 0; i < model.primitive.size(); i++)
  {
    const Primitive &primitive = model.primitive[i];
    for (int j = 0; j < primitive.boneindex.size(); j++)
    {
      Matrix world = primitive.boneindex[j].offset;
      const Pose *p = pose.GetBone(model.primitive[i].boneindex[j].index);
      if (p)
        world *= p->absolute;
      blend_transform_->Transform(world, (int)j);
    }
    int offset = 0;
    for (int j = 0; j < primitive.subset.size(); j++)
    {
      const Faces &face = primitive.subset[j];
      const Material &material = primitive.subset[j].material;
      //basic_material_->Diffuse(material.diffuse);
      //basic_material_->Ambient(material.ambient);
      int slot = 0;
      raw_->SetShader(blend_transform_, slot++);
      raw_->SetShader(world_transform_, slot++);
      if (directional_light_enable_)
      {
        raw_->SetShader(directional_light_, slot++);
      }
      if (material.texture.size())
      {
        if (Texture *decal = GetTexture(material.texture.data()))
        {
          decal_texture_->SetTexture(decal);
          raw_->SetShader(decal_texture_, slot++);
        }
      }
      //raw_->SetShader(basic_material_, slot++);
      if (material.power > 0)
      {
        specular_light_->Specular(material.specular);
        specular_light_->Power(material.power);
        raw_->SetShader(specular_light_, slot++);
      }
      raw_->SetShader(camera_, slot++);
      raw_->SetShader(shader::ShaderEnd(), slot++);
      raw_->DrawIndexedPrimitive(primitive.vb, primitive.ib, offset, face.faces);
      offset += face.faces * 3;
    }
  }
  for (int i = 0; i < model.child.size(); i++)
  {
    DrawAnimatedModel(mat, model.child[i], pose);
  }
}

void ManagedGraphics::DrawPrimitive(const Matrix &mat, const Primitive &primitive)
{
  world_transform_->Transform(mat);
  int offset = 0;
  for (int j = 0; j < primitive.subset.size(); j++)
  {
    const Faces &face = primitive.subset[j];
    const Material &material = primitive.subset[j].material;
    //basic_material_->Diffuse(material.diffuse);
    //basic_material_->Ambient(material.ambient);
    int slot = 0;

    raw_->SetShader(world_transform_, slot++);
    raw_->SetShader(camera_, slot++);
    if (directional_light_enable_)
    {
      raw_->SetShader(directional_light_, slot++);
    }
    if (material.texture.size())
    {
      if (Texture *decal = GetTexture(material.texture.data()))
      {
        decal_texture_->SetTexture(decal);
        raw_->SetShader(decal_texture_, slot++);
      }
    }
    //raw_->SetShader(basic_material_, slot++);
    if (material.power > 0)
    {
      specular_light_->Specular(material.specular);
      specular_light_->Power(material.power);
      raw_->SetShader(specular_light_, slot++);
    }
    raw_->SetShader(shader::ShaderEnd(), slot++);
    raw_->DrawIndexedPrimitive(primitive.vb, primitive.ib, offset, face.faces);
    offset += face.faces * 3;
  }
}

void ManagedGraphics::DrawModel(Matrix mat, const Model *model)
{
  mat = model->transform*mat;
  world_transform_->Transform(mat);
  for (int i = 0; i < model->primitive.size(); i++)
  {
    const Primitive &primitive = model->primitive[i];
    DrawPrimitive(mat, primitive);

    //int offset=0;
    //for(int j=0;j<primitive.subset.size();j++)
    //{
    //	const Faces &face=primitive.subset[j];
    //	const Material &material=primitive.subset[j].material;
    //	basic_material_->Diffuse(material.diffuse);
    //	basic_material_->Ambient(material.ambient);
    //	basic_material_->Specular(material.specular);
    //	basic_material_->Power(material.power);
    //	int slot=0;

    //	raw_->SetShader(world_transform_,slot++);
    //	raw_->SetShader(camera_,slot++);
    //	if(directional_light_enable_)
    //             {
    //		raw_->SetShader(directional_light_,slot++);
    //             }
    //	if(material.texture.size())
    //	{
    //		if(Texture *decal=GetTexture(material.texture.data()))
    //		{
    //			decal_texture_->SetTexture(decal);
    //			raw_->SetShader(decal_texture_,slot++);
    //		}
    //	}
    //	raw_->SetShader(basic_material_,slot++);
    //	if(material.power>0)
    //	{
    //		specular_light_->Specular(material.specular);
    //		specular_light_->Power(material.power);
    //		raw_->SetShader(specular_light_,slot++);
    //	}
    //	raw_->SetShader(shader::ShaderEnd(),slot++);
    //	raw_->DrawIndexedPrimitive(primitive.vb,primitive.ib,offset,face.faces);
    //	offset+=face.faces*3;
    //}
  }
  for (int i = 0; i < model->child.size(); i++)
  {
    DrawModel(mat, &model->child[i]);
  }
}
