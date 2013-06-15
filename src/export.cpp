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


const int REGION_WIDTH = 32; // chunks
const int CHUNK_WIDTH = 16;
const int CHUNK_HEIGHT = 256;
const int SECTION_HEIGHT = 16;
const int MAX_SECTIONS = CHUNK_HEIGHT / SECTION_HEIGHT;

const int BIOMES_SIZE = CHUNK_WIDTH * CHUNK_WIDTH;
const int HEIGHTMAP_SIZE = CHUNK_WIDTH * CHUNK_WIDTH; // ints
const int BLOCKS_SIZE = CHUNK_WIDTH * CHUNK_WIDTH * SECTION_HEIGHT;
const int DATA_SIZE = BLOCKS_SIZE/2;
const int BLOCKLIGHT_SIZE = BLOCKS_SIZE/2;
const int SKYLIGHT_SIZE = BLOCKS_SIZE/2;


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
    const BlockArray& b;
    BlockArray sl; // skylight
    BlockArray bl; // blocklight
    int32_t *hm; // heightmap, ZX order!

    const int sizeX, sizeZ, startX, startZ; // in chunks

    // dimensions in chunks
    WorldParams(const BlockArray& _b) :
        b(_b),
        sl(_b.xSize, _b.zSize),
        bl(_b.xSize, _b.zSize),
        hm(NULL),
        // assume size is multiple of 16
        sizeX(_b.xSize/16),
        sizeZ(_b.zSize/16),
        // center world on origin
        startX(-(sizeX/2)),
        startZ(-(sizeZ/2))
    {
        hm = new int32_t[b.zSize * b.xSize];

        cout << "lighting..." << endl;

        deque<Vec3> q, qnext; // processing queue

        // clear light maps
        std::fill_n(sl.buf, sl.xSize * sl.zSize * sl.ySize, 0);
        std::fill_n(bl.buf, bl.xSize * bl.zSize * bl.ySize, 0);

        // calculate heightmap and lighting
        for (int x = 0; x < b.xSize; x++)
        for (int z = 0; z < b.zSize; z++) {
            int y = b.ySize - 1;
            // while going through air
            while (y >= 0 && b.get(x,y,z) == 0) {
                // set skylight
                sl(x,y,z) = 15;
                y--;
            }
            // set heightmap
            hm[z * b.xSize + x] = y + 1;
        }

        // initialize lighting queue
        for (int x = 1; x < b.xSize - 1; x++)
        for (int z = 1; z < b.zSize - 1; z++) {
            int hi = max({
                    hm[(z)*b.xSize + x-1],
                    hm[(z)*b.xSize + x+1],
                    hm[(z-1)*b.xSize + x],
                    hm[(z+1)*b.xSize + x]});
            int lo = max(hm[z*b.xSize + x], 1);
            for (int y = hi - 1; y >= lo; y--)
                q.emplace_back(x,y,z); // add to queue
        }

        // perform lighting
        for (int i = 14; i > 0; i--) {
            for (deque<Vec3>::iterator it = q.begin(); it != q.end(); ++it) {
#define LIGHTING_TRY(x, y, z) \
                if (b.get(x,y,z) == 0 && sl(x,y,z) < i) { \
                    sl(x,y,z) = i; \
                    if (x>0 && y>0 && z>0 \
                            && x<b.xSize-1 && y<b.ySize-1 && z<b.zSize-1) \
                        qnext.emplace_back(x,y,z); \
                }
                int x = it->x;
                int y = it->y;
                int z = it->z;
                LIGHTING_TRY(x+1,y,z);
                LIGHTING_TRY(x-1,y,z);
                LIGHTING_TRY(x,y+1,z);
                LIGHTING_TRY(x,y-1,z);
                LIGHTING_TRY(x,y,z+1);
                LIGHTING_TRY(x,y,z-1);
#undef LIGHTING_TRY
            }
            q.clear();
            q.swap(qnext);
        }

    }

    ~WorldParams()
    {
        if (hm)
            delete[] hm;
    }
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
    int8_t yIndex;
    int8_t *Blocks;
    int8_t *Data;
    int8_t *BlockLight;
    int8_t *SkyLight;

    MCAChunkSection(int _yIndex, int xPos, int zPos, const WorldParams& params) :
        yIndex(_yIndex),
        Blocks(NULL),
        Data(NULL),
        BlockLight(NULL),
        SkyLight(NULL)
    {
        Blocks = new int8_t[BLOCKS_SIZE];
        Data = new int8_t[DATA_SIZE];
        BlockLight = new int8_t[BLOCKLIGHT_SIZE];
        SkyLight = new int8_t[SKYLIGHT_SIZE];

        const int sx = (xPos - params.startX) * 16;
        const int sz = (zPos - params.startZ) * 16;
        const int sy = yIndex * 16;
        for (int y=0; y<SECTION_HEIGHT; y++)
        for (int z=0; z<CHUNK_WIDTH; z++)
        for (int x=0; x<CHUNK_WIDTH; x++) {
            const int i = y * CHUNK_WIDTH * CHUNK_WIDTH + z * CHUNK_WIDTH + x;
            // copy in blockids and light
            Blocks[i] = params.b.get(sx+x, sy+y, sz+z);
            if (i & 1) {
                Data[i/2] = 0;
                BlockLight[i/2] = 0;
                SkyLight[i/2] = (params.sl.get(sx+x, sy+y, sz+z) << 4) | params.sl.get(sx+x-1, sy+y, sz+z);
            }
        }
    }

    ~MCAChunkSection()
    {
        if (Blocks)
            delete[] Blocks;
        if (Data)
            delete[] Data;
        if (BlockLight)
            delete[] BlockLight;
        if (SkyLight)
            delete[] SkyLight;
    }

    nbt_node* toNBT()
    {
        // important to use NULL for name when it'll be a tag_list entry
        return tag_compound(NULL, NBTList()
                << tag_byte("Y",yIndex)
                << tag_byte_array("Blocks",BLOCKS_SIZE,Blocks)
                << tag_byte_array("Data",DATA_SIZE,Data)
                << tag_byte_array("BlockLight",BLOCKLIGHT_SIZE,BlockLight)
                << tag_byte_array("SkyLight",SKYLIGHT_SIZE,SkyLight)
                );
    }
};


