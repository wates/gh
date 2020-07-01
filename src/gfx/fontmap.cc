#include "fontmap.h"

#include "fertex.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <ftoutln.h>
#include <ftbbox.h>

#include "../main/underscore.h"

using namespace gh;

typedef Fertex<FTX_POSITION | FTX_TEX1 | FTX_DIFFUSE> Vertex;

struct FontmapImpl
  :public Fontmap
{
  FontmapImpl(const void *ttf, int length, int font_size);
  bool Init(Graphics *dev);
  void Outline(float rad, float den);
  int GetWidth(wchar_t c);
  int GetHeight(wchar_t c);
  Frect GetSize(const wchar_t *str, float padding);
  void MakeShape(Shape *shape, const wchar_t *str, float padding = 0);
  void DrawUp(const Matrix &mat, const wchar_t *str, uint32_t color, bool force_color, float padding, int width);
  void DrawColor(const Matrix &mat, const wchar_t *str, uint32_t color, float padding, int width)
  {
    DrawUp(mat, str, color, true, padding, width);
  }
  void Draw(const Matrix &mat, const wchar_t *str, float padding, int width)
  {
    DrawUp(mat, str, outline_ ? 0xff000000 : 0xffffffff, false, padding, width);
  }
  void ReadPixel(wchar_t c, int *width, int *height, uint8_t *map)
  {
    FT_Load_Char(face_, c, FT_LOAD_RENDER);
    FT_Bitmap &b = slot_->bitmap;
    if (NULL == map)
    {
      *width = b.width;
      *height = b.rows;
      return;
    }

    for (int y = 0; y < b.rows; y++)
    {
      for (int x = 0; x < b.width; x++)
      {
        map[x + y*b.width] = b.buffer[y*b.pitch + x];
      }
    }
  }

  void LoadChar(wchar_t c);

  FT_Library library_;
  FT_Face    face_;
  FT_GlyphSlot slot_;
  FT_UInt glyph_index_;

  int texture_width_;
  int texture_height_;

  int font_size_;
  int offset_x_;
  int offset_y_;
  int max_rows_;

  bool outline_;
  float outline_rad_;
  struct OutlineDensity
  {
    int x;
    int y;
    float density;
  };
  std::vector<OutlineDensity> outline_density_;
  Frect outline_offset_;

  struct FontChar
    :public Frect
  {
    float baseline;
  };
  std::map<wchar_t, FontChar> chars;
  std::vector<Vertex> up;

  Graphics *dev;
  shader::WorldTransform *world;
  shader::DecalTexture *decal;
  Texture *tex;
};

Fontmap* Fontmap::Create(const void* ttf, int length, int font_size)
{
  return new FontmapImpl(ttf, length, font_size);
}

Fontmap::~Fontmap() {

}

FontmapImpl::FontmapImpl(const void *ttf, int length, int font_size)
  :outline_(false)
  , outline_rad_(0)
  , outline_offset_({ 0, 0, 0, 0 })
{
  offset_x_ = 0;
  offset_y_ = 0;
  max_rows_ = 0;
  this->font_size_ = font_size;

  FT_Init_FreeType(&library_);
  FT_New_Memory_Face(library_, static_cast<const uint8_t*>(ttf), length, 0, &face_);

  slot_ = face_->glyph;
  FT_Set_Char_Size(face_, 0, font_size_ * 64, 300, 300);
}

bool FontmapImpl::Init(Graphics *dev)
{
  this->dev = dev;
  this->texture_width_ = 1024;
  this->texture_height_ = 1024;

  dev->CreateTexture(&tex, texture_width_, texture_height_, TEXTURE_A8);

  LockInfo li;
  tex->Lock(li, LOCK_WRITE);
  for (int y = 0; y < texture_height_; y++)
  {
    for (int x = 0; x < texture_width_; x++)
    {
      li.Bits[y*li.Pitch + x] = 0;
    }
  }
  tex->Unlock();

  world = dev->CreateShader<shader::ShaderType::ClassID_WorldTransform>();
  decal = dev->CreateShader<shader::ShaderType::ClassID_AlphaBlendTexture>();

  decal->SetTexture(tex);
  return true;
}

