#include "blocks/basic.h"
#include "debugdraw.h"
#include "overlay.h"
#include "chunk.h"

struct TestDynamicBlockOverlay : Overlay {
	TestDynamicBlock* block;
	vec3 color = vec3{1,1,1};

	void Update() override {
		Debug::Point(block->GetWorldCenter(), color);
	}
};

struct TestDynamicBlockClientState {
	std::shared_ptr<TestDynamicBlockOverlay> testOverlay;
};

void TestDynamicBlock::OnPlace(u16 playerID){
	static_assert(sizeof(TestDynamicBlockClientState) <= sizeof(stateArena), "BlockState too large");

	auto state = new(stateArena) TestDynamicBlockClientState;
	state->testOverlay = std::make_shared<TestDynamicBlockOverlay>();
	state->testOverlay->block = this;
	OverlayManager::Get()->Add(state->testOverlay);

	Log("TestDynamicBlock") << "OnPlace Client " << playerID;
}

void TestDynamicBlock::OnBreak(u16 playerID){
	Log("TestDynamicBlock") << "OnBreak Client " << playerID;

	// Destroy overlay
	auto state = (TestDynamicBlockClientState*) stateArena;
	state->testOverlay.reset();
}

void TestDynamicBlock::OnInteract(u16 playerID){
	auto state = (TestDynamicBlockClientState*) stateArena;

	auto randf = []{ return (rand()%1000)/1000.f; };
	state->testOverlay->color = vec3{randf(), randf(), randf()};
}
