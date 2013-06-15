#ifndef H_EXPORT
#define H_EXPORT

#include "error.h"

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
    inline unsigned char& operator()(const int x, const int y, const int z)
    {
        return buf[x * zSize * ySize + z * ySize + y];
    }

    // unchecked!
    inline unsigned char get(const int x, const int y, const int z) const
    {
        return buf[x * zSize * ySize + z * ySize + y];
    }
};


ERR canExport(const char *worldName);
ERR exportWorld(const char *worldName, const BlockArray& b);

#endif

