#ifndef BL_BASIC_H
#define BL_BASIC_H

#include "voxelchunk.h"
#include "common.h"
#include "block.h"

struct SteelBlock : Block {
	static void PopulateBlockInfo(BlockInfo* bt) {
		bt->name = "steel";
		bt->geometry = GeometryType::Cube;
		bt->textures = {0,0,0,0,0,0};
		bt->dynamic = false;
		bt->doesOcclude = true;
	}
};

struct SteelSlab : Block {
	static void PopulateBlockInfo(BlockInfo* bt) {
		bt->name = "steelslab";
		bt->geometry = GeometryType::Slab;
		bt->textures = {0,0,0,0,0,0};
		bt->dynamic = false;
		bt->doesOcclude = true;
	}
};

struct LightThingBlock : Block {
	static void PopulateBlockInfo(BlockInfo* bt) {
		bt->name = "lightthing";
		bt->geometry = GeometryType::Cube;
		bt->textures = {1,1,1,1,1,1};
		bt->dynamic = false;
		bt->doesOcclude = true;
	}
};

struct RampBlock : Block {
	static void PopulateBlockInfo(BlockInfo* bt) {
		bt->name = "ramp";
		bt->geometry = GeometryType::Slope;
		bt->textures = {0,0,0,0,4,0};
		bt->dynamic = false;
		bt->doesOcclude = true;
	}
};

struct PoleBlock : Block {
	static void PopulateBlockInfo(BlockInfo* bt) {
		bt->name = "pole";
		bt->geometry = GeometryType::Cross;
		bt->textures = {2,2,2,2,2,2};
		bt->dynamic = false;
		bt->doesOcclude = false;
	}
};

struct ComputerBlock : DynamicBlock {
	static void PopulateBlockInfo(BlockInfo* bt) {
		bt->name = "computer";
		bt->geometry = GeometryType::Cube;
		bt->textures = {0,0,0,3,0,0};
		bt->dynamic = true;
		bt->doesOcclude = true;
	}

	void Update() override {
		// Log("ComputerBlock") << "Update";
	}

	void OnInteract() override {
		Log("ComputerBlock") << "OnInteract";
		// TODO: Create/Show overlay
	}
	
	void OnPlace() override {
		Log("ComputerBlock") << "OnPlace";
	}
	
	void OnBreak() override {
		Log("ComputerBlock") << "OnBreak";
	}
};

#include "shaderregistry.h"
#include "textrendering.h"
#include "camera.h"
#include "debugdraw.h"

struct TextBlock : DynamicBlock {
	static void PopulateBlockInfo(BlockInfo* bt) {
		bt->name = "text";
		bt->geometry = GeometryType::Cube;
		bt->textures = {0,0,0,5,0,0};
		bt->dynamic = true;
		bt->doesOcclude = true;
	}

	TextMesh tm;

	TextBlock() : tm{Font::defaultFont} {
		tm.lineHeightMult = 7.f/8.f;
		tm.SetText("Hello\nWorld!\nHallå Världen\nÅåÖö\u00d0\u00f0\u00d8\u00f8\u00de\u00fe\nÿÿÿ");
	}

	void PostRender() override {
		auto tsh = ShaderRegistry::GetProgram("text");
		tsh->Use();

		if(auto cam = Camera::mainCamera.lock())
			cam->SetUniforms(tsh.get());
	
		vec3 off = vec3{2.5f/16.f-.5, .5-2.5f/16.f, .51f};

		glUniform4f(tsh->GetUniform("color"), .2,.7,.2, 1);
		tm.modelMatrix = chunk->modelMatrix * 
			glm::translate<f32>(GetRelativeCenter()) *
			GetOrientationMat() *
			glm::translate<f32>(off) * 
			glm::scale(vec3{0.1f});
		tm.Render();
	}
};

#endif