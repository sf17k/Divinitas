#include "generate.h"

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
        b(ix,iy,iz) = id;
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
            b(ix+dx*i,iy+dy*i,iz+dz*i) = b(ix,iy,iz);
}


/* generates a square world 'size' chunks to a side,
 * with width 'voidPadding' of empty chunks on all sides.
 * 'worldName' is both directory name and in-game name.
 */
ERR generateWorld(const char *worldName, const int size, const int voidPadding)
{
    ERR result = canExport(worldName);
    if (result != ERR::NONE)
        return result;

    std::cout << "generating..." << std::endl;

    const int fullSize = size + voidPadding * 2; // inner padding
    const int mx = fullSize * 16;
    const int mz = fullSize * 16;
    const int my = 256;
    const int pd = voidPadding * 16; // padding in blocks

    BlockArray b(mx, mz); // our world representation
    MTRand rng; // random number generator

    clear(b);

    // make some stone
    for (int x = pd; x < mx - pd; x++)
    for (int z = pd; z < mz - pd; z++)
    for (int y = 0; y < my; y++) {
        b(x,y,z) = y < 128 ? 1 : 0;
    }

    // make random boxes near ground level
    const int BOXES_PER_CHUNK = 64;
    const int MAX_BOX_SIZE = 8;
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


    // export
    result = exportWorld(worldName, b);

    return result;
}

