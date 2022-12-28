#include "sys/viewport.h"
#include "gfx/graphics.h"
#include "gfx/fontmap.h"
#include "sys/fs.h"
#include "main/underscore.h"

#include "gfx/polygon.h"
#include "gfx/graphics.h"
#include <thread>
#include <chrono>
#include <functional>
#include <vector>
#include "scene.h"
#include "gfx/fertex.h"
using namespace gh;

struct VP :public ViewportMessage {
  void CreateFail() {

  }
  void CloseButton() {
    is_close = true;
  }
  bool ltouch = false;
  float forward = 0;
  float yaw = PI / 2, pitch = 0;
  float delta_x = 0, delta_y = 0;
  void Mouse(int x, int y, int z, unsigned int button) {
    if (button & ViewportMessage::MOUSE_L) {
      ltouch = true;
    }
    else {
      ltouch = false;
    }
    if (x) {
      if (0 == delta_x) {
        delta_x = x;
      }
      if (ltouch)
        yaw += (x - delta_x) / -200.0f;
      delta_x = x;
    }
    else {
      delta_x = 0;
    }
    if (y) {
      if (0 == delta_y) {
        delta_y = 0;
      }
      if (ltouch)
        pitch += (y - delta_y) / -200.0f;
      delta_y = y;
    }
    else {
      delta_y = 0;
    }
    if (0 < z) {
      forward += 1.0f;
    }
    else if (0 > z) {
      forward -= 1.0f;
    }
  }
  void Open() {
    is_open = true;
    gf = gh::CreateGraphicsD3D();
    gf->InitializeFromViewport(vp);
    gf->SetBgColor(0xff444444);
    if (setup) {
      setup();
    }
  }
  void Close() {
    is_close = true;
  }
  void Resize(int w, int h) {
  }
  bool is_open = false;
  bool is_close = false;
  gh::Graphics* gf;
  gh::Viewport* vp;
  std::function<void()> setup;
};

bool intersect_triangle(
  const Vector3& Origin, const Vector3& Dir, const Vector3& A, const Vector3& B, const Vector3& C, Vector3* Hit = nullptr
) {
  float t, u, v;
  Vector3 E1 = B - A;
  Vector3 E2 = C - A;
  Vector3 N = Cross(E1, E2);
  float det = -Dot(Dir, N);
  float invdet = 1.0 / det;
  Vector3 AO = Origin - A;
  Vector3 DAO = Cross(AO, Dir);
  u = Dot(E2, DAO) * invdet;
  v = -Dot(E1, DAO) * invdet;
  t = Dot(AO, N) * invdet;
  if (det >= 1e-6 && t >= 0.0 && u >= 0.0 && v >= 0.0 && (u + v) <= 1.0) {
    if (Hit) {
      *Hit = Origin + Dir * t;
    }
    return true;
  }
  else {
    return false;
  }
}

bool rayTriangleIntersect(
  const Vector3& orig, const Vector3& dir,
  const Vector3& v0, const Vector3& v1, const Vector3& v2,
  Vector3* Hit)
{
  float t;
  // compute plane's normal
  Vector3 v0v1 = v1 - v0;
  Vector3 v0v2 = v2 - v0;
  // no need to normalize
  Vector3 N = Cross(v0v1, v0v2);  //N 
  float area2 = N.Magnitude();

  // Step 1: finding P

  // check if ray and plane are parallel.
  float NdotRayDirection = Dot(N, dir);
  if (fabs(NdotRayDirection) < EPSILON)  //almost 0 
    return false;  //they are parallel so they don't intersect ! 

// compute d parameter using equation 2
  float d = -Dot(N, v0);

  // compute t (equation 3)
  t = -(Dot(N, orig) + d) / NdotRayDirection;

  // check if the triangle is in behind the ray
  if (t < 0) return false;  //the triangle is behind 

  // compute the intersection point using equation 1
  Vector3 P = orig + dir * t;

  // Step 2: inside-outside test
  Vector3 C;  //vector perpendicular to triangle's plane 

  // edge 0
  Vector3 edge0 = v1 - v0;
  Vector3 vp0 = P - v0;
  C = Cross(edge0, vp0);
  if (Dot(N, C) < 0) return false;  //P is on the right side 

  // edge 1
  Vector3 edge1 = v2 - v1;
  Vector3 vp1 = P - v1;
  C = Cross(edge1, vp1);
  if (Dot(N, C) < 0)  return false;  //P is on the right side 

  // edge 2
  Vector3 edge2 = v0 - v2;
  Vector3 vp2 = P - v2;
  C = Cross(edge2, vp2);
  if (Dot(N, C) < 0) return false;  //P is on the right side; 

  *Hit = P;

  return true;  //this ray hits the triangle 
}

