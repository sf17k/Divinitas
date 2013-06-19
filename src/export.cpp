#include "export.h"
#include "nbt.h"

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <deque>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>
using namespace std;
using namespace boost::filesystem;


inline void writeInt(int32_t n, FILE *out) {
    n = __builtin_bswap32(n); fwrite(&n, 4, 1, out);
}
inline void writeByte(int8_t n, FILE *out) {
    fwrite(&n, 1, 1, out);
}


struct Vec3
{
    int x, y, z;
    Vec3(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}
};


struct WorldParams
{
    const int sizeX, sizeZ, startX, startZ; // in chunks
    ChunkCallback chunkCB;
    SectionCallback sectionCB;

    // dimensions in chunks
    WorldParams(int _size, ChunkCallback _chunkCB, SectionCallback _sectionCB) :
        sizeX(_size),
        sizeZ(_size),
        // center world on origin
        startX(-(_size/2)),
        startZ(-(_size/2)),
        chunkCB(_chunkCB),
        sectionCB(_sectionCB)
    { }
};


struct LevelDat
{
    int32_t version;
    int8_t initialized;
    string LevelName;
    string generatorName;
    int32_t generatorVersion;
    string generatorOptions;
    int64_t RandomSeed;
    int8_t MapFeatures;
    int64_t LastPlayed;
    int64_t SizeOnDisk;
    int8_t allowCommands;
    int8_t hardcore;
    int32_t GameType;
    int64_t Time;
    int64_t DayTime;
    int32_t SpawnX;
    int32_t SpawnY;
    int32_t SpawnZ;
    int8_t raining;
    int32_t rainTime;
    int8_t thundering;
    int32_t thunderTime;

    // GameRules
    bool commandBlockOutput;
    bool doFireTick;
    bool doMobLoot;
    bool doMobSpawning;
    bool doTileDrops;
    bool keepInventory;
    bool mobGriefing;

    LevelDat() :
        version(19133),
        initialized(1),
        LevelName(),
        generatorName("flat"),
        generatorVersion(0),
        generatorOptions(),
        RandomSeed(0),
        MapFeatures(0),
        LastPlayed(0),
        SizeOnDisk(0),
        allowCommands(1),
        hardcore(0),
        GameType(1),
        Time(0),
        DayTime(0),
        SpawnX(0),
        SpawnY(255),
        SpawnZ(0),
        raining(0),
        rainTime(-1),
        thundering(0),
        thunderTime(-1),

        commandBlockOutput(true),
        doFireTick(true),
        doMobLoot(true),
        doMobSpawning(true),
        doTileDrops(true),
        keepInventory(false),
        mobGriefing(true)
    { }

    nbt_node* toNBT()
    {
        nbt_node *start = tag_compound("Data", NBTList()
                << tag_int("version",version)
                << tag_byte("initialized",initialized)
                << tag_string("LevelName",LevelName.c_str())
                << tag_string("generatorName",generatorName.c_str())
                << tag_int("generatorVersion",generatorVersion)
                << tag_string("generatorOptions",generatorOptions.c_str())
                << tag_long("RandomSeed",RandomSeed)
                << tag_byte("MapFeatures",MapFeatures)
                << tag_long("LastPlayed",LastPlayed)
                << tag_long("SizeOnDisk",SizeOnDisk)
                << tag_byte("allowCommands",allowCommands)
                << tag_byte("hardcore",hardcore)
                << tag_int("GameType",GameType)
                << tag_long("Time",Time)
                << tag_long("DayTime",DayTime)
                << tag_int("SpawnX",SpawnX)
                << tag_int("SpawnY",SpawnY)
                << tag_int("SpawnZ",SpawnZ)
                << tag_byte("raining",raining)
                << tag_int("rainTime",rainTime)
                << tag_byte("thundering",thundering)
                << tag_int("thunderTime",thunderTime)

                << tag_string("commandBlockOutput",commandBlockOutput?"true":"false")
                << tag_string("doFireTick",doFireTick?"true":"false")
                << tag_string("doMobLoot",doMobLoot?"true":"false")
                << tag_string("doMobSpawning",doMobSpawning?"true":"false")
                << tag_string("doTileDrops",doTileDrops?"true":"false")
                << tag_string("keepInventory",keepInventory?"true":"false")
                << tag_string("mobGriefing",mobGriefing?"true":"false")
                );

        // wrap it in a root tag
        return tag_compound("", NBTList() << start);
    }
};


