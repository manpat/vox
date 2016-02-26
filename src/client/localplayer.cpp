#include "clientnetinterface.h"
#include "chunkmanager.h"
#include "localplayer.h"
#include "camera.h"
#include "chunk.h"
#include "input.h"
#include "block.h"

static Log logger{"LocalPlayer"};

// TODO: This needs to take player orientation into account
struct PlayerMotionState : public btMotionState {
	Camera* cam;
	vec3 cameraOffset {0, PlayerBase::PlayerHeight/2.f, 0};

	PlayerMotionState(Camera* c) : cam{c} {}

	void getWorldTransform(btTransform& worldTrans) const override {
		worldTrans.setIdentity();
		worldTrans.setOrigin(o2bt(cam->position - cameraOffset));
	}
	void setWorldTransform(const btTransform& newTrans) override {
		auto pos = bt2o(newTrans.getOrigin());
		cam->position = pos + cameraOffset;
	}
};

LocalPlayer::LocalPlayer(std::shared_ptr<Camera> c): camera{c}, noclip{false} {
	auto ms = new PlayerMotionState{camera.get()};
	collider = new btCapsuleShape{.5f, PlayerBase::PlayerHeight - 1.f};

	btScalar mass = 10.;
	btVector3 inertia {0,0,0};
	collider->calculateLocalInertia(mass, inertia);

	RigidBodyInfo bodyInfo{mass, ms, collider, inertia};
	rigidbody = new RigidBody{bodyInfo};
	Physics::world->addRigidBody(rigidbody);
	
	rigidbody->setAngularFactor(0.f);
	rigidbody->setActivationState(DISABLE_DEACTIVATION);
	rigidbody->setFriction(0);
}

LocalPlayer::~LocalPlayer() {}