void OuterCircle(const Vector2 &p0, const Vector2 &p1, const Vector2 &p2, float *sqradius, Vector2 *o)
{
  float A1 = 2 * (p1.x - p0.x);
  float B1 = 2 * (p1.y - p0.y);
  float C1 = p0.x*p0.x - p1.x*p1.x + p0.y*p0.y - p1.y*p1.y;
  float A2 = 2 * (p2.x - p0.x);
  float B2 = 2 * (p2.y - p0.y);
  float C2 = p0.x*p0.x - p2.x*p2.x + p0.y*p0.y - p2.y*p2.y;
  o->x = (B1*C2 - B2*C1) / (A1*B2 - A2*B1);
  o->y = (C1*A2 - C2*A1) / (A1*B2 - A2*B1);
  *sqradius = (p0 - *o).Sqlen();
}

struct Trindex
{
  uint16_t pi[3];
};

struct SegmentIndex
{
  std::size_t a, b;
};

void FontmapImpl::MakeShape(Shape *shape, const wchar_t *str, float padding)
{
  _.increase(shape->subset);
  auto &idx = shape->subset[0].indices;
  float width_offset = 0;

  for (const wchar_t *ch = str; *ch; ch++)
  {
    const int idx_offset = shape->position.size();
    std::vector<std::vector<Vector3> > points;

    FT_Load_Char(face_, *ch, FT_LOAD_DEFAULT);
    FT_Outline outline = face_->glyph->outline;
    FT_Outline_Funcs funcs;
    funcs.conic_to = [](const FT_Vector *control, const FT_Vector *to, void *p)->int{
      std::vector<Vector3> *points = &reinterpret_cast<std::vector<std::vector<Vector3> >*>(p)->back();
      points->push_back(Vec3(to->x, to->y, 100.0f));
      return 0;
    };
    funcs.line_to = [](const FT_Vector *to, void *p)->int{
      std::vector<Vector3> *points = &reinterpret_cast<std::vector<std::vector<Vector3> >*>(p)->back();
      points->push_back(Vec3(to->x, to->y, 100.0f));
      return 0;
    };
    funcs.cubic_to = [](const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *p)->int{
      std::vector<Vector3> *points = &reinterpret_cast<std::vector<std::vector<Vector3> >*>(p)->back();
      points->push_back(Vec3(to->x, to->y, 100.0f));
      return 0;
    };
    funcs.move_to = [](const FT_Vector *to, void *p)->int{
      std::vector<std::vector<Vector3> > *points = reinterpret_cast<std::vector<std::vector<Vector3> >*>(p);
      _.increase(*points).back().push_back(Vec3(to->x, to->y, 100.0f));
      return 0;
    };
    funcs.shift = 0;
    funcs.delta = 0;
    FT_Outline_Decompose(&outline, &funcs, &points);
    for (auto &as : points)
      for (auto &p : as)
        p.x += width_offset;

    std::vector<Vector2> flat;
    std::vector<SegmentIndex> edge;
    for (const auto &as : points){
      bool first = true;
      std::size_t befor;
      for (const auto &p : as){
        if (first)
        {
          SegmentIndex si = { flat.size() + as.size() - 2, flat.size() };
          edge.push_back(si);
          first = false;
        }
        else
        {
          Vector2 c;
          c.x = p.x;
          c.y = p.y;
          flat.push_back(c);
          shape->position.push_back(p);
          shape->position.push_back(p);
          shape->position.back().z *= -1;
          if (&p != &as.back())
          {
            SegmentIndex si = { befor, flat.size() };
            edge.push_back(si);
          }
        }
        befor = flat.size();
      }
    }

    uint16_t const n = flat.size();
    uint16_t i, j, k;
    for (i = 0; i < n - 2; ++i)
    {
      for (j = i + 1; j < n - 1; ++j)
      {
        for (k = j + 1; k < n; ++k)
        {
          bool on = false;
          for (const auto &e : edge){
            if ((e.a == i&&e.b == j) || (e.a == j&&e.b == k) || (e.a == k&&e.b == i)
              || (e.b == i&&e.a == j) || (e.b == j&&e.a == k) || (e.b == k&&e.a == i))
            {
              on = true;
            }
          }
          Vector2 pp[3] = { (flat[i] + flat[j]) / 2, (flat[k] + flat[j]) / 2, (flat[i] + flat[k]) / 2 };
          Vector2 pc[3] = { flat[k], flat[i], flat[j] };
          for (const auto &as : points){
            bool first = true;
            Vector3 befor;
            for (const auto &p : as){
              if (first)
              {
                first = false;
              }
              else
              {
                for (int i = 0; i < 3; i++)
                {
                  if (((befor.x - p.x) * (pp[i].y - befor.y) + (befor.y - p.y) * (befor.x - pp[i].x)) *
                    ((befor.x - p.x) * (pc[i].y - befor.y) + (befor.y - p.y) * (befor.x - pc[i].x)) < 0 &&
                    ((pp[i].x - pc[i].x) * (befor.y - pp[i].y) + (pp[i].y - pc[i].y) * (pp[i].x - befor.x)) *
                    ((pp[i].x - pc[i].x) * (p.y - pp[i].y) + (pp[i].y - pc[i].y) * (pp[i].x - p.x)) < 0)
                  {
                    on = true;
                  }
                }
              }
              befor = p;
              //float cross = center.x * (befor.y - p.y) + befor.x * (p.y - center.y) + p.x * (center.y - befor.y);
            }
          }
          if (!on)  continue;

          Vector2 o;
          float sqradius;
          OuterCircle(flat[i], flat[j], flat[k], &sqradius, &o);

          bool bad = false;
          for (int w = 0; w < n; ++w)
          {
            if (w == i)
              continue;
            if (w == j)
              continue;
            if (w == k)
              continue;

            if ((o - flat[w]).Sqlen() < sqradius)
            {
              bad = true;
              break;
            }

          }
          if (!bad)
          {
            int cp = 0;
            Vector2 center = (flat[i] + flat[j] + flat[k]) / 3;
            Vector2 left = center;
            left.x -= 10000;
            for (const auto &as : points){
              bool first = true;
              Vector3 befor;
              for (const auto &p : as){
                if (first)
                {
                  first = false;
                }
                else
                {
                  if (((befor.x - p.x) * (left.y - befor.y) + (befor.y - p.y) * (befor.x - left.x)) *
                    ((befor.x - p.x) * (center.y - befor.y) + (befor.y - p.y) * (befor.x - center.x)) < 0 &&
                    ((left.x - center.x) * (befor.y - left.y) + (left.y - center.y) * (left.x - befor.x)) *
                    ((left.x - center.x) * (p.y - left.y) + (left.y - center.y) * (left.x - p.x)) < 0)
                  {
                    cp++;
                  }
                }
                befor = p;
                //float cross = center.x * (befor.y - p.y) + befor.x * (p.y - center.y) + p.x * (center.y - befor.y);
              }
            }
            if ((cp % 2) == 1)
            {
              if (0 < Cross(shape->position[i * 2] - shape->position[j * 2], shape->position[i * 2] - shape->position[k * 2]).z)
              {
                _.push(idx).position = idx_offset+i * 2;
                _.push(idx).position = idx_offset+j * 2;
                _.push(idx).position = idx_offset+k * 2;
                _.push(idx).position = idx_offset+i * 2 + 1;
                _.push(idx).position = idx_offset+k * 2 + 1;
                _.push(idx).position = idx_offset+j * 2 + 1;
              }
              else
              {
                _.push(idx).position = idx_offset+i * 2;
                _.push(idx).position = idx_offset+k * 2;
                _.push(idx).position = idx_offset+j * 2;
                _.push(idx).position = idx_offset+i * 2 + 1;
                _.push(idx).position = idx_offset+j * 2 + 1;
                _.push(idx).position = idx_offset+k * 2 + 1;
              }
            }
          }
        }
      }
    }

    //cout << print(vi,"\n") << endl;

    for (auto as : points){
      const int offset = shape->position.size();
      int count = 0;
      const int mod = (as.size() - 1) * 2;
      for (auto p : as){
        _.push(idx).position = offset + (count * 2 + 0) % mod;
        _.push(idx).position = offset + (count * 2 + 2) % mod;
        _.push(idx).position = offset + (count * 2 + 1) % mod;
        _.push(idx).position = offset + (count * 2 + 2) % mod;
        _.push(idx).position = offset + (count * 2 + 3) % mod;
        _.push(idx).position = offset + (count * 2 + 1) % mod;
        shape->position.push_back(p);
        shape->position.push_back(p);
        shape->position.back().z *= -1;
        count++;
      }
    }

    FT_BBox bb;
    FT_Outline_Get_BBox(&outline, &bb);
    width_offset += (bb.xMax - bb.xMin) + 500;

  }
}

