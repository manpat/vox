#include "clientnetinterface.h"
#include "chunkmanager.h"
#include "localplayer.h"
#include "voxelchunk.h"
#include "camera.h"
#include "input.h"
#include "block.h"

static Log logger{"LocalPlayer"};

struct PlayerMotionState : public btMotionState {
	Camera* cam;
	vec3 cameraOffset {0, LocalPlayer::Height/2.f, 0};

	PlayerMotionState(Camera* c) : cam{c} {}

	void getWorldTransform(btTransform& worldTrans) const override {
		worldTrans.setIdentity();
		worldTrans.setOrigin(o2bt(cam->position - cameraOffset));
	}
	void setWorldTransform(const btTransform& newTrans) override {
		auto pos = bt2o(newTrans.getOrigin());
		cam->position = pos + cameraOffset;

		ClientNetInterface::UpdatePlayerState(pos, vec3{0}, quat{0,0,0,1});
	}
};

LocalPlayer::LocalPlayer(std::shared_ptr<Camera> c): camera{c}, noclip{false} {
	auto ms = new PlayerMotionState{camera.get()};
	collider = new btCapsuleShape{.5f, LocalPlayer::Height - 1.f};

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

	if(noclip){
		quat rot;
		rot = glm::angleAxis(-camRot.y, vec3{0,1,0});
		rot = rot * glm::angleAxis(-camRot.x, vec3{1,0,0});

		inputDir = rot * inputDir;
		camera->position += vec3(inputDir) * Time::dt * 6.f * (1.f + Input::GetMapped(Input::Boost)*3.f);
		camera->rotation = rot;
	}else{
		quat rot;
		rot = glm::angleAxis(-camRot.y, vec3{0,1,0});

		camera->rotation = rot * glm::angleAxis(-camRot.x, vec3{1,0,0});
		inputDir = rot * inputDir;

		auto vel = vec3(inputDir);
		vel = vel * 6.f * (1.f + Input::GetMapped(Input::Boost)*3.f);
		vel.y = bt2o(rigidbody->getLinearVelocity()).y;

		if(Input::GetMappedDown(Input::Jump))
			vel.y += 10.f;

		rigidbody->setLinearVelocity(o2bt(vel));
	}

	camera->UpdateMatrices();

	if(Input::GetKeyDown(SDLK_1)) blockType = 1;
	if(Input::GetKeyDown(SDLK_2)) blockType = 2;
	if(Input::GetKeyDown(SDLK_3)) blockType = 3;
	if(Input::GetKeyDown(SDLK_4)) blockType = 4;
	if(Input::GetKeyDown(SDLK_5)) blockType = 5;
	if(Input::GetKeyDown(SDLK_6)) blockType = 6;
	if(Input::GetKeyDown(SDLK_7)) blockType = 7;
	if(Input::GetKeyDown(SDLK_8)) blockType = 8;
	if(Input::GetKeyDown(SDLK_9)) blockType = 9;

	if(Input::GetKeyDown(SDLK_0)) blockType = 0;

	if(Input::GetKeyDown(SDLK_q)) blockRot = (blockRot+1)&3;
	if(Input::GetKeyDown(SDLK_e)) blockRot = (blockRot-1)&3;

	if(Input::GetButtonDown(Input::MouseLeft) || Input::GetButtonDown(Input::MouseRight)) {
		auto raycastResult = Physics::Raycast(
			camera->position + camera->forward*0.3f,
			camera->position + camera->forward*10.f);

		if(raycastResult.hit){
			auto col = raycastResult.rigidbody->getCollisionShape();
			// TODO: Change this
			// It will explode as soon as we start using the user pointers for other things
			if(auto chnk = (VoxelChunk*)col->getUserPointer()) {

				if(Input::GetButtonDown(Input::MouseRight) || !blockType)
					raycastResult.normal = -raycastResult.normal;

				auto chpos = raycastResult.position + raycastResult.normal * 0.1f;
				auto vxpos = chnk->WorldToVoxelSpace(chpos);

				if(!blockType) {
					auto blk = chnk->GetBlock(vxpos);
					if(blk) {
						if(auto dyn = blk->AsDynamic())
							dyn->OnInteract();
					}
				}else{
					if(Input::GetButtonDown(Input::MouseRight)){
						ClientNetInterface::SetBlock(chnk->chunkID, vxpos, 0, 0);
					}else{
						// if(!chnk->InBounds(vxpos)) {
						// 	auto nchnk = chnk->GetOrCreateNeighbor(chpos);
						// 	vxpos = nchnk->WorldToVoxelSpace(chpos);

						// 	chnk = nchnk.get();
						// }

						// auto blk = chnk->CreateBlock(vxpos, blockType);
						// if(blk) blk->orientation = blockRot;

						ClientNetInterface::SetBlock(chnk->chunkID, vxpos, blockType, blockRot);
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

