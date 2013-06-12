#include "generate.h"

#include "export.h"

/* generates a square world 'size' chunks to a side,
 * with width 'voidPadding' of empty chunks on all sides.
 * 'worldName' is both directory name and in-game name.
 */
ERR generateWorld(const char *worldName, int size, int voidPadding)
{
    ERR result = canExport(worldName);
    if (result != ERR::NONE)
        return result;

    size += voidPadding * 2; // inner padding
    const int mx = size * 16;
    const int mz = size * 16;
    const int my = 256;
    const int pd = voidPadding * 16;

    BlockArray b(mx, mz);

    // empty space on four sides
    for (int x = 0; x < pd; x++)
    for (int z = 0; z < mz; z++)
    for (int y = 0; y < my; y++)
        b(x,y,z) = 0;
    for (int x = mx - pd; x < mx; x++)
    for (int z = 0; z < mz; z++)
    for (int y = 0; y < my; y++)
        b(x,y,z) = 0;
    for (int x = pd; x < mx - pd; x++)
    for (int z = 0; z < pd; z++)
    for (int y = 0; y < my; y++)
        b(x,y,z) = 0;
    for (int x = pd; x < mx - pd; x++)
    for (int z = mz - pd; z < mz; z++)
    for (int y = 0; y < my; y++)
        b(x,y,z) = 0;

    // fill in the middle
    for (int x = pd; x < mx - pd; x++)
    for (int z = pd; z < mz - pd; z++)
    for (int y = 0; y < my; y++) {
        b(x,y,z) = y < 16 ? 1 : 0;
    }

    result = exportWorld(worldName, b);

    return result;
}

