#include "generate.h"

#include "lithosphere.hpp" // platec
#include "export.h"
#include "MersenneTwister.h"
#include <algorithm>
#include <iostream>


// fill with zeroes
inline void clear(BlockArray& b) { 
    std::fill_n(b.buf, b.xSize * b.zSize * b.ySize, 0);
}

// unchecked!
inline void makeBox(BlockArray& b,
        int x, int y, int z, int sx, int sy, int sz, int id) {
    for (int ix = x; ix < x + sx; ix++)
    for (int iz = z; iz < z + sz; iz++)
    for (int iy = y; iy < y + sy; iy++)
        b.set(ix,iy,iz, id);
}

// unchecked!
inline void duplicateBox(BlockArray& b,
        int x, int y, int z, int sx, int sy, int sz,
        int dx, int dy, int dz, int iterations) {
    for (int i = 1; i <= iterations; i++)
    for (int ix = x; ix < x + sx; ix++)
    for (int iz = z; iz < z + sz; iz++)
    for (int iy = y; iy < y + sy; iy++)
        if(b(ix,iy,iz))
            b.set(ix+dx*i, iy+dy*i, iz+dz*i, b(ix,iy,iz));
}



BlockArray gen1(const int size, const int voidPadding)
{
    const int fullSize = size + voidPadding * 2; // inner padding
    const int mx = fullSize * 16;
    const int mz = fullSize * 16;
    const int my = 256;
    const int pd = voidPadding * 16; // padding in blocks

    BlockArray b(mx, mz); // our world representation
    clear(b);

    MTRand rng; // random number generator

    // make some stone
    for (int x = pd; x < mx - pd; x++)
    for (int z = pd; z < mz - pd; z++)
    for (int y = 0; y < my; y++) {
        b.set(x,y,z, y < 128 ? 1 : 0);
    }

    // make random boxes near ground level
    //const int BOXES_PER_CHUNK = 64;
    //const int MAX_BOX_SIZE = 8;
    const int DUPES_PER_CHUNK = 8;
    const int MAX_DUP_SIZE = 10;
    const int MAX_DUP_ITERATIONS = 6;

    const int rangex = mx - pd * 2 - 1;
    const int rangez = mz - pd * 2 - 1;
    /*
    for (int i = 0; i < size*size*BOXES_PER_CHUNK; i++) {
        int sizex = rng.randInt(MAX_BOX_SIZE - 1) + 1;
        int sizey = rng.randInt(MAX_BOX_SIZE - 1) + 1;
        int sizez = rng.randInt(MAX_BOX_SIZE - 1) + 1;
        int id = rng.randInt(1) * rng.randInt(5);
        makeBox(b,
                rng.randInt(rangex - sizex - 1) + pd,
                rng.randInt(MAX_BOX_SIZE*2) - rng.randInt(MAX_BOX_SIZE*3) + 128,
                rng.randInt(rangez - sizez - 1) + pd,
                sizex, sizey, sizez, id);
    }
    */

    // copypaste boxes
    // XXX: likely to generate outside bounds!
    for (int i = 0; i < size*size*DUPES_PER_CHUNK; i++) {
        int sizex = rng.randInt(MAX_DUP_SIZE - 1) + 1;
        int sizey = rng.randInt(MAX_DUP_SIZE - 1) + 1;
        int sizez = rng.randInt(MAX_DUP_SIZE - 1) + 1;
        duplicateBox(b,
                rng.randInt(rangex - sizex - 1) + pd,
                rng.randInt(MAX_DUP_SIZE) - MAX_DUP_SIZE/2 + 128,
                rng.randInt(rangez - sizez - 1) + pd,
                sizex, sizey, sizez,
                rng.randInt(sizex) - rng.randInt(sizex),
                rng.randInt(sizey),
                rng.randInt(sizez) - rng.randInt(sizez),
                rng.randInt(MAX_DUP_ITERATIONS - 3) + 3);
    }

    return b;
}



// platec stuff

#define COLOR_STEP	1.5f
#define HEIGHT_TOP	(6.0f * COLOR_STEP)

#define MAX_MAP_SIDE	4096
#define MIN_MAP_SIDE	64
#define DEFAULT_MAP_SIDE 512

#define MAX_PLATES	1024
#define MIN_PLATES	2
#define DEFAULT_NUM_PLATES 10

#define DEFAULT_AGGR_OVERLAP_ABS	5000
#define DEFAULT_AGGR_OVERLAP_REL	0.10f
#define DEFAULT_CYCLE_COUNT		2
#define DEFAULT_EROSION_PERIOD		60
#define DEFAULT_FOLDING_RATIO		0.001f
#define DEFAULT_SEA_LEVEL		0.65f

