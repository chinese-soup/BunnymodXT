#include "ghost.hpp"

namespace Ghost
{
    template <typename T>
    T READ(const uint8_t *data, uint32_t &offset)
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
        string str;
        bool escaping = false;
        int buf_pos = 0;
        uint8_t buffer[8];

        while(true)
        {
            if(data[offset] != 3) return str;
            if(strncmp((const char*)&data[offset+9], "//BXTD0", 7)) return str;
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

    void Ghost::processBxtCommand(string str, float &accumTime, GhostEntry &entry)
    {
        uint32_t size = str.size();
        const uint8_t *data = (const uint8_t*)str.data();

        for(uint32_t pos = 4; pos < size; )
        {
            switch(data[pos++])
            {
                case 1: //version info
                    {
                        pos += 4;
                        uint32_t vi_size = READ<uint32_t>(data, pos);
                        pos += vi_size;
                    }
                    break;
                case 2: //cvar values
                    {
                        uint32_t count = READ<uint32_t>(data, pos);
                        for(uint32_t cv = 0; cv < count; cv++)
                        {
                            uint32_t cv_size = READ<uint32_t>(data, pos);
                            pos += cv_size;
                            uint32_t val_size = READ<uint32_t>(data, pos);
                            pos += val_size;
                        }
                    }
                    break;
                case 3: //time
                    {
                        uint32_t hours = READ<uint32_t>(data, pos);
                        uint8_t minutes = READ<uint8_t>(data, pos);
                        uint8_t seconds = READ<uint8_t>(data, pos);
                        double rem = READ<double>(data, pos);
                        accumTime = hours*3600 + minutes*60 + seconds + rem;
                    }
                    break;
                case 4: //bound cmd
                    {
                        uint32_t cmd_size = READ<uint32_t>(data, pos);
                        pos += cmd_size;
                    }
                    break;
                case 5: //alias expansion
                    {
                        uint32_t alias_size = READ<uint32_t>(data, pos);
                        pos += alias_size;
                        uint32_t cmd_size = READ<uint32_t>(data, pos);
                        pos += cmd_size;
                    }
                    break;
                case 6: //script exec
                    {
                        uint32_t name_size = READ<uint32_t>(data, pos);
                        pos += name_size;
                        uint32_t content_size = READ<uint32_t>(data, pos);
                        pos += content_size;
                    }
                    break;
                case 7: //cmd exec
                    {
                        uint32_t cmd_size = READ<uint32_t>(data, pos);
                        
                        if(0 == strncmp((const char*)&data[pos], "changelevel2 ", 13))
                        {
                            string newMap;
                            for(uint32_t i = 13; data[pos+i] != ' '; i++)
                                newMap.push_back(data[pos+i]);

                            entry.end = nodes.size()-1;
                            entries.push_back(entry);

                            strcpy((char*)entry.mapName, newMap.c_str());
                            entry.realTime = accumTime;
                            entry.begin = nodes.size();   
                        }

                        pos += cmd_size;
                    }
                    break;
                case 8: //game end
                    {
                    }
                    break;
                case 9: //loaded modules
                    {
                        uint32_t count = READ<uint32_t>(data, pos);
                        for(uint32_t md = 0; md < count; md++)
                        {
                            uint32_t mod_size = READ<uint32_t>(data, pos);
                            pos += mod_size;
                        }
                    }
                    break;
                case 10: //custom trigger cmd
                    {
                        pos += 24;
                        uint32_t cmd_size = READ<uint32_t>(data, pos);
                        pos += cmd_size;   
                    }
                    break;
                case 11: //edicts
                    {
                        pos += 4;
                    }
                    break;
                default:
                    return;
            }
        }
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
                    //vec3 position = READ<vec3>(data, offset);
                    offset += 12*5;
                    float frameTime = READ<float>(data, offset);
                    accumTime += frameTime;
                    offset += 36;
                    vec3 origin = READ<vec3>(data, offset);
                    nodes.push_back({origin, accumTime});
                    
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
                        if(str.size() > 0) processBxtCommand(str, accumTime, entry);
                        
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