void FontmapImpl::Outline(float rad, float den)
{
  den /= rad*rad;
  for (int y = -rad - 1; y <= rad + 1; y++)
  {
    for (int x = -rad - 1; x <= rad + 1; x++)
    {
      float d = sqrtf(y*y + x*x) - rad;
      if (d < 0)
      {
        OutlineDensity o;
        o.x = x;
        o.y = y;
        o.density = -d*den;
        outline_density_.push_back(o);
      }
    }
  }
  outline_offset_ = { 0, 0, 0, 0 };
  for (int i = 0; i < outline_density_.size(); i++)
  {
    OutlineDensity &o = outline_density_[i];
    if (1 < o.density)
      o.density = 1;
    if (outline_offset_.x1 > o.x)
      outline_offset_.x1 = o.x;
    if (outline_offset_.x2<o.x)
      outline_offset_.x2 = o.x;
    if (outline_offset_.y1>o.y)
      outline_offset_.y1 = o.y;
    if (outline_offset_.y2 < o.y)
      outline_offset_.y2 = o.y;
  }
  this->outline_ = true;
  this->outline_rad_ = rad;
}

int FontmapImpl::GetWidth(wchar_t c)
{
  LoadChar(c);
  const FontChar &r = chars[c];
  return texture_width_*(r.x2 - r.x1);
}

