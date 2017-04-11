#include "ParticleRenderer.h"
#include <math.h> 
#include "Enemy.h" // FOR TESTING
#define SHADERS 30
#define DIVIDE 8 // should be divisible by 2
#define START_SIZE 8096  // should be divisible by 2

using namespace DirectX::SimpleMath;

ParticleRenderer::ParticleRenderer(CameraClass *camera, Enemy *enemy) 
	: mCamera(camera) {
	this->mGraphicsData = new GraphicsData();
	frame = 0;
	this->enemy = enemy;
}

ParticleRenderer::~ParticleRenderer() {
	delete this->mGraphicsData;
}

void ParticleRenderer::setup(ID3D11Device *device, ShaderHandler &shaders) {
	D3D11_INPUT_ELEMENT_DESC desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "DIMENSIONS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	shaders.setupVertexShader(device, SHADERS, L"ParticleVS.hlsl", "main", desc, ARRAYSIZE(desc));
	shaders.setupPixelShader(device, SHADERS, L"ParticlePS.hlsl", "main");
	shaders.setupGeometryShader(device, SHADERS, L"ParticleGS.hlsl", "main");

	for (int i = 0; i < START_SIZE; i++) {
		addRandomParticle();
	}

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &this->mParticleVertices[0];

	mGraphicsData->createVertexBuffer(0, getSize(), &data, device, true);
	mGraphicsData->createConstantBuffer(1, sizeof(Vector4), nullptr, device, true);
	mGraphicsData->loadTexture(0, L"../Resource/Textures/sand.png", device);
	mGraphicsData->loadTexture(1, L"../Resource/Textures/enemy.png", device);
}

void ParticleRenderer::updateCameraBuffer(ID3D11DeviceContext *context) {
	Vector4 cam = this->mCamera->getPos();

	D3D11_MAPPED_SUBRESOURCE res;
	context->Map(this->mGraphicsData->getBuffer(1), 0, D3D11_MAP_WRITE_DISCARD, NULL, &res);
	memcpy(res.pData, &cam, sizeof(Vector4));
	context->Unmap(this->mGraphicsData->getBuffer(1), 0);
}

void ParticleRenderer::updateParticles(ID3D11DeviceContext *context) {
	frame++;
	float temp = 0;
	int mod = frame % DIVIDE;
	int piece = mParticleVertices.size() / DIVIDE;
	int start = piece * mod;

	for (int i = start; i < start + piece; i++) {
		ParticleVertex *particle = &this->mParticleVertices[i];
		ParticleData *data = &this->mParticleData[i];

		// TEMP
		particle->position += data->direction / 1000.f;
		if (rand() % 1000 == 0) {
			temp = rand() % 4 - 1;
			data->direction = Vector3(temp, temp, temp);
		}
	}

	/* TEEST TETSTTETET REMOVE BEFORE MERGING */
	ParticleVertex *vertex = &this->mParticleVertices[mParticleVertices.size() - 1];
	ParticleData *data = &this->mParticleData[mParticleVertices.size() - 1];
	vertex->dimensions.x = 2.f;
	vertex->dimensions.y = 4.f;
	vertex->position = enemy->getPosition();
	/* TEEST TETSTTETET */


	timeCheck(start, piece);

	D3D11_MAPPED_SUBRESOURCE res;
	context->Map(this->mGraphicsData->getBuffer(0), 0, D3D11_MAP_WRITE_DISCARD, NULL, &res);
	char *ptr = static_cast<char*> (res.pData);
	memcpy(ptr, &this->mParticleVertices[0], getSize());
	context->Unmap(this->mGraphicsData->getBuffer(0), 0);
}

void ParticleRenderer::render(ID3D11DeviceContext *context, ShaderHandler &shaders) {
	UINT stride = sizeof(ParticleVertex), offset = 0;
	ID3D11Buffer *buffer = this->mGraphicsData->getBuffer(0),
						   *cam = this->mGraphicsData->getBuffer(1),
						   *vp = this->mCamera->getMatrixBuffer();
	ID3D11ShaderResourceView *srv = this->mGraphicsData->getSRV(0);
	ID3D11ShaderResourceView *srv2 = this->mGraphicsData->getSRV(1);
	shaders.setShaders(context, SHADERS, 20, SHADERS); //20 is from entity shader, change later

	updateCameraBuffer(context);
	updateParticles(context);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->IASetInputLayout(shaders.getInputLayout(SHADERS));
	context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
	context->PSSetShaderResources(0, 1, &srv);
	context->GSSetConstantBuffers(0, 1, &vp);
	context->GSSetConstantBuffers(1, 1, &cam);

	context->Draw(this->mParticleVertices.size() - 1, 0);
	context->PSSetShaderResources(0, 1, &srv2);
	context->Draw(1, this->mParticleVertices.size() - 1);
}

UINT ParticleRenderer::getSize() const {
	return this->mParticleVertices.size() * sizeof(ParticleVertex);
}

void ParticleRenderer::addRandomParticle() {
	ParticleVertex particle;
	ParticleData partData;

	//TEMP
	particle.position = Vector3(
		(rand() % 200) / 10.f - 10,
		(rand() % 250) / 10.f,
		(rand() % 200) / 10.f - 10
	);
	particle.dimensions = Vector2(0.01f, 0.01f);

	partData.direction = Vector3(rand() % 4 - 1, rand() % 4 - 1, rand() % 4 - 1);
	partData.timeLeft = rand() % 100 + 100;

	this->mParticleVertices.push_back(particle);
	this->mParticleData.push_back(partData);
}

void ParticleRenderer::timeCheck(int start, int piece) {
	for (int i = start; i < start + piece; i++) {
		ParticleVertex *particle = &this->mParticleVertices[i];
		ParticleData *data = &this->mParticleData[i];

		data->timeLeft -= 0.0001f;
		if (data->timeLeft <= 0) {
			addRandomParticle();
			std::swap(this->mParticleData[i], this->mParticleData[this->mParticleData.size() - 1]);
			std::swap(this->mParticleVertices[i], this->mParticleVertices[this->mParticleVertices.size() - 1]);
			this->mParticleData.pop_back();
			this->mParticleVertices.pop_back();
		}
	}
}