typedef Fertex<FTX_POSITION | FTX_DIFFUSE> Vpc;

Vpc vbox[8] = {
  {Vec3(1,1,1),0x440000ff},
  {Vec3(-1,1,1),0x440000ff},
  {Vec3(1,-1,1),0x440000ff},
  {Vec3(-1,-1,1),0x440000ff},
  {Vec3(1,1,-1),0x440000ff},
  {Vec3(-1,1,-1),0x440000ff},
  {Vec3(1,-1,-1),0x440000ff},
  {Vec3(-1,-1,-1),0x440000ff},
};

uint16_t ibox[24] = {
  0,1,1,3,3,2,2,0,
  0,4,1,5,2,6,3,7,
  4,5,5,7,7,6,6,4,
};

uint16_t itbox[32] = {

};

typedef Fertex<FTX_POSITION | FTX_NORMAL | FTX_DIFFUSE> Vgg;

Vgg vgrass[] = {
  {Vec3(0,0,0),Vec3(0,0,1),0xFF444466},
  {Vec3(1,10,0),Vec3(0,0,1),0xFF66ff88},
  {Vec3(2,0,0),Vec3(0,0,1),0xFF444455},
  {Vec3(3,10,0),Vec3(0,0,1),0xFF66ff99},
  {Vec3(4,0,0),Vec3(0,0,1),0xFF444444},
  {Vec3(5,10,0),Vec3(0,0,1),0xFF66ff77},
  {Vec3(6,0,0),Vec3(0,0,1),0xFF444455},
  {Vec3(7,10,0),Vec3(0,0,1),0xFF66ff66},
  {Vec3(8,0,0),Vec3(0,0,1),0xFF444466},
  {Vec3(9,10,0),Vec3(0,0,1),0xFF66ff77},
  {Vec3(10,0,0),Vec3(0,0,1),0xFF444466},
};

uint16_t igrass[] = {
  0,1,2,2,3,4,4,5,6,6,7,8,8,9,10
};

typedef Fertex<FTX_POSITION | FTX_TEX1> Vpt;

struct DisplaceTexture {
  Texture* displace;
  Matrix proj;
  Matrix view;
  shader::GrassMap* gm;
  void Setup(Graphics *gf,std::function<void()> render) {
    gf->CreateRenderTarget(512, 512, TextureFormat::TEXTURE_ARGB8, TextureFormat::TEXTURE_D24S8, &displace);

    float wh = 300;
    view.LookAtLH({ 0,100,0 }, { 0,0,0 }, { 1,0,0 });
    proj.OrthoLH(-100, 110, wh, wh);
    Matrix vp = view * proj;

    gm = gf->CreateShader<shader::ClassID_GrassMap>();
    gm->SetProjection(vp);
    gm->SetTexture(displace);
    gf->SetRenderTarget(displace);
    gf->SetRenderState(RENDER_Z, true);
    gf->SetBgColor(0xffffffff);

    gf->clear();
    gf->BeginScene();
    render();
    gf->EndScene();

    gf->SetRenderTarget(nullptr);
  }
};

#include <Windows.h>
#include <WinUser.h>