struct MCAChunkSection
{
    WorldParams *params;
    int xPos, yPos, zPos; // blocks/16

    MCAChunkSection(int _xPos, int _yPos, int _zPos, WorldParams *_params) :
        params(_params),
        xPos(_xPos),
        yPos(_yPos),
        zPos(_zPos)
    { }

    nbt_node* toNBT()
    {
        // fill arrays
        uint8_t *Blocks = allocate_byte_array(BLOCKS_SIZE);
        uint8_t *Data = allocate_byte_array(DATA_SIZE);
        uint8_t *BlockLight = allocate_byte_array(BLOCKLIGHT_SIZE);
        uint8_t *SkyLight = allocate_byte_array(SKYLIGHT_SIZE);

        params->sectionCB(xPos - params->startX, yPos, zPos - params->startZ, Blocks, Data, BlockLight, SkyLight);

        // important to use NULL for name when it'll be a tag_list entry
        return tag_compound(NULL, NBTList()
                << tag_byte("Y",yPos)
                << tag_byte_array_allocated("Blocks",BLOCKS_SIZE,Blocks)
                << tag_byte_array_allocated("Data",DATA_SIZE,Data)
                << tag_byte_array_allocated("BlockLight",BLOCKLIGHT_SIZE,BlockLight)
                << tag_byte_array_allocated("SkyLight",SKYLIGHT_SIZE,SkyLight)
                );
    }
};


struct MCAChunk
{
    typedef vector<MCAChunkSection*> SectionsList;

    WorldParams *params;
    int32_t xPos;
    int32_t zPos;
    int64_t LastUpdate;
    int8_t TerrainPopulated;
    SectionsList Sections;
    //Entities
    //TileEntities

    MCAChunk(int32_t x, int32_t z, WorldParams *_params) :
        params(_params),
        xPos(x),
        zPos(z),
        LastUpdate(0),
        TerrainPopulated(1),
        Sections()
    {
        for (int i=0; i<MAX_SECTIONS; i++)
            Sections.push_back(new MCAChunkSection(xPos, i, zPos, params));
    }

    ~MCAChunk()
    {
        for (SectionsList::iterator it=Sections.begin(); it!=Sections.end(); ++it)
            delete (*it);
    }

    nbt_node* toNBT()
    {
        // fill arrays
        uint8_t *Biomes = NULL;
        int32_t *HeightMap = NULL;
        Biomes = allocate_byte_array(BIOMES_SIZE);
        HeightMap = allocate_int_array(HEIGHTMAP_SIZE);

        params->chunkCB(xPos - params->startX, zPos - params->startZ, Biomes, HeightMap);

        NBTList sectlist;
        for (SectionsList::iterator it=Sections.begin(); it!=Sections.end(); ++it)
            sectlist << (*it)->toNBT();

        nbt_node *start = tag_compound("Level", NBTList()
                << tag_int("xPos",xPos)
                << tag_int("zPos",zPos)
                << tag_long("LastUpdate",LastUpdate)
                << tag_byte("TerrainPopulated",TerrainPopulated)
                << tag_byte_array_allocated("Biomes",BIOMES_SIZE,Biomes)
                << tag_int_array_allocated("HeightMap",HEIGHTMAP_SIZE,HeightMap)
                << tag_list("Sections", sectlist)
                );

        // wrap it in a root tag
        return tag_compound("", NBTList() << start);
    }
};


