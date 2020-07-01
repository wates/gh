#ifndef SHAPE_H_INCLUDED
#define SHAPE_H_INCLUDED

#include <vector>
#include <string>
#include "matrix.h"
#include "color.h"

namespace gh{
  
  typedef unsigned short IndexType;

  struct IndexVertex
  {
    IndexType position;
    IndexType normal;
    IndexType color;
    IndexType texture;
  };

  struct Material
  {
    Color ambient;
    Color diffuse;
    Color specular;
    float power;
    std::string texture;
  };

  struct Subset
  {
    std::vector<IndexVertex> indices;
    Material material;
  };

  struct WeightIndex
  {
    IndexType index;
    float weight;
  };

  struct Weight
  {
    std::string bonename;//deformer
    Matrix offset;//basepose
    std::vector<WeightIndex> position;
    std::vector<WeightIndex> normal;
  };

  struct Morphing {
    struct Target {
      uint16_t index;
      Vector3 position;
      //Vector3 normal;
    };
    std::string name;
    std::vector<Target> target;
  };

  struct Shape
  {
    std::vector<Vector3> position;
    std::vector<Vector3> normal;
    std::vector<Vector3> tangent;
    std::vector<Color> color;
    std::vector<Vector2> texture;
    std::vector<Subset> subset;
    std::vector<Weight> weight;
    std::vector<Morphing> morphing;

    void Normalize(float scale = 1.0f);
    void SetNormal();
    void ReducePosition(float threshold);
    void MakeTangent();
    void UVScale(float scale);
    void ReverseCulling();
    void Centering();
    void CenteringX();
    void Scale(float x);
    float Magnitude()const;
  };

}

#endif