#pragma once

#include <glm/glm.hpp>
#include "stdio.h"
#include "stdint.h"
#include <cstring>
#include <string>
#include <vector>

namespace Ghost
{
    using glm::vec3;
    using std::string;
    using std::vector;

    struct DemoHeader
    {
        uint8_t tag[8];
        uint32_t demoProtocol;
        uint32_t netProtocol;
        uint8_t  mapName[260];
        uint8_t  modName[260];
        int32_t  mapCrc;
        uint32_t dirOffset;
    };

    struct DemoDirectory
    {
        uint32_t id;
        uint8_t name[64];
        uint32_t flags;
        uint32_t cdTrack;
        float time;
        uint32_t frames;
        uint32_t offset;
        uint32_t length;
    };

    struct GhostEntry
    {
        char mapName[260];
        uint32_t begin, end;
        float realTime;
    };

    struct GhostNode
    {
        vec3 position;
        float realTime;
    };

    struct Ghost
    {
        vector<GhostEntry> entries;
        vector<GhostNode> nodes;

        void processBxtCommand(string, float&, GhostEntry&);
        bool process_demo(uint8_t*, float&);
        void set(const char*);
        uint32_t getInfo(float, const char*);
    };

    extern Ghost ghost;
};