struct MCAChunk
{
    typedef vector<MCAChunkSection*> SectionsList;

    int32_t xPos;
    int32_t zPos;
    int64_t LastUpdate;
    int8_t TerrainPopulated;
    int8_t *Biomes;
    int32_t *HeightMap;
    SectionsList Sections;
    //Entities
    //TileEntities

    MCAChunk(int32_t x, int32_t z, const WorldParams& params) :
        xPos(x),
        zPos(z),
        LastUpdate(0),
        TerrainPopulated(1),
        Biomes(NULL),
        HeightMap(NULL),
        Sections()
    {
        Biomes = new int8_t[BIOMES_SIZE];
        HeightMap = new int32_t[HEIGHTMAP_SIZE];
        
        for (int i=0; i<BIOMES_SIZE; i++)
            Biomes[i] = 0;
        
        const int sx = (xPos - params.startX) * 16;
        const int sz = (zPos - params.startZ) * 16;
        for (int z=0; z<CHUNK_WIDTH; z++)
        for (int x=0; x<CHUNK_WIDTH; x++)
            HeightMap[z*CHUNK_WIDTH + x] = params.hm[(sz+z)*params.b.xSize + sx+x];
        
        for (int i=0; i<MAX_SECTIONS; i++)
            Sections.push_back(new MCAChunkSection(i, xPos, zPos, params));
    }

    ~MCAChunk()
    {
        if (Biomes)
            delete[] Biomes;
        if (HeightMap)
            delete[] HeightMap;
        for (SectionsList::iterator it=Sections.begin(); it!=Sections.end(); ++it)
            delete (*it);
    }

