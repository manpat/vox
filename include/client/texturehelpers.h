#ifndef TEXTUREHELPERS_H
#define TEXTUREHELPERS_H

#include "common.h"

u32 CreateTextureArrayFromAtlas(std::string fname, u32 textureWidth, u32 textureHeight);
u32 CreateTextureFromFile(std::string fname);

#endif