void LocalPlayer::Update() {
	if(Input::GetKeyDown(SDLK_g)){
		SetNoclip(!noclip);
	}

	// TODO: Move all this shit out into a PlayerState
	auto nyaw = Input::GetMouseDelta().x * 2.0 * PI * Time::dt * 7.f;
	auto npitch = -Input::GetMouseDelta().y * 2.0 * PI * Time::dt * 7.f;
	constexpr f32 limit = PI/2.f;

	camRot.x = clamp(camRot.x + npitch, -limit, limit);
	camRot.y += nyaw;

	vec4 inputDir {
		Input::GetMapped(Input::Right) - Input::GetMapped(Input::Left), 0, 
		Input::GetMapped(Input::Backward) - Input::GetMapped(Input::Forward), 0
	};

	vec3 prevVelocity = bt2o(rigidbody->getLinearVelocity());
	vec3 velocity {0.f};
	quat forwardRot {1,0,0,0};
	if(noclip){
		forwardRot = glm::angleAxis(-camRot.y, vec3{0,1,0});
		quat rot = forwardRot * glm::angleAxis(-camRot.x, vec3{1,0,0});

		inputDir = rot * inputDir;
		velocity = vec3(inputDir) * 10.f * (1.f + Input::GetMapped(Input::Boost)*6.f);
		prevVelocity = velocity;
		camera->position += velocity * Time::dt;
		camera->rotation = rot;
	}else{
		forwardRot = glm::angleAxis(-camRot.y, vec3{0,1,0});

		camera->rotation = forwardRot * glm::angleAxis(-camRot.x, vec3{1,0,0});
		inputDir = forwardRot * inputDir;

		velocity = vec3(inputDir);
		velocity = velocity * 6.f * (1.f + Input::GetMapped(Input::Boost)*3.f);
		velocity.y = prevVelocity.y;

		if(Input::GetMappedDown(Input::Jump))
			velocity.y += 10.f;

		rigidbody->setLinearVelocity(o2bt(velocity));
	}
	
	// TODO: Limit send rate - probably doesn't need to be sent every 16ms
	auto eyeRot = glm::inverse(camera->rotation);
	auto bodyPos = camera->position - vec3{0,PlayerHeight,0} * forwardRot;
	ClientNetInterface::UpdatePlayerState(bodyPos, prevVelocity, forwardRot, eyeRot);

	camera->UpdateMatrices();

	if(Input::GetKeyDown('1')) blockType = 1; 
	if(Input::GetKeyDown('2')) blockType = 2;
	if(Input::GetKeyDown('3')) blockType = 3;
	if(Input::GetKeyDown('4')) blockType = 4;
	if(Input::GetKeyDown('5')) blockType = 5;
	if(Input::GetKeyDown('6')) blockType = 6;
	if(Input::GetKeyDown('7')) blockType = 7;
	if(Input::GetKeyDown('8')) blockType = 8;
	if(Input::GetKeyDown('9')) blockType = 9;

	if(Input::GetKeyDown('0')) blockType = 0;

	if(Input::GetKeyDown('[')) blockRot = (blockRot-1)&3;
	if(Input::GetKeyDown(']')) blockRot = (blockRot+1)&3;

	if(Input::GetButtonDown(Input::MouseLeft) || Input::GetButtonDown(Input::MouseRight)) {
		auto raycastResult = Physics::Raycast(
			camera->position + camera->forward*0.3f,
			camera->position + camera->forward*10.f);

		if(raycastResult.hit){
			auto col = raycastResult.rigidbody->getCollisionShape();
			// TODO: Change this
			// It will explode as soon as we start using the user pointers for other things
			if(auto chnk = (Chunk*)col->getUserPointer()) {
				auto normal = raycastResult.normal;

				if(Input::GetButtonDown(Input::MouseRight) || !blockType)
					normal = -normal;

				auto chpos = raycastResult.position + normal * 0.1f;
				auto vxpos = chnk->WorldToVoxelSpace(chpos);

				if(!blockType) {
					auto blk = chnk->GetBlock(vxpos);
					if(blk && blk->AsDynamic()) {
						ClientNetInterface::DoInteract(chnk->chunkID, vxpos);
					}
				}else{
					if(Input::GetButtonDown(Input::MouseRight)){
						ClientNetInterface::SetBlock(chnk->chunkID, vxpos, 0, 0);
					}else{
						// If raycast normal is perpendicular to the up of the 
						//	chunk, rotate such that player look direction is block north
						// Otherwise, rotate based on normal such that
						//	 the north faces -normal
						
						auto chnkUp = chnk->rotation * vec3{0,1,0};
						auto chnkRgt = chnk->rotation * vec3{1,0,0};
						auto chnkFwd = chnk->rotation * vec3{0,0,-1};
						auto plyFwd = camera->rotation * vec3{0,0,-1};

						bool upPerpendicular = glm::abs(glm::dot(normal, chnkUp)) > 0.707f;

						auto chu = glm::dot(chnkFwd, upPerpendicular?plyFwd:-normal);
						auto chv = glm::dot(chnkRgt, upPerpendicular?plyFwd:-normal);

						u8 blkRot = 0;
						if(glm::abs(chu) > glm::abs(chv)) {
							blkRot = (chu <= 0)<<1;
						}else{
							blkRot = ((chv <= 0)<<1) + 1;
						}

						ClientNetInterface::SetBlock(chnk->chunkID, vxpos, blockType, (blkRot + blockRot)&3);
					}
				}

			}else{
				logger << col;
			}
		}
	}
}

void LocalPlayer::SetNoclip(bool n) {
	noclip = n;

	Physics::world->removeRigidBody(rigidbody);

	auto flags = rigidbody->getCollisionFlags();
	if(noclip){
		flags |= btCollisionObject::CF_KINEMATIC_OBJECT;
	}else{
		flags &= ~btCollisionObject::CF_KINEMATIC_OBJECT;
	}
	rigidbody->setCollisionFlags(flags);

	Physics::world->addRigidBody(rigidbody);
}

