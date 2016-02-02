#include "shaderregistry.h"
#include "chunkmanager.h"
#include "voxelchunk.h"
#include "camera.h"
#include "block.h"

#include <SDL2/SDL_image.h>

static Log logger{"ChunkManager"};

ChunkManager::ChunkManager() {
	vertexBuildBuffer = new u8[VertexBufferSize];
	faceBuildBuffer = new u8[FaceBufferSize];

	PopulateVoxelInfo();

	constexpr u32 TextureSize = 16;

	auto InitAtlas = [](std::string fname) {
		auto surf = IMG_Load(fname.data());
		if(!surf) {
			logger << "Failed to load image " << fname;
			return;
		}

		bool hasAlpha = (surf->format->BytesPerPixel == 4);
		if(!hasAlpha && (surf->format->BytesPerPixel != 3)) {
			logger << "Invalid image format: " << surf->format->BytesPerPixel;
			SDL_FreeSurface(surf);
			return;
		}

		static u8 tempBuffer[TextureSize*TextureSize*4]; // Assume alpha

		u32 numTilesPerRow = surf->w/TextureSize;
		u32 bytesPerTileRow = surf->pitch/numTilesPerRow;

		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 
			TextureSize, TextureSize, numTilesPerRow*surf->h/TextureSize,  0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		for(u32 ty = 0; ty < surf->h/TextureSize; ty++)
		for(u32 tx = 0; tx < numTilesPerRow; tx++) {
			for(u32 y = 0; y < TextureSize; y++)
			for(u32 x = 0; x < bytesPerTileRow; x++) {
				u32 tidx = x + y * bytesPerTileRow;
				u32 sidx = x + tx*bytesPerTileRow + (y + ty*TextureSize) * surf->pitch;
				tempBuffer[tidx] = ((u8*)surf->pixels)[sidx];
			}

			u32 idx = tx + ty * numTilesPerRow;
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, idx, TextureSize, TextureSize, 1,
				hasAlpha?GL_RGBA:GL_RGB, GL_UNSIGNED_BYTE, tempBuffer);
		}

		SDL_FreeSurface(surf);
	};

	glGenTextures(1, &textureArray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);

	InitAtlas("textures/atlas.png");
	logger << "Initialised texture atlas";

	// TODO: Figure out why this hangs sometimes
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	logger << "Generated mipmaps";
	
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	logger << "Init mesh maker";
	stbvox_init_mesh_maker(&mm);
	auto vinput = stbvox_get_input_description(&mm);
	memset(vinput, 0, sizeof(stbvox_input_description));
	
	vinput->block_geometry = &voxelTypes[0];
	vinput->block_tex1_face = (u8(*)[6]) &voxelTextures[0];
	logger << "Initialisation complete";
}

ChunkManager::~ChunkManager() {
	delete[] vertexBuildBuffer;
	delete[] faceBuildBuffer;
}

void ChunkManager::PopulateVoxelInfo() {
	static std::map<GeometryType, u32> voxelGeomMap {
		{GeometryType::Cube,	STBVOX_GEOM_solid},
		{GeometryType::Slab,	STBVOX_GEOM_slab_lower},
		{GeometryType::Cross,	STBVOX_GEOM_crossed_pair},
		{GeometryType::Slope,	STBVOX_GEOM_floor_slope_north_is_top},
	};

	voxelTypes = {STBVOX_MAKE_GEOMETRY(STBVOX_GEOM_empty, 0, 0)};
	voxelTextures = {0,0,0,0,0,0};
	u16 id = 1;

	for(u16 i = 0; i < BlockRegistry::blockInfoCount; i++) {
		auto& bt = BlockRegistry::blocks[i];
		bt.voxelID = id;

		if(bt.RequiresIDsForRotations()) {
			id += 4;
			for(u8 r = 0; r < 4; r++) {
				voxelTypes.push_back(STBVOX_MAKE_GEOMETRY(voxelGeomMap[bt.geometry], r, 0));
				voxelTextures.insert(voxelTextures.end(), bt.textures.begin(), bt.textures.end());
			}
		}else{
			id++;
			voxelTypes.push_back(STBVOX_MAKE_GEOMETRY(voxelGeomMap[bt.geometry], 0, 0));
			voxelTextures.insert(voxelTextures.end(), bt.textures.begin(), bt.textures.end());
		}

		logger << "Voxel type created for " << bt.name << ": " << bt.blockID << " -> " << bt.voxelID;
	}
}

std::shared_ptr<VoxelChunk> ChunkManager::CreateChunk(u32 w, u32 h, u32 d, vec3 position, bool placeBlock) {
	auto nchunk = std::make_shared<VoxelChunk>(this, w,h,d);
	chunks.push_back(nchunk);

	if(placeBlock) {
		u32 cx = nchunk->width/2;
		u32 cy = nchunk->height/2;
		u32 cz = nchunk->depth/2;

		nchunk->CreateBlock(cx,cy,cz, 1);
	}

	position.x += nchunk->width/-2.f -1;
	position.y += nchunk->height/-2.f -1;
	position.z += nchunk->depth/2.f +1;

	nchunk->modelMatrix = glm::translate(position);
	return nchunk;
}

void ChunkManager::Update() {
	for(auto& vc: chunks) {
		vc->Update();
	}
}

void ChunkManager::Render(Camera* cam) {
	auto program = ShaderRegistry::GetProgram("voxel");

	program->Use();

	stbvox_uniform_info sui;
	if (stbvox_get_uniform_info(&sui, STBVOX_UNIFORM_normals))
		glUniform3fv(program->GetUniform(sui.name), sui.array_length, sui.default_value);
		
	if (stbvox_get_uniform_info(&sui, STBVOX_UNIFORM_texgen))
		glUniform3fv(program->GetUniform(sui.name), sui.array_length, sui.default_value);

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

	cam->UpdateMatrices();
	cam->SetUniforms(program.get());
	
	for(auto& vc: chunks) {
		vc->Render(program.get());
	}

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	for(auto& vc: chunks) {
		vc->PostRender();
	}
}