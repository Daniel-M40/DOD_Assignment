/** @file
 *  Shader class - loading and selection
 */

#ifndef MSC_SHADER_DX_H_INCLUDED
#define MSC_SHADER_DX_H_INCLUDED

#include <d3d11.h>
#include <atlbase.h>
#include <string>
#include <vector>

namespace msc {

enum class ShaderType
{
  Vertex,
  Hull,
  Domain,
  Geometry,
  Pixel,
  Compute
};

class ShaderDX
{
  //-------------------------------------------------------------------------------------------------
  // Construction
  //-------------------------------------------------------------------------------------------------
public:
  ShaderDX(ID3D11Device* device, ShaderType type, std::string name);
  ~ShaderDX();

  // Prevent copy/move/assignment
  ShaderDX(const ShaderDX&) = delete;
  ShaderDX(ShaderDX&&) = delete;
  ShaderDX& operator=(const ShaderDX&) = delete;
  ShaderDX& operator=(ShaderDX&&) = delete;


  //-------------------------------------------------------------------------------------------------
  // Public interface
  //-------------------------------------------------------------------------------------------------
public:
  // Only exposing these private details because of an annoying requirement to provide them when
  // creating vertex layouts (which is done elsewhere)
  UINT        ByteCodeLength() const { return static_cast<UINT>(mByteCode.size()); }
  const void* ByteCode()       const { return mByteCode.data(); }

  // Select this shader on the given context
  void Set(ID3D11DeviceContext* context);

  // Static method to disable the given shader stage on the given context
  static void Disable(ID3D11DeviceContext* context, ShaderType type);


  //-------------------------------------------------------------------------------------------------
  // Data
  //-------------------------------------------------------------------------------------------------
private:
  ShaderType mType;

  // Compiled shader, to be converted to shader object
  std::vector<char> mByteCode;

  // Single pointer to the DirectX shader object. Possible to implement different shader types
  // using polymorphism, but becomes unnecessary code bloat imparing the simplicity of the class
  union {
    ID3D11VertexShader*   mVSPtr = nullptr;
    ID3D11HullShader*     mHSPtr;
    ID3D11DomainShader*   mDSPtr;
    ID3D11GeometryShader* mGSPtr;
    ID3D11PixelShader*    mPSPtr;
    ID3D11ComputeShader*  mCSPtr;
  };
};


} // namespaces

#endif // Header guard
