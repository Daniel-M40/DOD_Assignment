/*
 *  Shader class - loading and selection
 */

#include "shader_dx.h"
#include <fstream>

namespace msc {

//-------------------------------------------------------------------------------------------------
// Construction
//-------------------------------------------------------------------------------------------------

// Constructor reads the bytecode and creates shader object. Empty name creates null shader (disable stage)
ShaderDX::ShaderDX(ID3D11Device* device, ShaderType type, std::string name) : mType(type)
{
  if (name != "")
  {
    // Open compiled shader object file
    std::ifstream shaderFile(name + ".cso", std::ios::in|std::ios::binary|std::ios::ate); 
	  if (!shaderFile.is_open())  throw std::runtime_error("Error opening shader: " + name);

    // Read file
	  std::streamoff fileSize = shaderFile.tellg();
	  shaderFile.seekg(0, std::ios::beg);
    mByteCode.resize(static_cast<decltype(mByteCode.size())>(fileSize));
	  shaderFile.read(&mByteCode[0], fileSize);
    if (shaderFile.fail())  throw std::runtime_error("Error reading shader: " + name);

    // Create object of appropriate type
    HRESULT err = E_INVALIDARG;
    if      (mType == ShaderType::Pixel   )  err = device->CreatePixelShader   (ByteCode(), ByteCodeLength(), nullptr, &mPSPtr);
    else if (mType == ShaderType::Vertex  )  err = device->CreateVertexShader  (ByteCode(), ByteCodeLength(), nullptr, &mVSPtr);
    else if (mType == ShaderType::Geometry)  err = device->CreateGeometryShader(ByteCode(), ByteCodeLength(), nullptr, &mGSPtr);
    else if (mType == ShaderType::Domain  )  err = device->CreateDomainShader  (ByteCode(), ByteCodeLength(), nullptr, &mDSPtr);
    else if (mType == ShaderType::Hull    )  err = device->CreateHullShader    (ByteCode(), ByteCodeLength(), nullptr, &mHSPtr);
    else if (mType == ShaderType::Compute )  err = device->CreateComputeShader (ByteCode(), ByteCodeLength(), nullptr, &mCSPtr);
    if (FAILED(err))  throw std::runtime_error("Error creating shader from: " + name);
  }
}

ShaderDX::~ShaderDX()
{
  // Not using a CComPtr in this class (due to the use of a union, see header), so release explicitly
  if      (mType == ShaderType::Pixel   )  mPSPtr->Release();
  else if (mType == ShaderType::Vertex  )  mVSPtr->Release();
  else if (mType == ShaderType::Geometry)  mGSPtr->Release();
  else if (mType == ShaderType::Domain  )  mDSPtr->Release();
  else if (mType == ShaderType::Hull    )  mHSPtr->Release();
  else if (mType == ShaderType::Compute )  mCSPtr->Release();
}


//-------------------------------------------------------------------------------------------------
// Usage
//-------------------------------------------------------------------------------------------------

// Select this shader on the given context
void ShaderDX::Set(ID3D11DeviceContext* context)
{
  if      (mType == ShaderType::Pixel   )  context->PSSetShader(mPSPtr, nullptr, 0);
  else if (mType == ShaderType::Vertex  )  context->VSSetShader(mVSPtr, nullptr, 0);
  else if (mType == ShaderType::Geometry)  context->GSSetShader(mGSPtr, nullptr, 0);
  else if (mType == ShaderType::Domain  )  context->DSSetShader(mDSPtr, nullptr, 0);
  else if (mType == ShaderType::Hull    )  context->HSSetShader(mHSPtr, nullptr, 0);
  else if (mType == ShaderType::Compute )  context->CSSetShader(mCSPtr, nullptr, 0);
}

// Static method to disable the given shader stage on the given context
void ShaderDX::Disable(ID3D11DeviceContext* context, ShaderType type)
{
  if      (type == ShaderType::Pixel   )  context->PSSetShader(nullptr, nullptr, 0);
  else if (type == ShaderType::Vertex  )  context->VSSetShader(nullptr, nullptr, 0);
  else if (type == ShaderType::Geometry)  context->GSSetShader(nullptr, nullptr, 0);
  else if (type == ShaderType::Domain  )  context->DSSetShader(nullptr, nullptr, 0);
  else if (type == ShaderType::Hull    )  context->HSSetShader(nullptr, nullptr, 0);
  else if (type == ShaderType::Compute )  context->CSSetShader(nullptr, nullptr, 0);
}


} // namespaces