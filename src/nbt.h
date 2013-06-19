#ifndef H_NBT
#define H_NBT

/* supplementary NBT stuff.
 *
 * be sure to free your nbt nodes with nbt_free()!
 */

#include <nbt/nbt.h> // cNBT
#include <vector>
//#include <cstdint>

/* list builder class for passing multiple args conveniently.
 *
 * IMPORTANT: use only to construct tag_list() and tag_compound(),
 * so they will take responsibility for freeing the memory.
 * (it's not done automatically!)
 *
 * IMPORTANT: if passing to tag_list(), each element must have a NULL name,
 * or the resulting nbt file will be corrupt
 */
struct NBTList {
    std::vector<nbt_node*> v;

    NBTList& operator<<(nbt_node* elem) {
        v.push_back(elem);
        return *this;
    }
};

// duplicated from cNBT
struct nbt_list {
    nbt_node* data;
    list_head entry;
};

uint8_t* allocate_byte_array(int32_t length);
int32_t* allocate_int_array(int32_t length);

nbt_node *tag_byte(const char *name, int8_t);
nbt_node *tag_short(const char *name, int16_t);
nbt_node *tag_int(const char *name, int32_t);
nbt_node *tag_long(const char *name, int64_t);
nbt_node *tag_float(const char *name, float);
nbt_node *tag_double(const char *name, double);
nbt_node *tag_string(const char *name, const char* str);
//nbt_node *tag_byte_array(const char *name, int32_t length, const uint8_t* data);
//nbt_node *tag_int_array(const char *name, int32_t length, const int32_t* data);
nbt_node *tag_byte_array_allocated(const char *name, int32_t length, uint8_t* data);
nbt_node *tag_int_array_allocated(const char *name, int32_t length, int32_t* data);
nbt_node *tag_list(const char *name, NBTList entries);
nbt_node *tag_compound(const char *name, NBTList entries);


#endif

