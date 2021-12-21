#include "ghost.hpp"
#include <iostream>

namespace Ghost
{
    template <typename T>
    T READ(uint8_t *data, uint32_t &offset)
    {
        T *ptr = (T*)&data[offset];
        offset += sizeof(T);
        return *ptr;
    }

    uint8_t *read_file(const char *filename)
    {
        FILE *input = fopen(filename, "rb");
        if(!input) return NULL;

        fseek(input, 0, SEEK_END);
        int size = ftell(input);
        fseek(input, 0, SEEK_SET);

        uint8_t *data = new uint8_t[size];
        if(1 != fread(data, size, 1, input)) return NULL;

        fclose(input);
        return data;
    }

    bool Ghost::process_demo(uint8_t *data, float &t)
    {
        uint32_t offset = 0;
        DemoHeader header = READ<DemoHeader>(data, offset);

        GhostEntry entry;
        strcpy((char*)entry.mapName, (const char*)header.mapName);
        entry.realTime = t;
        entry.begin = nodes.size();


        offset = header.dirOffset;
        uint32_t numDirs = READ<uint32_t>(data, offset);
        DemoDirectory dir;

        bool playbackFound = false;
        for(unsigned int i = 0; i < numDirs; i++)
        {
            dir = READ<DemoDirectory>(data, offset);
            if(0 == strcmp("Playback", (const char*)dir.name))
            {
                playbackFound = true;
                break;
            }
        }

        if(!playbackFound) return false;

        offset = dir.offset;

        bool end = false;
        while(!end)
        {
            uint8_t type = READ<uint8_t>(data, offset);
            float time = READ<float>(data, offset);
            uint32_t frame = READ<uint32_t>(data, offset);

            switch(type)
            {
                case 0:
                case 1:
                {
                    offset += 4;
                    vec3 position = READ<vec3>(data, offset);
                    offset += 48;
                    float frameTime = READ<float>(data, offset);
                    nodes.push_back({position, t});
                    t += frameTime;
                    offset += 396;
                    uint32_t frameDataLength = READ<uint32_t>(data, offset);
                    offset += frameDataLength;
                }
                break;
                case 2: break;
                case 3: offset += 64; break;
                case 4: offset += 32; break;
                case 5: end = true; break;
                case 6: offset += 84; break;
                case 7: offset += 8; break;
                case 8:
                {
                    offset += 4;
                    uint32_t sampleSize = READ<uint32_t>(data, offset);
                    offset += sampleSize + 16;
                }
                break;
                case 9:
                {
                    uint32_t num = READ<uint32_t>(data, offset);
                    offset += num;
                }
                break;
            }
        }

        if(nodes.size() == 0) return false;
        entry.end = nodes.size()-1;
        entries.push_back(entry);
        return true;
    }

    void Ghost::set(const char *prefix)
    {
        entries.clear();
        nodes.clear();
        float realTime = 0;

        for(int dem = 1; true; dem++)
        {
            char input_name[260];
            sprintf(input_name, "%s_%d.dem", prefix, dem);
            uint8_t *data = read_file(input_name);
            
            if(data == NULL) break; //end of demos

            process_demo(data, realTime);

            if(data) delete[] data;
        }
    }

    uint32_t Ghost::getInfo(float realTime, const char *mapName)
    {
        if(nodes.size() < 1) return -1;
        if(entries.size() < 1) return -1;

        //binary search
        uint32_t e0 = 0;
        uint32_t e1 = entries.size()-1;
        while(e1 - e0 > 1)
        {
            uint32_t mid = (e0+e1)/2;
            float midTime = entries[mid].realTime;
            if(realTime < midTime) e1 = mid; else e0 = mid;
        }

        //if(0 != strcmp(mapName, entries[curEntry].mapName)) return false;

        //binary search
        uint32_t n0 = entries[e0].begin;
        uint32_t n1 = entries[e0].end;
        while(n1 - n0 > 1)
        {
            uint32_t mid = (n0+n1)/2;
            float midTime = nodes[mid].realTime;
            if(realTime < midTime) n1 = mid; else n0 = mid;
        }

        return n0;
    }

    Ghost ghost;
};