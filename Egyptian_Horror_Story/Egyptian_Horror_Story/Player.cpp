#include "Player.h"
#include <SDL.h>

#define SPEED 15.f;

#define GRAVITY 2.5f // Gravity per second
#define GROUND_Y 0.f // Ground position
#define JUMP_START_VELOCITY 0.95f // Start velocity after jumping, reduced by GRAVITY after a second (lerping)

#define MAX_STAMINA 15.f // Max Stamina
#define SPRINT_MULTIPLIER 2.f // Multiplier for sprinting
#define TIRED_MULTIPLIER 0.6f // Multiplier after running out of stamina
#define START_STAMINA 3.f // Bad name, but basicly meaning how much stamina is needed to start sprinting
#define STAMINA_LOSS -4.5f // Loss per second sprinting
#define STAMINA_GAIN 0.5f // Gained per second not sprinting

#define SNEAK_MULTIPLIER 0.35f // Multiplier for sneaking
#define SNEAK_Y -4.f // Camera change while sneaking
#define SNEAK_TIME 0.2f //Time to go from standing to sneaking and vice versa

using namespace DirectX::SimpleMath;

Player::Player(CameraClass* camera, ID3D11Device* device, ID3D11DeviceContext* context, int key, GraphicsData* gData)
	:Entity(key)
{
	this->mCamera = camera;

	// movement
	this->mSneaking = false;
	this->mSprinting = false;
	this->mSneakTime = 0;

	this->mMaxStamina = MAX_STAMINA;
	this->mSpeed = SPEED;
	this->mStamina = this->mMaxStamina;

	//REMOVE
	this->col = new Capsule(this->mCamera->getPos(), 2, 1);

	// jumping stuff
	this->mJumping = false;
	this->mJumpingVelocity = 0;

	this->mLight = new Light(this->mCamera->getPos(), this->mCamera->getForward(), device, context, gData);
}

Player::~Player()
{
	if (this->mLight)
	delete this->mLight;

	delete this->col;
}

void Player::updatePosition(float dt)
{
	this->mPrevPos = this->getPosition();
	computeVelocity();
	handleJumping(dt);
	handleSprinting(dt);

	DirectX::SimpleMath::Vector3 newPos = this->getPosition() + this->mVelocity * mSpeed * getMovementMultiplier() * dt;
	setPosition(newPos);

	newPos.y += handleSneaking(dt);
	
	this->mCamera->setPos(newPos);
	updateLightPosition();
	
	//ASDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD
	this->col->mPoint = this->mCamera->getPos();
}

void Player::handleJumping(float dt) {
	this->mVelocity.y = 0;
	this->mVelocity.Normalize(); // Norm to make speed forward speed same if you look up or down
	if (this->mJumping) {
		this->mJumpingVelocity -= GRAVITY * dt;
		this->mVelocity.y = mJumpingVelocity;

		if (getPosition().y + this->mVelocity.y * mSpeed * dt <= GROUND_Y) {
			// set position to ground y
			DirectX::SimpleMath::Vector3 newPos = getPosition();
			newPos.y = GROUND_Y;
			setPosition(newPos);

			// reset velocity
			this->mVelocity.y = 0;
			this->mJumping = false;
			this->mVelocity.Normalize();
		}
	}
}

bool Player::handleKeyboardPress(SDL_KeyboardEvent const &key)
{
	switch (key.keysym.scancode) {
		case SDL_SCANCODE_A:
			this->mDirection.x = -1;
			break;
		case SDL_SCANCODE_D:
			this->mDirection.x = 1;
			break;
		case SDL_SCANCODE_W:
			this->mDirection.y = 1;
			break;
		case SDL_SCANCODE_S:
			this->mDirection.y = -1;
			this->mSprinting = false;
			break;
		case SDL_SCANCODE_SPACE:
			if (!this->mJumping && getMovementMultiplier() == 1.f) {
				this->mJumping = true;
				this->mJumpingVelocity = JUMP_START_VELOCITY;
			}
			break;
		case SDL_SCANCODE_LSHIFT:
			startSprint();
			break;
		case SDL_SCANCODE_LCTRL:
			startSneaking();
			break;
	}

	return true;
}

