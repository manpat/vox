#include "texturehelpers.h"

#include <SDL2/SDL_image.h>

static Log logger{"TextureHelpers"};

u32 CreateTextureArrayFromAtlas(std::string fname, u32 textureWidth, u32 textureHeight) {
	auto surf = IMG_Load(fname.data());
	if(!surf) {
		logger << "Failed to load image " << fname;
		return 0;
	}

	bool hasAlpha = (surf->format->BytesPerPixel == 4);
	if(!hasAlpha && (surf->format->BytesPerPixel != 3)) {
		logger << "Invalid image format: " << surf->format->BytesPerPixel;
		SDL_FreeSurface(surf);
		return 0;
	}

	std::vector<u8> tempBuffer(textureWidth*textureHeight*surf->format->BytesPerPixel);

	u32 numTilesPerRow = surf->w/textureWidth;
	u32 bytesPerTileRow = surf->pitch/numTilesPerRow;

	u32 textureArray;
	glGenTextures(1, &textureArray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 
		textureWidth, textureHeight, numTilesPerRow*surf->h/textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Copies tiles one by one into tempBuffer and uploads to a new layer
	for(u32 ty = 0; ty < surf->h/textureHeight; ty++)
	for(u32 tx = 0; tx < numTilesPerRow; tx++) {
		for(u32 y = 0; y < textureHeight; y++)
		for(u32 x = 0; x < bytesPerTileRow; x++) {
			u32 tidx = x + y * bytesPerTileRow;
			u32 sidx = x + tx*bytesPerTileRow + (y + ty*textureHeight) * surf->pitch;
			tempBuffer[tidx] = ((u8*)surf->pixels)[sidx];
		}

		u32 idx = tx + ty * numTilesPerRow;
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, idx, textureWidth, textureHeight, 1,
			hasAlpha?GL_RGBA:GL_RGB, GL_UNSIGNED_BYTE, &tempBuffer[0]);
	}

	// These must be set at least once otherwise the texture can't be used
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

	SDL_FreeSurface(surf);

	return textureArray;
}

u32 CreateTextureFromFile(std::string fname) {
	auto surf = IMG_Load(fname.data());
	if(!surf) {
		logger << "Failed to load image " << fname;
		return 0;
	}

	bool hasAlpha = (surf->format->BytesPerPixel == 4);
	if(!hasAlpha && (surf->format->BytesPerPixel != 3)) {
		logger << "Invalid image format: " << surf->format->BytesPerPixel;
		SDL_FreeSurface(surf);
		return 0;
	}

	u32 texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, surf->w, surf->h, 0, 
		hasAlpha?GL_RGBA:GL_RGB, GL_UNSIGNED_BYTE, surf->pixels);

	// These must be set at least once otherwise the texture can't be used
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);

	SDL_FreeSurface(surf);

	return texture;
}
