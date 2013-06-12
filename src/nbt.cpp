#include "nbt.h"

#include <cstdlib>
#include <cstring>
using namespace std;

// assumes all entries are the type of the first
nbt_list *list_payload(NBTList list)
{
    nbt_list *ret = (nbt_list*)malloc(sizeof *ret);
    ret->data = (nbt_node*)malloc(sizeof *ret->data);
    ret->data->type = TAG_BYTE;
    ret->data->name = NULL;
    INIT_LIST_HEAD(&ret->entry);

    vector<nbt_node*> *entries = &list.v;

    if (!entries->empty())
        ret->data->type = entries->front()->type;

    for (vector<nbt_node*>::iterator it = entries->begin(); it != entries->end(); ++it) {
        nbt_list *elem = (nbt_list*)malloc(sizeof *elem);
        // take over memory management for NBTList (it does not manage by itself)
        elem->data = *it;
        list_add_tail(&elem->entry, &ret->entry);
    }
    return ret;
}

nbt_list *compound_payload(NBTList list)
{
    nbt_list *ret = (nbt_list*)malloc(sizeof *ret);
    ret->data = NULL;
    INIT_LIST_HEAD(&ret->entry);

    vector<nbt_node*> *entries = &list.v;

    for (vector<nbt_node*>::iterator it = entries->begin(); it != entries->end(); ++it) {
        nbt_list *elem = (nbt_list*)malloc(sizeof *elem);
        // take over memory management for NBTList (it does not manage by itself)
        elem->data = *it;
        list_add_tail(&elem->entry, &ret->entry);
    }
    return ret;
}

inline nbt_node *new_node(const char *name, nbt_type type)
{
    nbt_node *node = (nbt_node*)malloc(sizeof *node);
    node->type = type;
    node->name = NULL;
    if (name) {
        size_t len = strlen(name)+1;
        node->name = (char*)malloc(len);
        strcpy(node->name, name);
    }
    return node;
}

#define NBT_TAG_CONSTRUCTOR(payload_name, nbt_type, payload_type) \
nbt_node *payload_name(const char *name, payload_type payload) { \
    nbt_node *node = new_node(name, nbt_type); \
    node->payload.payload_name = payload; \
    return node; \
}

NBT_TAG_CONSTRUCTOR(tag_byte, TAG_BYTE, int8_t)
NBT_TAG_CONSTRUCTOR(tag_short, TAG_SHORT, int16_t)
NBT_TAG_CONSTRUCTOR(tag_int, TAG_INT, int32_t)
NBT_TAG_CONSTRUCTOR(tag_long, TAG_LONG, int64_t)
NBT_TAG_CONSTRUCTOR(tag_float, TAG_FLOAT, float)
NBT_TAG_CONSTRUCTOR(tag_double, TAG_DOUBLE, double)

nbt_node *tag_string(const char *name, const char* str) {
    nbt_node *node = new_node(name, TAG_STRING);
    size_t payloadlen = strlen(str)+1;
    node->payload.tag_string = (char*)malloc(payloadlen);
    strcpy(node->payload.tag_string, str);
    return node;
}
nbt_node *tag_byte_array(const char *name, int32_t length, const int8_t* data) {
    nbt_node *node = new_node(name, TAG_BYTE_ARRAY);
    node->payload.tag_byte_array.length = length;
    node->payload.tag_byte_array.data = (unsigned char*)malloc(length);
    memcpy(node->payload.tag_byte_array.data, data, length);
    return node;
}
nbt_node *tag_int_array(const char *name, int32_t length, const int32_t* data) {
    nbt_node *node = new_node(name, TAG_INT_ARRAY);
    node->payload.tag_int_array.length = length;
    node->payload.tag_int_array.data = (int32_t*)malloc(length * 4);
    memcpy(node->payload.tag_int_array.data, data, length * 4);
    return node;
}
nbt_node *tag_list(const char *name, NBTList entries) {
    nbt_node *node = new_node(name, TAG_LIST);
    node->payload.tag_list = (decltype(node->payload.tag_list))list_payload(entries);
    return node;
}
nbt_node *tag_compound(const char *name, NBTList entries) {
    nbt_node *node = new_node(name, TAG_COMPOUND);
    node->payload.tag_compound = (decltype(node->payload.tag_compound))compound_payload(entries);
    return node;
}