bool Player::handleKeyboardRelease(SDL_KeyboardEvent const &key)
{
	switch (key.keysym.scancode) {
		case SDL_SCANCODE_Q:
			SDL_Log("x = %f, y = %f, z = %f", getPosition().x, getPosition().y, getPosition().z); // TESTING METHOD
			break;
		case SDL_SCANCODE_A:
			if (this->mDirection.x == -1)
				this->mDirection.x = 0;
			break;
		case SDL_SCANCODE_D:
			if (this->mDirection.x == 1)
				this->mDirection.x = 0;
			break;
		case SDL_SCANCODE_W:
			if (this->mDirection.y == 1)
				this->mDirection.y = 0;
			break;
		case SDL_SCANCODE_S:
			if (this->mDirection.y == -1)
				this->mDirection.y = 0;
			break;
		case SDL_SCANCODE_LSHIFT:
			this->mSprinting = false;
			break;
		case SDL_SCANCODE_LCTRL:
			this->mSneaking = false;
			break;
	}

	return true;
}

void Player::handleMouseMotion(SDL_MouseMotionEvent const &motion)
{
	if (motion.xrel != 0)
	{
		this->mCamera->setYaw(this->mCamera->getYaw() + motion.xrel);
	}

	if (motion.yrel != 0)
	{
		this->mCamera->setPitch(this->mCamera->getPitch() - motion.yrel);
	}
}

Light* Player::getLight()
{
	return this->mLight;
}

CameraClass* Player::getCamera()
{
	return this->mCamera;
}

void Player::setPosition(DirectX::SimpleMath::Vector3 pos)
{
	Entity::setPosition(pos);
	this->mCamera->setPos(pos);
}

DirectX::SimpleMath::Vector3 Player::getPrevPos() const
{
	return this->mPrevPos;
}

DirectX::SimpleMath::Vector3 Player::getVelocity() const
{
	return this->mVelocity;
}

void Player::setPrevPos(DirectX::SimpleMath::Vector3 pos)
{
	this->mPrevPos = pos;
}

// private
void Player::updateLightPosition() {
	DirectX::SimpleMath::Vector3 offset;

	offset += this->mCamera->getRight() * 0.7f;
	offset += this->mCamera->getUp() * -1.f;

	this->mLight->update(this->mCamera->getPos(), offset, this->mCamera->getForward());
}

void Player::computeVelocity() {
	Vector3 normal = Vector3(0, 1, 0); //Normal of plane, shouldn't change
	Vector3 forward = this->mCamera->getForward();

	Vector3 proj = forward - normal * (forward.Dot(normal) / normal.LengthSquared()); // Project forward vector on plane
	proj.Normalize();

	this->mVelocity = this->mDirection.x * this->mCamera->getRight();
	this->mVelocity += this->mDirection.y * proj;
}

void Player::handleSprinting(float dt) {
	if (this->mSprinting) {
		this->mStamina += STAMINA_LOSS * dt; //change later
		if (this->mStamina <= 0) {
			this->mStamina = 0;
			this->mSprinting = false;
		}
	}
	else if (mStamina < MAX_STAMINA) {
		this->mStamina += STAMINA_GAIN * dt; //change later
		if (this->mStamina > MAX_STAMINA)
			this->mStamina = MAX_STAMINA;
	}
}

float Player::handleSneaking(float dt) {
	if (this->mSneaking) {
		this->mSneakTime += dt / SNEAK_TIME;
		if (this->mSneakTime >= 1) this->mSneakTime = 1;
	}
	else if (this->mSneakTime > 0) {
		this->mSneakTime -= dt / SNEAK_TIME;
		if (this->mSneakTime <= 0) this->mSneakTime = 0;
	}

	return SNEAK_Y * (SNEAK_TIME * this->mSneakTime);
}

void Player::startSprint() {
	if (!this->mSprinting && this->mDirection.y != -1 && //no backsies
		this->mStamina > START_STAMINA && !this->mSneaking && !this->mJumping) {
		this->mSprinting = true;
		this->mStamina -= START_STAMINA;
	}
}

void inline Player::startSneaking() {
	if (!this->mSprinting)
		this->mSneaking = true;
}

float inline Player::getMovementMultiplier() {
	if (this->mSneaking) {
		return SNEAK_MULTIPLIER;
	} else if (this->mSprinting) {
		return SPRINT_MULTIPLIER;
	} else if (this->mStamina < START_STAMINA) {
		return TIRED_MULTIPLIER;
	}
	
	return 1.f;
}

void Player::damage() {
	// todo
}