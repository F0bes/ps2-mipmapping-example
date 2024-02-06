#pragma once
#include <tamtypes.h>

#define DECLARE_MIP_LEVEL(index) \
    extern u32 size_mip_level_##index; \
    extern u8 mip_level_##index[];

DECLARE_MIP_LEVEL(0)
DECLARE_MIP_LEVEL(1)
DECLARE_MIP_LEVEL(2)
DECLARE_MIP_LEVEL(3)
DECLARE_MIP_LEVEL(4)
DECLARE_MIP_LEVEL(5)
