#include "StelRenderer.hpp"
#include "StelTextureNew.hpp"

StelTextureNew::~StelTextureNew()
{
	renderer->destroyTextureBackend(backend);
}

void StelTextureNew::bind(const int textureUnit)
{
	renderer->bindTextureBackend(backend, textureUnit);
}