struct Region 
{
    WorldParams *params;
    // region coords (i.e. blocks/32)
    int xIndex;
    int zIndex;
    // start of 'chunks' array in absolute chunk coords
    int startX;
    int startZ;
    // dimensions of 'chunks' array
    int sizeX;
    int sizeZ;
    // 2d array of pointers: ZX order
    MCAChunk **chunks;

    Region(const int _xIndex, const int _zIndex, WorldParams *p) :
        params(p),
        xIndex(_xIndex),
        zIndex(_zIndex),
        startX(_xIndex * REGION_WIDTH),
        startZ(_zIndex * REGION_WIDTH),
        sizeX(REGION_WIDTH),
        sizeZ(REGION_WIDTH),
        chunks(NULL)
    {
        // bounds adjustment
        if (startX < p->startX) {
            sizeX -= p->startX - startX;
            startX = p->startX;
        }
        if (startZ < p->startZ) {
            sizeZ -= p->startZ - startZ;
            startZ = p->startZ;
        }
        if (startX + sizeX > p->startX + p->sizeX) {
            sizeX -= (startX + sizeX) - (p->startX + p->sizeX);
        }
        if (startZ + sizeZ > p->startZ + p->sizeZ) {
            sizeZ -= (startZ + sizeZ) - (p->startZ + p->sizeZ);
        }
        if (sizeX <= 0 || sizeZ <= 0) {
            // empty area
            sizeX = 0;
            sizeZ = 0;
            startX = 0;
            startZ = 0;
            chunks = NULL;
        } else {
            // create chunks
            chunks = new MCAChunk*[sizeZ * sizeX];
            for (int i = 0; i < sizeZ * sizeX; i++)
                chunks[i] = NULL;
            for (int iz = 0; iz < sizeZ; iz++)
            for (int ix = 0; ix < sizeX; ix++) {
                chunks[iz * sizeX + ix] = new MCAChunk(startX + ix, startZ + iz, p);
            }

        }
    }

    ~Region()
    {
        if (chunks) {
            for (int i = 0; i < sizeZ * sizeX; i++)
                if (chunks[i])
                    delete chunks[i];
            delete[] chunks;
        }
    }

    ERR writeToFile() const
    {
        // fill buffers with compressed chunk data
        buffer bufs[sizeZ * sizeX];
        //buffer rawbufs[sizeZ * sizeX]; // DEBUG
        for (int i = 0; i < sizeZ * sizeX; i++) {
            nbt_node *chunknbt = chunks[i]->toNBT();
            bufs[i] = nbt_dump_compressed(chunknbt, STRAT_INFLATE);
            //rawbufs[i] = nbt_dump_binary(chunknbt); // DEBUG
            nbt_free(chunknbt);
            if (bufs[i].data == NULL)
                return ERR::NBT_ERROR;
        }

        /*
        // DEBUG: output uncompressed NBT for inspection
        FILE *chunkfile = fopen("chunkNBTs", "ab");
        for (int i = 0; i < sizeZ * sizeX; i++)
            fwrite(rawbufs[i].data, 1, rawbufs[i].len, chunkfile);
        fclose(chunkfile);
        */

        // build filename
        ostringstream oss;
        oss << "r." << xIndex << "." << zIndex << ".mca";
        string filename = oss.str();

        // open file
        ERR result = ERR::NONE;
        FILE *outfile = fopen(filename.c_str(), "wb");
        if (!outfile) {
            result = ERR::OPEN_FILE;
        } else {
            // write file
            uint32_t currentOffset = 2; // sectors
            uint32_t offsets[sizeZ * sizeX];

            // header (8192 bytes)
            for (int j = 0; j < 2048; j++)
                writeInt(0, outfile);

            // locations
            const int sx = startX - xIndex * 32;
            const int sz = startZ - zIndex * 32;
            for (int iz = 0; iz < sizeZ; iz++)
            for (int ix = 0; ix < sizeX; ix++) {
                const int i = iz * sizeX + ix;
                uint8_t numSectors = (bufs[i].len + 5 + 4095)/4096;
                int32_t location = (currentOffset << 8) | numSectors;
                fseek(outfile, 4 * ((sz + iz) * 32 + sx + ix), SEEK_SET);
                writeInt(location, outfile);
                offsets[i] = currentOffset; // store offset for later
                currentOffset += numSectors; // offset for next chunk
            }

            // timestamps
            /*
            for (int i = 0; i < sizeZ * sizeX; i++)
                writeInt(0, outfile);
            */

            // chunk data
            fseek(outfile, 8192, SEEK_SET);
            for (int i = 0; i < sizeZ * sizeX; i++) {
                // check that we get same offset as before
                // (we should, if we did our math correctly)
                unsigned long pos = ftell(outfile);
                if (pos != offsets[i] * 4096) {
                    result = ERR::WRITING_CHUNKS;
                    break;
                }
                // write compressed chunk data
                writeInt(bufs[i].len + 1, outfile); // chunk data length
                writeByte(2, outfile); // compression type
                fwrite(bufs[i].data, 1, bufs[i].len, outfile); // actual data
                // pad with zeroes
                int padding = 4096 - ((bufs[i].len + 5) % 4096);
                for (int j = 0; j < padding; j++)
                    writeByte(0, outfile);
            }

            fclose(outfile);
        }

        for (int i = 0; i < sizeZ * sizeX; i++)
            free(bufs[i].data);

        return result;
    }
};


