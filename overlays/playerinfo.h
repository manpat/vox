#ifndef OVR_PLAYERINFO_H
#define OVR_PLAYERINFO_H

#include <sstream>

#include "gui.h"
#include "block.h"
#include "common.h"
#include "player.h"
#include "overlay.h"
#include "textrendering.h"
#include "shaderregistry.h"

struct PlayerInfoOverlay : Overlay {
	std::shared_ptr<Player> player;
	TextMesh timeText;

	PlayerInfoOverlay(std::shared_ptr<Player> p) : player{p}, timeText{Font::defaultFont} {}
	void Render() override {
		auto textProgram = ShaderRegistry::GetProgram("text");
		textProgram->Use();

		auto gui = GUI::Get();
		gui->camera->SetUniforms(textProgram.get());

		glUniform4f(textProgram->GetUniform("color"), 0.5,0.7,0.8, 1);
		static f32 fps = 60.f;
		fps = (fps*29.f + 1.f/Time::dt)/30.f;

		static const char* playerRotStrings[] = {
			"North (-Z)", "East (+X)", "South (+Z)", "West (-X)"
		};

		std::ostringstream ss;
		ss << (s32)fps << '\n';
		ss << playerRotStrings[player->blockRot] << '\n';
		ss << ((!player->blockType)?"interact":BlockRegistry::blocks[player->blockType-1].name) << '\n';

		timeText.SetText(ss.str());
		// timeText.modelMatrix = glm::translate<f32>(vec3{-gui->aspect*10.f, 10.f, 0});
		f32 scale = 2.f/timeText.size.x;
		timeText.modelMatrix = glm::translate(vec3{0.f, 12.f, 0} * vec3{gui->cellSize,0}) * glm::scale(vec3{scale, scale, scale});
		timeText.Render();
	}
};

#endif