    nbt_node* toNBT()
    {
        NBTList sectlist;
        for (SectionsList::iterator it=Sections.begin(); it!=Sections.end(); ++it)
            sectlist << (*it)->toNBT();

        nbt_node *start = tag_compound("Level", NBTList()
                << tag_int("xPos",xPos)
                << tag_int("zPos",zPos)
                << tag_long("LastUpdate",LastUpdate)
                << tag_byte("TerrainPopulated",TerrainPopulated)
                //<< tag_byte_array("Biomes",BIOMES_SIZE,Biomes)
                << tag_int_array("HeightMap",HEIGHTMAP_SIZE,HeightMap)
                << tag_list("Sections", sectlist)
                );

        // wrap it in a root tag
        return tag_compound("", NBTList() << start);
    }
};


struct Region 
{
    // region coords (i.e. blocks/32)
    int xIndex;
    int zIndex;
    // start of 'chunks' array in absolute chunk coords
    int startX;
    int startZ;
    // dimensions of 'chunks' array
    int sizeX;
    int sizeZ;
    // references contents of World::chunks
    MCAChunk **chunks;

    Region(const int _xIndex, const int _zIndex, const WorldParams& p, MCAChunk** wchunks) :
        xIndex(_xIndex),
        zIndex(_zIndex),
        startX(_xIndex * REGION_WIDTH),
        startZ(_zIndex * REGION_WIDTH),
        sizeX(REGION_WIDTH),
        sizeZ(REGION_WIDTH),
        chunks(NULL)
    {
        // bounds adjustment
        if (startX < p.startX) {
            sizeX -= p.startX - startX;
            startX = p.startX;
        }
        if (startZ < p.startZ) {
            sizeZ -= p.startZ - startZ;
            startZ = p.startZ;
        }
        if (startX + sizeX > p.startX + p.sizeX) {
            sizeX -= (startX + sizeX) - (p.startX + p.sizeX);
        }
        if (startZ + sizeZ > p.startZ + p.sizeZ) {
            sizeZ -= (startZ + sizeZ) - (p.startZ + p.sizeZ);
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
            // offsets between world's and this region's 'chunks' arrays
            const int dz = startZ - p.startZ;
            const int dx = startX - p.startX;
            // raw 2D access!
            for (int iz = 0; iz < sizeZ; iz++)
            for (int ix = 0; ix < sizeX; ix++)
                chunks[iz * sizeX + ix] = wchunks[(dz + iz) * p.sizeX + (dx + ix)];
        }
    }

    ~Region()
    {
        // don't delete actual chunks (taken care of by World)
        if (chunks)
            delete[] chunks;
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

    LevelDat *leveldat;
    // 2d array of pointers: ZX order
    MCAChunk **chunks;

    // parameters measured in chunks
    World(const char *_name, const BlockArray& b) :
        name(_name),
        params(b),
        leveldat(NULL),
        chunks(NULL)
    {
        cout << "converting world..." << endl;

        // create level.dat structure
        leveldat = new LevelDat();

        // create chunks
        chunks = new MCAChunk*[params.sizeZ * params.sizeX];
        for (int i = 0; i < params.sizeZ * params.sizeX; i++)
            chunks[i] = NULL;
        for (int iz = 0; iz < params.sizeZ; iz++)
        for (int ix = 0; ix < params.sizeX; ix++) {
            //bool empty = ix < padding || iz < padding || sizeX - ix <= padding || sizeZ - iz <= padding;
            chunks[iz * params.sizeX + ix] = new MCAChunk(params.startX + ix, params.startZ + iz, params);
        }
    }

    ~World()
    {
        if (chunks) {
            for (int i = 0; i < params.sizeZ * params.sizeX; i++)
                if (chunks[i])
                    delete chunks[i];
            delete[] chunks;
        }
    }

    ERR writeToDir(const char *dirName)
    {
        if (exists(dirName))
            return ERR::PATH_EXISTS;

        cout << "exporting..." << endl;

        // create world dir
        create_directory(dirName);
        current_path(dirName);

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
            Region *rgn = new Region(ix, iz, params, chunks);
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

ERR exportWorld(const char *worldName, const BlockArray& b)
{
    ERR result = canExport(worldName);
    if (result != ERR::NONE)
        return result;

    World *world = new World(worldName, b);

    result = world->writeToDir(worldName);

    delete world;

    return result;
}