int FontmapImpl::GetHeight(wchar_t c)
{
  LoadChar(c);
  const FontChar &r = chars[c];
  return texture_width_*(r.y2 - r.y1);
}

void FontmapImpl::LoadChar(wchar_t c)
{
  if (chars.end()==chars.find(c))
  {
    FT_Load_Char(face_, c, FT_LOAD_RENDER);
    FT_Bitmap &b = slot_->bitmap;

    if (outline_)
    {
      if (offset_x_ + b.width + 1 - outline_offset_.x1 + outline_offset_.x2 > texture_width_)
      {
        offset_x_ = 0;
        offset_y_ += max_rows_ + 1;
        max_rows_ = 0;
      }

      if (max_rows_ < b.rows - outline_offset_.y1 + outline_offset_.y2)
        max_rows_ = b.rows - outline_offset_.y1 + outline_offset_.y2;

      LockInfo li;
      tex->Lock(li, LOCK_READWRITE);
      OutlineDensity *po = outline_density_.data();
      for (int y = 0; y < 1 + b.rows - outline_offset_.y1 + outline_offset_.y2; y++)
      {
        for (int x = 0; x < 1 + b.width - outline_offset_.x1 + outline_offset_.x2; x++)
        {
          if (y&&x)
          {
            float sum = 0;
            for (int i = 0; i < outline_density_.size(); i++)
            {
              int cx = po[i].x + outline_offset_.x1 + x - 1;
              int cy = po[i].y + outline_offset_.y1 + y - 1;
              if (0 <= cx&&cx < b.width && 0 <= cy&&cy < b.rows)
              {
                sum += b.buffer[cy*b.pitch + cx] * po[i].density;
              }
            }
            if (sum > 255)
              sum = 255;
            li.Bits[(y + offset_y_)*li.Pitch + (x + offset_x_)] = (int)sum;
          }
          else
          {
            li.Bits[(y + offset_y_)*li.Pitch + (x + offset_x_)] = 0;
          }
        }
      }
      tex->Unlock();

      FontChar f;
      f.x1 = (0.5f + offset_x_) / texture_width_;
      f.x2 = (0.5f + offset_x_ + b.width - outline_offset_.x1 + outline_offset_.x2) / texture_width_;
      f.y1 = (0.5f + offset_y_) / texture_height_;
      f.y2 = (0.5f + offset_y_ + b.rows - outline_offset_.y1 + outline_offset_.y2) / texture_height_;
      f.baseline = slot_->bitmap_top - b.rows - outline_offset_.y2;

      offset_x_ += b.width + 1 - outline_offset_.x1 + outline_offset_.x2;
      chars[c] = f;
    }
    else
    {
      if (offset_x_ + b.width + 1 > texture_width_)
      {
        offset_x_ = 0;
        offset_y_ += max_rows_ + 1;
        max_rows_ = 0;
      }

      if (max_rows_ < b.rows)
        max_rows_ = b.rows;

      LockInfo li;
      tex->Lock(li, LOCK_READWRITE);
      for (int y = 0; y < b.rows + 1; y++)
      {
        for (int x = 0; x < b.width + 1; x++)
        {
          if (y&&x)
          {
            li.Bits[(y + offset_y_)*li.Pitch + (x + offset_x_)] = b.buffer[(y - 1)*b.pitch + (x - 1)];
          }
          else
          {
            li.Bits[(y + offset_y_)*li.Pitch + (x + offset_x_)] = 0;
          }
        }
      }
      tex->Unlock();

      FontChar f;
      f.x1 = (0.5f + offset_x_) / texture_width_;
      f.x2 = (0.5f + offset_x_ + b.width) / texture_width_;
      f.y1 = (0.5f + offset_y_) / texture_height_;
      f.y2 = (0.5f + offset_y_ + b.rows) / texture_height_;
      f.baseline = slot_->bitmap_top - b.rows;

      offset_x_ += b.width + 1;
      chars[c] = f;
    }
  }
}

