#include "shader.h"

namespace gh {
  namespace shader {
    Shader::~Shader()
    {
    }

    Shader* ShaderEnd()
    {
      return (Shader*)ShaderEnd;
    }
    Shader* ShaderIgnore()
    {
      return (Shader*)ShaderIgnore;
    }
  }
}