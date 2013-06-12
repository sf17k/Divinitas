#ifndef H_ERROR
#define H_ERROR

// FIXME: crappy error handling
enum ERR {
    NONE = 0,
    PATH_EXISTS,
    NBT_ERROR,
    OPEN_FILE,
    WRITING_CHUNKS,
};

#endif