Frect FontmapImpl::GetSize(const wchar_t *str, float padding)
{
  Frect f = { 0, 0, 0, 0 };
  float off_x = 0;
  float off_y = 0;
  bool cmd = false;
  float width = 0;
  for (const wchar_t *p = str; *p; p++)
  {
    if (cmd)
    {
      if (L'}' == *p)
      {
        cmd = false;
      }
      else if (L'#' == *p&&p[1] && p[2] && p[3])
      {
        p += 3;
      }
    }
    else if (L' ' == *p)
    {
      off_x += font_size_;
    }
    else if (L'{' == *p)
    {
      cmd = true;
    }
    else
    {
      LoadChar(*p);
      const FontChar &r = chars[*p];

      float w = texture_width_*(r.x2 - r.x1);
      float h = texture_height_*(r.y2 - r.y1) + r.baseline;

      off_x += outline_offset_.x1;
      if (width&&width < off_x + w - outline_offset_.x2)
      {
        off_y -= font_size_ * 2;
        off_x = 0;
      }
      off_x += w - outline_offset_.x2;

      if (f.y1 < h){
        f.y1 = h;
      }
      if (f.y2 > r.baseline)
      {
        f.y2 = r.baseline;
      }
      if (f.x2 < off_x)
      {
        f.x2 = off_x;
      }
      off_x += padding;
    }
  }
  return f;
}

static int hextoi(wchar_t c)
{
  if (L'0' <= c&&c <= L'9')
    return c - L'0';
  else if (L'a' <= c&&c <= L'f')
    return c - L'a' + 10;
  else if (L'A' <= c&&c <= L'F')
    return c - L'A' + 10;
  return 0;
}

