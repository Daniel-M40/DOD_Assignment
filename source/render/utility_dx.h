// Utility functions for working with DirectX

#ifndef MSC_UTILITY_DX_H_INCLUDED
#define MSC_UTILITY_DX_H_INCLUDED

#include <dxgi.h>
#include <stdint.h>

namespace msc
{
	// Return size of given DirectX format
	uint32_t SizeOfDXGIFormat(DXGI_FORMAT format);
} // namespaces

#endif // Header guard
