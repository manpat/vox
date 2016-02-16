#include "texturehelpers.h"
#include "shaderregistry.h"
#include "chunkrenderer.h"
#include "chunkmanager.h"
#include "voxelchunk.h"
#include "quadbuffer.h"
#include "camera.h"
#include "block.h"

static Log logger{"ChunkRenderer"};

ChunkRenderer::ChunkRenderer() {
	textureArray = CreateTextureArrayFromAtlas("textures/atlas.png", 16, 16);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
	
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	logger << "Note: this bit hangs sometimes and I don't know why";
	// TODO: Figure out why this hangs sometimes
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	// Init voxel texture info
	voxelTextures = {0,0,0,0,0,0};
	for(u16 i = 0; i < BlockRegistry::blockInfoCount; i++) {
		auto& bt = BlockRegistry::blocks[i];

		voxelTextures.insert(voxelTextures.end(), bt.textures.begin(), bt.textures.end());
		if(bt.RequiresIDsForRotations()){
			voxelTextures.insert(voxelTextures.end(), bt.textures.begin(), bt.textures.end());
			voxelTextures.insert(voxelTextures.end(), bt.textures.begin(), bt.textures.end());
			voxelTextures.insert(voxelTextures.end(), bt.textures.begin(), bt.textures.end());
		}
	}

	auto chunkManager = ChunkManager::Get();
	auto vinput = stbvox_get_input_description(&chunkManager->mm);
	vinput->block_tex1_face = (u8(*)[6]) &voxelTextures[0];
}

ChunkRenderer::~ChunkRenderer() {
	glDeleteTextures(1, &textureArray);
}

void ChunkRenderer::Render() {
	auto chunkManager = ChunkManager::Get();

	auto program = ShaderRegistry::GetProgram("voxel");
	program->Use();

	// Set up uniforms
	stbvox_uniform_info sui;
	for(auto u: {STBVOX_UNIFORM_normals, STBVOX_UNIFORM_texgen}) {
		if (stbvox_get_uniform_info(&sui, u))
			glUniform3fv(program->GetUniform(sui.name), sui.array_length, sui.default_value);
	}

	if (stbvox_get_uniform_info(&sui, STBVOX_UNIFORM_ambient)){
		vec4 data[4] {vec4{0.f}};
		data[0] = glm::normalize(vec4(1,0.5,1,1));
		data[1] = vec4(0.5f);
		data[2] = vec4(1, 1, 1, 1);

		glUniform4fv(program->GetUniform(sui.name), sui.array_length, &data[0].x);
	}

	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
	glUniform1i(program->GetUniform("tex_array"), 1);

	glActiveTexture(GL_TEXTURE0);

	if(auto cam = Camera::mainCamera.lock()) {
		cam->UpdateMatrices();
		cam->SetUniforms(program.get());
	}
	
	for(auto& vc: chunkManager->chunks) {
		auto infoMap = &chunkRenderInfoMap[vc->chunkID];

		if(vc->voxelsDirty) {
			infoMap->Update(vc);
			vc->voxelsDirty = false;
		}

		if(!vc->numQuads) continue;

		QuadElementBuffer::SetNumQuads(vc->numQuads);

		glBindBuffer(GL_ARRAY_BUFFER, infoMap->vertexBO);
		glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 4, nullptr);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, infoMap->faceTex);
		glUniform1i(program->GetUniform("facearray"), 0);

		// glUniformMatrix4fv(program->GetUniform("model"), 1, false, glm::value_ptr(vc->modelMatrix));
		glUniformMatrix4fv(program->GetUniform("model"), 1, false, glm::value_ptr(glm::translate(vc->position)));

		QuadElementBuffer::Draw(vc->numQuads);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	for(auto& vc: chunkManager->chunks) {
		vc->PostRender();
	}
}


/*
	                                                                                                      
	88888888ba                                 88                       88               ad88             
	88      "8b                                88                       88              d8"               
	88      ,8P                                88                       88              88                
	88aaaaaa8P' ,adPPYba, 8b,dPPYba,   ,adPPYb,88  ,adPPYba, 8b,dPPYba, 88 8b,dPPYba, MM88MMM ,adPPYba,   
	88""""88'  a8P_____88 88P'   `"8a a8"    `Y88 a8P_____88 88P'   "Y8 88 88P'   `"8a  88   a8"     "8a  
	88    `8b  8PP""""""" 88       88 8b       88 8PP""""""" 88         88 88       88  88   8b       d8  
	88     `8b "8b,   ,aa 88       88 "8a,   ,d88 "8b,   ,aa 88         88 88       88  88   "8a,   ,a8"  
	88      `8b `"Ybbd8"' 88       88  `"8bbdP"Y8  `"Ybbd8"' 88         88 88       88  88    `"YbbdP"'   
	                                                                                                      
	                                                                                                      
*/

ChunkRenderInfo::ChunkRenderInfo() : vertexBO{0}, faceBO{0}, faceTex{0} {}
ChunkRenderInfo::ChunkRenderInfo(ChunkRenderInfo&& o) {
	vertexBO = o.vertexBO;
	faceTex = o.faceTex;
	faceBO = o.faceBO;

	o.vertexBO = 0;
	o.faceTex = 0;
	o.faceBO = 0;
}

ChunkRenderInfo::~ChunkRenderInfo() {
	glDeleteBuffers(1, &vertexBO);
	glDeleteBuffers(1, &faceBO);
	glDeleteTextures(1, &faceTex);
}

void ChunkRenderInfo::Update(std::shared_ptr<VoxelChunk> vc) {
	vc->GenerateMesh();

	if(!vertexBO) glGenBuffers(1, &vertexBO);
	if(!faceBO) glGenBuffers(1, &faceBO);

	auto manager = ChunkManager::Get();

	glBindBuffer(GL_ARRAY_BUFFER, vertexBO);
	glBufferData(GL_ARRAY_BUFFER, vc->numQuads*4*sizeof(u32), manager->vertexBuildBuffer, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_TEXTURE_BUFFER, faceBO);
	glBufferData(GL_TEXTURE_BUFFER, vc->numQuads*sizeof(u32), manager->faceBuildBuffer, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);

	if(!faceTex) {
		glGenTextures(1, &faceTex);
		glBindTexture(GL_TEXTURE_BUFFER, faceTex);
		glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8UI, faceBO);
		glBindTexture(GL_TEXTURE_BUFFER, 0);
	}
}