int main(int, char**)
{
  VP m;
  ViewportInitializeParameter p;
  Scene* st;

  Mesh* ms = CreateMesh();
  Mesh* ground = CreateMesh();
  Mesh* grass = CreateMesh();

  m.setup = [&]() {
  };

  m.vp = CreateViewport(p, &m);
  while (!m.is_open) {
    m.vp->MessageFetch();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  CreateScene(&st);

  VertexBuffer* gvb;
  IndexBuffer* gib;
  {
    std::vector<Vgg> vtx;
    std::vector<uint16_t> idx;
    int vn = 0, in = 0;
    idx.resize(sizeof(igrass) / sizeof(igrass[0]) * 1000);
    vtx.resize(sizeof(vgrass) / sizeof(vgrass[0]) * 1000);
    for (int i = 0; i < 1000; i++) {
      Matrix mat;
      mat.Identity();
      mat.Translate(-5, 0, 0);
      float r = _.randomf() * PI * 2;
      mat.RotateY(r);
      mat.Translate(_.randomf() * 45, 0, _.randomf() * 45);
      for (auto v : vgrass) {
        mat.Transform(v.position);
        v.position.y += _.randomf() * 2 - 1;
        v.normal.x = cosf(r);
        v.normal.y = _.randomf() * 0.5f + 1.0f;
        v.normal.z = sinf(r);
        v.normal.Normalize();
        vtx[vn++] = v;
      }
      for (auto q : igrass) {
        idx[in++] = q + i * sizeof(vgrass) / sizeof(vgrass[0]);
      }
    }
    m.gf->CreateVertexBuffer(&gvb, Vgg::format, vtx.size());
    m.gf->CreateIndexBuffer(&gib, idx.size());
    gvb->WriteVertex(&vtx.front());
    gib->WriteIndex(&idx.front());

  }

  {
    Bone* bn = st->GetBone("/model/canyon.glb");
    grass->Setup(m.gf);
    bn->shape[0].SubsetMerge();
    grass->InitOneSkin(bn->shape[0]);
    ground->Setup(m.gf);
    ground->InitOneSkin(bn->shape[2]);
  }

  {
    Bone* bn = st->GetBone("/model/kenny_nature/tree_palmTall.glb");
    ms->Setup(m.gf);
    Shape sh = bn->shape[0];
    sh.SubsetMerge();
    ms->InitOneSkin(sh);
  }
  Mesh* qin = CreateMesh();
  {
    Bone* bn = st->GetBone("/model/qin.pmx");
    qin->Setup(m.gf);
    qin->InitOneSkin(bn->shape[0]);
  }
  shader::Camera* cam = m.gf->CreateShader<shader::ClassID_Camera>();
  shader::WorldTransform* form = m.gf->CreateShader<shader::ClassID_WorldTransform>();
  shader::SoftLight* light = m.gf->CreateShader<shader::ClassID_SoftLight>();

  Vector3 from;
  Vector3 at;
  Vector3 up;
  float fov;
  float near_clip;
  float far_clip;
  float aspect;
  up = { 0, 1, 0 };
  near_clip = 1;
  far_clip = 1000;
  fov = PI / 3;
  aspect = 9.0f / 16.0f;

  cam->PerspectiveFov(near_clip, far_clip, fov, aspect);

  Vector3 light_dir = { -5,5,2 };

  light->Diffuse(0xffffffff);
  light->Direction(light_dir);
  light->Ambient(0xff888888);

  light->Specular(0xff444444);
  light->Power(10.0f);

  Matrix sp;

  Texture* sd;
  m.gf->CreateRenderTarget(512, 512, TextureFormat::TEXTURE_R32F, TextureFormat::TEXTURE_D24S8, &sd);

  shader::ShadowMap* sm = m.gf->CreateShader<shader::ClassID_ShadowMap>();
  sm->SetProjection(sp);
  sm->SetTexture(sd);
  shader::ShadowMapProjection* smp = m.gf->CreateShader<shader::ClassID_ShadowMapProjection>();
  smp->SetProjection(sp);
  smp->SetTexture(sd);

  DisplaceTexture dt;
  {
    dt.Setup(m.gf, [&]() {
      Matrix mat;
      mat.Identity();
      mat.Scale(200);
      form->Transform(mat);
      int slot = 0;
      m.gf->SetShader(form, slot++);
      m.gf->SetShader(dt.gm, slot++);
      m.gf->SetShader(shader::ShaderEnd(), slot);
      ground->Draw();
    });
  }

  Vpt plane[6] = {
    { Vec3(-1,-1,0),0,1 },
    { Vec3(-1,1,0),0,0 },
    { Vec3(1,-1,0),1,1 },
    { Vec3(1,-1,0),1,1 },
    { Vec3(-1,1,0),0,0 },
    { Vec3(1,1,0),1,0 },
  };

  shader::DecalTexture* decal = m.gf->CreateShader<shader::ClassID_DecalTexture>();

  at = { 0,10,20 };
  Vector3 acc = { 0,0,0 };
  int count = 0;
  while (!m.is_close) {
    m.vp->MessageFetch();
    if (GetAsyncKeyState('R')) {
      count++;
    }

    if (GetAsyncKeyState('E')) {
      acc -= {cosf(m.yaw), 0, sinf(m.yaw)};
    }
    if (GetAsyncKeyState('D')) {
      acc += {cosf(m.yaw), 0, sinf(m.yaw)};
    }
    if (GetAsyncKeyState('S')) {
      acc += {sinf(m.yaw), 0, -cosf(m.yaw)};
    }
    if (GetAsyncKeyState('F')) {
      acc -= {sinf(m.yaw), 0, -cosf(m.yaw)};
    }
    if (GetAsyncKeyState(VK_SPACE)) {
      acc += {0, 1, 0};
    }
    if (GetAsyncKeyState(VK_SHIFT)) {
      acc -= {0, 1, 0};
    }
    at += acc * 0.1f;
    if (acc.Magnitude() > 0.1f) {
      acc *= 0.8f;
    }
    else {
      acc = { 0,0,0 };
    }

    Vector3 cam_dir = { cosf(m.yaw),-m.pitch,sinf(m.yaw) };
    from = cam_dir + at;
    cam->LookAt(from, at, up);
    Vector3 light_center = at - cam_dir * 100;

#if 0
    //ray pick ‚Å‰e‚Ì’†S‚ð’T‚·
    {
      Bone* bn = st->GetBone("/model/canyon.glb");
      auto& idx = bn->shape[2].subset[0].indices;
      auto& pos = bn->shape[2].position;
      Vector3 tmp;
      bool hit = false;
      Vector3 Origin = from / 200.0f;
      Vector3 Dir = cam_dir / -200.0f;
      float l = 10000000;
      for (int n = 0; n < idx.size() / 3; n++) {
        if (rayTriangleIntersect(Origin, Dir, pos[idx[n * 3 + 0].position], pos[idx[n * 3 + 1].position], pos[idx[n * 3 + 2].position], &tmp)) {
          hit = true;
          if ((Origin - tmp).Magnitude() < l) {
            l = (Origin - tmp).Magnitude();
            light_center = tmp * 200;
          }
        }
      }
    }
#else
    {
      light_center.y += 10;
    }
#endif

    {
      light_dir = { cosf(count / 200.0f) * 5,5,sinf(count / 200.0f) * 5 };
      light->Direction(light_dir);
      Matrix view, proj;
      float wh = (at - light_center).Magnitude() * 2 + 10;
      //light_center = at;
      //wh = 300;
      view.LookAtLH(light_dir + light_center, light_center, { 0,1,0 });
      proj.OrthoLH(-500, 500, wh, wh);
      sp = view * proj;
      sm->SetProjection(sp);
      smp->SetProjection(sp);
    }

    m.gf->SetRenderTarget(sd);
    m.gf->SetRenderState(RENDER_Z, true);
    m.gf->SetBgColor(0xffffffff);

    m.gf->clear();
    m.gf->BeginScene();
    Matrix mat;
    {
      int slot = 0;
      m.gf->SetShader(form, slot++);
      m.gf->SetShader(sm, slot++);
      m.gf->SetShader(shader::ShaderEnd(), slot);
      Bone* bn = st->GetBone("/model/canyon.glb");
      for (const auto& i : bn->child[2].child) {
        if (i.name == "tree_palmTall") {
          Matrix w = i.transform;
          w.Scale(200);
          mat.Identity();
          mat.RotateY(w._43);
          mat.Scale(10);
          mat.Translate(w._41, w._42, w._43);
          form->Transform(mat);
          ms->Draw();
        }
      }
    }
    mat.Identity();
    mat.Scale(200);
    form->Transform(mat);
    ground->Draw();


    m.gf->EndScene();

    m.gf->SetRenderTarget(nullptr);
    m.gf->SetBgColor(0xff886644);
    m.gf->clear();
    m.gf->BeginScene();

    //for (int y = 0; y < 5; y++) {
    //  for (int x = 0; x < 5; x++) {
    //    Matrix mat;
    //    mat.Identity();
    //    mat.Scale(0.5 * abs(cos(x + y)) + 1.0);
    //    mat.Scale(15, 15, 15);
    //    mat.RotateY(y * 2 * x * count / 200.0f);
    //    mat.Translate((x - 2) * 20, 0, (y - 2) * 20);

    //    form->Transform(mat);
    //    ms->Draw();
    //  }
    //}
    {
      int slot = 0;
      m.gf->SetShader(form, slot++);
      m.gf->SetShader(light, slot++);
      m.gf->SetShader(smp, slot++);
      m.gf->SetShader(cam, slot++);
      m.gf->SetShader(shader::ShaderEnd(), slot);
      Bone* bn = st->GetBone("/model/canyon.glb");
      for (const auto& i : bn->child[2].child) {
        if (i.name == "tree_palmTall") {
          Matrix w = i.transform;
          w.Scale(200);
          mat.Identity();
          mat.RotateY(w._43);
          mat.Scale(10);
          mat.Translate(w._41, w._42, w._43);
          form->Transform(mat);
          ms->Draw();
        }
      }
      slot = 0;
      m.gf->SetShader(form, slot++);
      m.gf->SetShader(light, slot++);
      m.gf->SetShader(smp, slot++);
      m.gf->SetShader(cam, slot++);
      m.gf->SetShader(shader::ShaderEnd(), slot);
      for (const auto& i : bn->child[2].child) {
        if (i.name == "grass_leafs_") {
          Matrix w = i.transform;
          mat.Identity();
          mat.RotateY(w._13);
          mat.Scale(5, 25 * w._22, 5);
          w.Scale(200);
          mat.Translate(w._41, w._42, w._43);
          form->Transform(mat);
          grass->Draw();
        }
      }
    }

    //gnd
    mat.Identity();
    mat.Scale(200);
    form->Transform(mat);
    ground->Draw();

    mat.Identity();
    mat.Scale(1);
    form->Transform(mat);
    //qin->Draw2([&](int j) {  });
    //m.gf->DrawPrimitiveUP(PRIMITIVE_TRIANGLELIST, 2, plane, Vpt::format);

    //grass
    {
      int slot = 0;
      m.gf->SetShader(form, slot++);
      m.gf->SetShader(light, slot++);
      m.gf->SetShader(smp, slot++);
      m.gf->SetShader(cam, slot++);
      m.gf->SetShader(shader::ShaderEnd(), slot);
      mat.Identity();
      mat.Scale(2);
      mat.Translate(0, 10, 0);
      m.gf->SetRenderState(RENDER_CULLMODE, CULL_NONE);
      m.gf->DrawIndexedPrimitive(gvb, gib, 0, gib->Indices() / 3);
      m.gf->SetRenderState(RENDER_CULLMODE, CULL_CCW);
    }


    // air box
    {
      mat = sp;
      mat.Inverse();
      form->Transform(mat);
      int slot = 0;
      m.gf->SetShader(form, slot++);
      m.gf->SetShader(cam, slot++);
      m.gf->SetShader(shader::ShaderEnd(), slot++);
      m.gf->DrawIndexedPrimitiveUP(PRIMITIVE_LINELIST, 12, ibox, vbox, 8, Vpc::format);
    }

    {
      Matrix mat;
      mat.Identity();
      mat.Scale(0.2, 0.2, 1);
      mat.Translate(-0.8, -0.8, 0);

      form->Transform(mat);

      decal->SetTexture(sd);
      int slot = 0;
      m.gf->SetShader(form, slot++);
      m.gf->SetShader(decal, slot++);
      m.gf->SetShader(shader::ShaderEnd(), slot++);
      m.gf->DrawPrimitiveUP(PRIMITIVE_TRIANGLELIST, 2, plane, Vpt::format);
    }
    {
      Matrix mat;
      mat.Identity();
      mat.Scale(0.2, 0.2, 1);
      mat.Translate(-0.8, -0.4, 0);

      form->Transform(mat);

      decal->SetTexture(dt.displace);
      int slot = 0;
      m.gf->SetShader(form, slot++);
      m.gf->SetShader(decal, slot++);
      m.gf->SetShader(shader::ShaderEnd(), slot++);
      m.gf->DrawPrimitiveUP(PRIMITIVE_TRIANGLELIST, 2, plane, Vpt::format);
    }

    m.gf->EndScene();

    m.gf->Flip(m.vp);


    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  DeleteViewport(m.vp);
}

