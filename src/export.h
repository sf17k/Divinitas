#ifndef H_EXPORT
#define H_EXPORT

#include "error.h"
#include <cstdint>
#include <iostream>//DEBUG


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


/* should fill heightmap with 16x16 ints in ZX order and
 * biomes with 16x16 bytes in XZ order
 */
typedef void (*ChunkCallback)(int x, int z, uint8_t *biomes, int32_t *heightmap);

/* should fill blocks with 16x16x16 bytes in YZX order and
 * data, blocklight, skylight with 16x16x16 half-bytes in YZX order
 */
typedef void (*SectionCallback)(int x, int y, int z,
        uint8_t *blocks, uint8_t *data, uint8_t *blocklight, uint8_t *skylight);


/* XZY ordered byte array, assume height is 256
 */
struct BlockArray
{
    unsigned char *buf;
    const int xSize;
    const int zSize;
    const int ySize;

    BlockArray(int _xSize, int _zSize) :
        buf(0), xSize(_xSize), zSize(_zSize), ySize(256)
    {
        buf = new unsigned char[xSize * zSize * ySize];
    }

    ~BlockArray()
    {
        if (buf)
            delete[] buf;
    }

    // unchecked!
    inline unsigned char operator()(const int x, const int y, const int z) const
    {
        return buf[x * zSize * ySize + z * ySize + y];
    }

    // unchecked!
    inline void set(const int x, const int y, const int z, const int id)
    {
        buf[x * zSize * ySize + z * ySize + y] = id;
    }
};

/* ZX ordered height map of floats
 */
struct Heightmap
{
    float *buf;
    const int size;

    Heightmap(int _size) :
        buf(0), size(_size)
    {
        buf = new float[size * size];
    }

    ~Heightmap()
    {
        if (buf)
            delete[] buf;
    }

    // unchecked!
    inline float get(const int x, const int z) const
    {
        return buf[z * size + x];
    }

    // unchecked!
    inline void set(const int x, const int z, const float val)
    {
        buf[z * size + x] = val;
    }
};


ERR canExport(const char *worldName);
ERR exportWorld(const char *worldName, int size, ChunkCallback chunkCB, SectionCallback sectionCB);

#endif

