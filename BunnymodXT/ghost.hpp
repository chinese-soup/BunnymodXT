#pragma once

#include <GL/gl.h>
#include "stdio.h"
#include "stdint.h"

namespace Ghost
{

    struct GhostEntry
    {
        const char mapName[260];
        uint32_t begin, end;
        float realTime;
    };

    struct GhostNode
    {

    };

    struct Ghost
    {

    };
};