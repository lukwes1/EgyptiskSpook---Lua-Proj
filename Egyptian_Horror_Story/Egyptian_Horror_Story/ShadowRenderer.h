#ifndef SHADOWRENDERER_H
#define SHADOWRENDERER_H

#include "Direct3DHeader.h"
#include "ShaderHandler.h"
#include "Light.h"
#include "Renderer.h"

class ShadowRenderer : public Renderer
{
private:
	D3D11_VIEWPORT mViewport;

	ID3D11DepthStencilView* mDSV;
	ID3D11ShaderResourceView* mSRV;
	
	Light* mLight;
public:
	ShadowRenderer(Light* light);
	virtual ~ShadowRenderer();

	void setup(ID3D11Device* device, ShaderHandler& shaders);
	void render(ID3D11DeviceContext* context, ShaderHandler& shaders);
};

#endif