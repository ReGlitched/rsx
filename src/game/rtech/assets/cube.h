#pragma once
#include <game/rtech/cpakfile.h>

struct CubeAssetHeader_t
{
	char gap[40];
};

static_assert(sizeof(CubeAssetHeader_t) == 40);