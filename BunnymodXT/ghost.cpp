#include "ghost.hpp"
#include <iostream>
#include <stdlib.h>

namespace Ghost
{
    using std::string;

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

    void decrypt(uint8_t *data)
    {
        static const uint32_t key[4] = { 0x1337FACE, 0x12345678, 0xDEADBEEF, 0xFEEDABCD };
        uint32_t *num = (uint32_t*)data;
        uint32_t v0=num[0], v1=num[1], sum=0xC6EF3720, i;
        uint32_t delta=0x9e3779b9;
        uint32_t k0=key[0], k1=key[1], k2=key[2], k3=key[3];

        for (i=0; i < 32; i++)
        {
            v1 -= ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
            v0 -= ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
            sum -= delta;
        }
        
        num[0]=v0; num[1]=v1;
    }

    string getBxtCommand(uint8_t *data, uint32_t &offset)
    {
        const char HEADER[] = "//BXTD0";
        string str;
        bool escaping = false;
        int buf_pos = 0;
        uint8_t buffer[8];

        while(true)
        {
            if(data[offset] != 3) return str;
            for(int i = 0; i < 7; i++) if(HEADER[i] != data[offset+9+i]) return str;
            offset += 9;

            for(int i = 7; i < 63; i++)
            {
                if(escaping)
                {
                    escaping = false;
                    switch(data[offset+i])
                    {
                        case 0x01: buffer[buf_pos++] = 0; break;
                        case 0x02: buffer[buf_pos++] = '"'; break;
                        case 0x03: buffer[buf_pos++] = '\n'; break;
                        case 0x04: buffer[buf_pos++] = ';'; break;
                        case 0xFF: buffer[buf_pos++] = 0xFF; break;
                    }
                }
                else
                {
                    if(data[offset+i] == 0xFF)
                    {
                        escaping = true;
                    }
                    else
                    {
                        buffer[buf_pos++] = data[offset+i];
                    }
                }

                if(buf_pos == 8)
                {
                    decrypt(buffer);
                    buf_pos = 0;
                    for(int i = 0; i < 8; i++)
                    {
                        //printf("%c", buffer[i]);
                        str += buffer[i];
                    }
                }
            }

            offset += 64;
        }

        return str;
    }

    bool Ghost::process_demo(uint8_t *data, float &accumTime)
    {
        uint32_t offset = 0;
        DemoHeader header = READ<DemoHeader>(data, offset);

        GhostEntry entry;
        strcpy((char*)entry.mapName, (const char*)header.mapName);
        entry.realTime = accumTime;
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

        float time = 0;
    
        bool end = false;
        while(!end)
        {
            uint8_t type = READ<uint8_t>(data, offset);
            time = READ<float>(data, offset);
            //uint32_t frame = READ<uint32_t>(data, offset);
            offset += 4;

            switch(type)
            {
                case 0:
                case 1:
                {
                    offset += 4;
                    vec3 position = READ<vec3>(data, offset);
                    offset += 48;
                    offset += 40;
                    vec3 origin = READ<vec3>(data, offset);
                    nodes.push_back({origin, accumTime + time});
                    
                    offset += 348;
                    uint32_t frameDataLength = READ<uint32_t>(data, offset);
                    offset += frameDataLength;
                }
                break;
                case 2: break;
                case 3:
                    {
                        offset -= 9;
                        string str = getBxtCommand(data, offset);
                        if(str.size() != 0)
                        {
                            size_t f = str.find("changelevel2 ");
                            if(f != std::string::npos)
                            {
                                entry.end = nodes.size()-1;
                                entries.push_back(entry);

                                for(int i = 0; i+f+13 < str.size(); i++)
                                {
                                    entry.mapName[i] = str[i+f+13];
                                    if(str[i+f+13] == ' ') entry.mapName[i] = 0;
                                }

                                entry.realTime = accumTime+time;
                                entry.begin = nodes.size();
                            }
                        }
                        else offset += 64+9;
                    }
                    break;
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

        accumTime += time;
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

        if(0 != strcmp(mapName, entries[e0].mapName)) return -1;

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