// returns new heightmap (delete it yourself)
float *runPlatec(
    size_t num_plates,
    size_t map_side,
    size_t aggr_overlap_abs,
    float aggr_overlap_rel,
    size_t cycle_count,
    size_t erosion_period,
    float folding_ratio,
    float sea_level)
{
    lithosphere* world;

	#define CHECK_RANGE(_DEST, _TYPE, _FORMAT, _C, _MIN, _MAX, _DEFAULT) \
	do { \
        _TYPE val = _DEST; \
        if (val > _MAX) \
            printf("Value of %c is too large: " \
                "setting it to " _FORMAT ".\n", \
                _C, val = _MAX); \
        else if (val < _MIN) \
            printf("Value of %c is too small: " \
                "setting it to " _FORMAT ".\n", \
                _C, val = _MIN); \
        _DEST = val; \
	} while (0)

	CHECK_RANGE(map_side, size_t, "%u", 'l', MIN_MAP_SIDE,
		MAX_MAP_SIDE, DEFAULT_MAP_SIDE);

	if (map_side & (map_side - 1))
		printf("Length of map's side must be a power "
		       "of two! Using default value %d.\n",
		       map_side = DEFAULT_MAP_SIDE);

	CHECK_RANGE(num_plates, size_t, "%u", 'n', MIN_PLATES,
		MAX_PLATES, DEFAULT_NUM_PLATES);

	CHECK_RANGE(aggr_overlap_abs, size_t, "%u", 'a', 0,
		MAX_MAP_SIDE * MAX_MAP_SIDE, DEFAULT_AGGR_OVERLAP_ABS);
	CHECK_RANGE(cycle_count, size_t, "%u", 'c', 0,
		(size_t)(-1), DEFAULT_CYCLE_COUNT);
	CHECK_RANGE(erosion_period, size_t, "%u", 'e', 0,
		(size_t)(-1), DEFAULT_EROSION_PERIOD);
	CHECK_RANGE(folding_ratio, float, "%f", 'f', 0.0f,
		1.0f, DEFAULT_AGGR_OVERLAP_REL);
	CHECK_RANGE(aggr_overlap_rel, float, "%f", 'r', 0.0f,
		1.0f, DEFAULT_FOLDING_RATIO);
	CHECK_RANGE(sea_level, float, "%f", 's', 0.0f,
		CONTINENTAL_BASE, DEFAULT_SEA_LEVEL);

	printf("map:\t\t%u\nsea:\t\t%f\nplates:\t\t%u\nerosion period:\t%u\n"
	       "folding:\t%f\noverlap abs:\t%u\noverlap rel:\t%f\n"
	       "cycles:\t\t%u\n", map_side, sea_level, num_plates,
	       erosion_period, folding_ratio, aggr_overlap_abs,
	       aggr_overlap_rel, cycle_count);

	srand(time(0));

	world = new lithosphere(map_side, sea_level, erosion_period,
		folding_ratio, aggr_overlap_abs, aggr_overlap_rel, cycle_count);
	world->createPlates(num_plates);

    // main loop
    while (world->getPlateCount()) {
        world->update();
    }

    const float *hmap = world->getTopography();
    float *hmapCopy = new float[map_side*map_side];
    std::copy_n(hmap, map_side*map_side, hmapCopy);

	delete world;
	return hmapCopy;
}


BlockArray genPlatec(const int size, const int voidPadding,
        const int scaleh, const int scalev)
{
    const int fullSize = size + voidPadding * 2; // inner padding
    const int mx = fullSize * 16;
    const int mz = fullSize * 16;
    const int my = 256;
    const int pd = voidPadding * 16; // padding in blocks

    const float yscale = 256.0f/(float)scalev;

    const int map_side = (mz - pd*2) / scaleh;
    const float sea_level = DEFAULT_SEA_LEVEL;

    // get heightmap from platec (be sure to delete it)
    float *hm = runPlatec(
            DEFAULT_NUM_PLATES,
            map_side,
            DEFAULT_AGGR_OVERLAP_ABS,
            DEFAULT_AGGR_OVERLAP_REL,
            DEFAULT_CYCLE_COUNT,
            DEFAULT_EROSION_PERIOD,
            DEFAULT_FOLDING_RATIO,
            sea_level);


    BlockArray b(mx, mz); // our world representation
    clear(b);

    //MTRand rng; // random number generator

    // convert heightmap to blockids
    for (int x = 0; x < mx - pd*2; x++)
    for (int z = 0; z < mz - pd*2; z++)
    for (int y = 0; y < my; y++) {
        float val = (y - 64) * yscale/256.0f;
        b.set(x+pd, y, z+pd,
                val < hm[(x/scaleh)*map_side + z/scaleh] ? 1 : (val < sea_level ? 9 : 0));
    }

    delete[] hm;
    return b;
}



/* generates a square world 'size' chunks to a side,
 * with width 'voidPadding' of empty chunks on all sides.
 * 'worldName' is both directory name and in-game name.
 */
ERR generateWorld(const char *worldName, const int size, const int voidPadding,
        int pt_scaleh, int pt_scalev)
{
    ERR result = canExport(worldName);
    if (result != ERR::NONE)
        return result;

    std::cout << "generating..." << std::endl;

    // generate
    //BlockArray b = gen1(size, voidPadding);
    BlockArray b = genPlatec(size, voidPadding, pt_scaleh, pt_scalev);

    // export
    result = exportWorld(worldName, b);

    return result;
}