void FontmapImpl::DrawUp(const Matrix &mat, const wchar_t *str, uint32_t col, bool force_color, float padding, int width)
{
  int count = 0;
  bool cmd = false;
  for (const wchar_t *p = str; *p; p++)
  {
    if (cmd)
    {
      if (L'}' == *p)
      {
        cmd = false;
      }
      else if (L'c' == *p&&p[1] && p[2] && p[3])
      {
        p += 3;
      }
    }
    else if (L' ' == *p)
    {
    }
    else if (L'{' == *p)
    {
      cmd = true;
    }
    else
    {
      LoadChar(*p);
      count++;
    }
  }

  if (up.size() < count * 6)
  {
    up.resize(count * 6);
  }
  int vc = 0;
  float off_x = 0;
  float off_y = 0;
  for (const wchar_t *p = str; *p; p++)
  {
    if (cmd)
    {
      if (L'}' == *p)
      {
        cmd = false;
      }
      else if (L'#' == *p&&p[1] && p[2] && p[3])
      {
        if (!force_color)
        {
          col = 0xff000000 | (hextoi(p[3]) * 0x11 << 16) |
            (hextoi(p[2]) * 0x11 << 8) | hextoi(p[1]) * 0x11;
        }
        p += 3;
      }
    }
    else if (L' ' == *p)
    {
      off_x += font_size_;
    }
    else if (L'{' == *p)
    {
      cmd = true;
    }
    else
    {
      const FontChar &r = chars[*p];

      float w = texture_width_*(r.x2 - r.x1);
      float h = texture_height_*(r.y2 - r.y1) + r.baseline;

      off_x += outline_offset_.x1;
      if (width&&width < off_x + w - outline_offset_.x2)
      {
        off_y -= font_size_ * 2;
        off_x = 0;
      }

      up[vc].position = Vec3(off_x, off_y + r.baseline, 0);
      up[vc].u = r.x1;
      up[vc].v = r.y2;
      up[vc].diffuse = col;
      vc++;
      up[vc].position = Vec3(off_x, off_y + h, 0);
      up[vc].u = r.x1;
      up[vc].v = r.y1;
      up[vc].diffuse = col;
      vc++;
      up[vc].position = Vec3(off_x + w, off_y + r.baseline, 0);
      up[vc].u = r.x2;
      up[vc].v = r.y2;
      up[vc].diffuse = col;
      vc++;
      up[vc].position = Vec3(off_x + w, off_y + r.baseline, 0);
      up[vc].u = r.x2;
      up[vc].v = r.y2;
      up[vc].diffuse = col;
      vc++;
      up[vc].position = Vec3(off_x, off_y + h, 0);
      up[vc].u = r.x1;
      up[vc].v = r.y1;
      up[vc].diffuse = col;
      vc++;
      up[vc].position = Vec3(off_x + w, off_y + h, 0);
      up[vc].u = r.x2;
      up[vc].v = r.y1;
      up[vc].diffuse = col;
      vc++;
      off_x += w - outline_offset_.x2 + padding;
    }
  }

  world->Transform(mat);

  dev->SetShader(world, 0);
  dev->SetShader(decal, 1);
  dev->SetShader(shader::ShaderEnd(), 2);

  dev->SetRenderState(RENDER_ZWRITE, VALUE_DISABLE);
  dev->SetAlphaBlendMode(ALPHABLEND_MODULATE);
  dev->SetRenderState(RENDER_CULLMODE, CULL_NONE);

  dev->DrawPrimitiveUP(PRIMITIVE_TRIANGLELIST, vc / 3, up.data(), Vertex::format);
  dev->SetAlphaBlendMode(ALPHABLEND_NONE);
  dev->SetRenderState(RENDER_ZWRITE, VALUE_ENABLE);
  dev->SetRenderState(RENDER_CULLMODE, CULL_CCW);
}

