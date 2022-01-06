#include "doc.h"
#include "main/underscore.h"
using namespace gh;
#include "gfx/fertex.h"

IdGenerator<std::uint32_t> IdTypeGenerator<Element>::gen;

void Node::Append(Element* e) {
  node.insert(node.end(),e);
}

using namespace gh;

DocRender::DocRender(gh::Graphics* gf) {
  this->gf = gf;
  CreateManagedGraphics(&mg);
  mg->Initialize(gf);
  cam = gf->CreateShader < gh::shader::ShaderType::ClassID_Camera>();

}

typedef Fertex<FTX_POSITION | FTX_DIFFUSE> Vertex;

template<typename F>
void MakeGrid(std::vector<F>& v, int n) {
  for (int i = 0; i <= n; i++) {
    Color c = (i % 10) ? Color(0xff999999) : Color(0xffbbbbbb);
    v.push_back({ Vec3(i - n / 2,0,-n / 2),c });
    v.push_back({ Vec3(i - n / 2,0,n / 2),c });
  }
  for (int i = 0; i <= n; i++) {
    Color c = (i % 10) ? Color(0xff999999) : Color(0xffbbbbbb);
    v.push_back({ Vec3(-n / 2,0,i - n / 2),c });
    v.push_back({ Vec3(n / 2,0,i - n / 2),c });
  }
}

const Vector3 HEX[7] = {
  {0.0f   ,0.0f,-1.0f},
  {0.866f,0.0f,-0.5f},
  {0.866f,0.0f,0.5f },
  {0.0f   ,0.0f,1.0f },
  {-0.866f ,0.0f,0.5f },
  {-0.866f ,0.0f,-0.5f},
  {0.0f   ,0.0f,-1.0f},
};
void DocRender::Draw(Node* root) {
  int shader_slot = 0;
  mg->directional_light_->Diffuse(0xffffffff);
  mg->directional_light_->Direction(Vec3(1, 2, -1));
  mg->directional_light_enable_ = true;
  for (auto i : root->node) {
    if (i->type == Element::FOVCAMERA) {
      auto f = reinterpret_cast<FovCamera*>(&*i);
      mg->camera_->LookAt(f->from, f->at, f->up);
      mg->camera_->PerspectiveFov(f->near_clip,f->far_clip, f->fov, f->aspect);
      gf->SetShader(mg->camera_, shader_slot++);

      std::vector<Vertex> up;
      MakeGrid(up, 60);
      up.push_back({ Vec3(-100,0,0),Color(0xffffaaaa) });
      up.push_back({ Vec3(100,0,0),Color(0xffffaaaa) });
      //up.push_back({ Vec3(0,-100,0),Color(0xffaaffaa) });
      //up.push_back({ Vec3(0,100,0),Color(0xffaaffaa) });
      up.push_back({ Vec3(0,0,-100),Color(0xffaaaaff) });
      up.push_back({ Vec3(0,0,100),Color(0xffaaaaff) });

      gf->SetShader(shader::ShaderEnd(), 1);
      gf->DrawPrimitiveUP(PRIMITIVE_LINELIST, up.size() / 2, up.data(), Vertex::format);

    }

    if (i->type == Element::MESH) {
      auto f = reinterpret_cast<DocMesh*>(&*i);
      auto m=mesh.find(f->path);
      if (m == mesh.end()) {
        auto bn=mg->GetBone(f->path);
        gh::Mesh* ms = mg->CreateMesh();
        ms->InitOneSkin(bn->child[0].child[0].shape[0]);
        mesh[f->path] = ms;
      }
      else {
        m->second->Draw(f->pose);
      }

    }
    //if (i->type == Element::MESH) {
    //  auto f = reinterpret_cast<DocMesh*>(&*i);
    //  auto m = mesh.find(f->path);
    //  if (m == mesh.end()) {
    //    gh::Primitive* prim = new gh::Primitive;
    //    auto bn = mg->GetBone(f->path);
    //    mg->CreatePrimitiveFromSubset(prim, bn->child[0].child[0].shape[0], 0);
    //    mesh[f->path] = prim;
    //  }
    //  else {
    //    auto& prim = *m->second;
    //    Matrix mat;
    //    mat.Identity();
    //    gf->SetShader(shader::ShaderEnd(), 1);
    //    int offset = 0;
    //    for (int j = 0; j < prim.subset.size(); j++)
    //    {
    //      const Faces& face = prim.subset[j];
    //      const Material& material = prim.subset[j].material;
    //      gf->SetShader(shader::ShaderEnd(), shader_slot);
    //      gf->DrawIndexedPrimitive(prim.vb, prim.ib, offset, face.faces);
    //      offset += face.faces * 3;
    //    }
    //  }

    //}

    if (i->type == Element::HEXBLOCK) {
      auto f = reinterpret_cast<HexBlock*>(&*i);

      float fx = 0.5f;
      float fy = 0.866f;
      
      std::vector<Vertex> vtx;
      std::vector<uint16_t> idx;
      int n = 0;
      //for (int y = 0; y < 16; y++) {
      //  for (int x = 0; x < 16; x++) {
      //    Vector3 offs = { x * 1.732f + (y % 2) * 0.866f ,2 + 0.5f * (int)(2 * cosf(y * 0.3f + x * 0.2f)),y * 1.5f };
      //    float r = x * (1.0f / 16);
      //    float g = y * (1.0f / 16);
      //    for (int i = 0; i < 6; i++) {
      //      vtx.push_back({ HEX[i]+offs,Color(r,g,0.5f) });
      //    }
      //    idx.push_back(n+0);
      //    idx.push_back(n+2);
      //    idx.push_back(n+1);
      //    idx.push_back(n+0);
      //    idx.push_back(n+3);
      //    idx.push_back(n+2);
      //    idx.push_back(n+0);
      //    idx.push_back(n+4);
      //    idx.push_back(n+3);
      //    idx.push_back(n+0);
      //    idx.push_back(n+5);
      //    idx.push_back(n+4);
      //    n += 6;
      //    Vector3 boffs = offs;
      //    boffs.y = -1;
      //    for (int i = 0; i < 6; i++) {
      //      vtx.push_back({ HEX[i] + offs,0xff444488 });
      //      vtx.push_back({ HEX[i+1] + offs,0xff444488 });
      //      vtx.push_back({ HEX[i] + boffs,0xff444488 });
      //      vtx.push_back({ HEX[i+1] + boffs,0xff444488 });
      //      idx.push_back(n + 0);
      //      idx.push_back(n + 1);
      //      idx.push_back(n + 2);
      //      idx.push_back(n + 2);
      //      idx.push_back(n + 1);
      //      idx.push_back(n + 3);
      //      n += 4;
      //    }
      //  }
      //}

      //gf->SetShader(shader::ShaderEnd(), 1);
      //gf->DrawIndexedPrimitiveUP(PRIMITIVE_TRIANGLELIST, idx.size() / 3,idx.data(), vtx.data(),vtx.size(), Vertex::format);
    }

  }

}