struct World
{
    string name;
    WorldParams params;

    // parameters measured in chunks
    World(const char *_name, WorldParams _params) :
        name(_name),
        params(_params)
    { }

    ERR writeToDir(const char *dirName)
    {
        if (exists(dirName))
            return ERR::PATH_EXISTS;

        cout << "exporting..." << endl;

        // create world dir
        create_directory(dirName);
        current_path(dirName);

        // create level.dat structure
        LevelDat *leveldat = new LevelDat();

        // get level.dat NBT
        nbt_node *levelnbt = leveldat->toNBT();

        // open level.dat for writing
        ERR result = ERR::NONE;
        nbt_status nbterr;
        FILE *outfile = fopen("level.dat", "wb");
        if (!outfile) {
            result = ERR::OPEN_FILE;
        } else {
            // write file
            nbterr = nbt_dump_file(levelnbt, outfile, STRAT_GZIP);
            fclose(outfile);
        }

        nbt_free(levelnbt);
        delete leveldat;

        if (result != ERR::NONE)
            return result;
        if (nbterr != NBT_OK)
            return ERR::NBT_ERROR;

        // create region subdir
        create_directory("region");
        current_path("region");

        // write region files
        int rgnMinX = params.startX >> 5;
        int rgnMinZ = params.startZ >> 5;
        int rgnMaxX = (params.startX + params.sizeX - 1) >> 5;
        int rgnMaxZ = (params.startZ + params.sizeZ - 1) >> 5;
        for (int iz = rgnMinZ; iz <= rgnMaxZ; iz++)
        for (int ix = rgnMinX; ix <= rgnMaxX; ix++) {
            // short-lived region instance
            Region *rgn = new Region(ix, iz, &params);
            ERR result = rgn->writeToFile();
            delete rgn;
            if (result != ERR::NONE)
                return result;
        }

        return ERR::NONE;
    }
};


ERR canExport(const char *worldName)
{
    if (exists(worldName))
        return ERR::PATH_EXISTS;
    return ERR::NONE;
}

// size in chunks
ERR exportWorld(const char *worldName, int size, ChunkCallback chunkCB, SectionCallback sectionCB)
{
    ERR result = canExport(worldName);
    if (result != ERR::NONE)
        return result;

    World *world = new World(worldName, WorldParams(size, chunkCB, sectionCB));

    result = world->writeToDir(worldName);

    delete world;

    return result;
}

