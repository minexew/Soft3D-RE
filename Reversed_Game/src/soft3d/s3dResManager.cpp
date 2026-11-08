#include "soft3d/s3dResManager.hpp"

#include "soft3d/Animation.hpp"
#include "soft3d/AudioData.hpp"
#include "soft3d/ObjectRes.hpp"
#include "soft3d/Resource.hpp"
#include "soft3d/TextureRes.hpp"

#include <algorithm>
#include <cstring>
#include <new>
#include <vector>

namespace soft3d {
namespace {

void CopyString(char16_t* destination, const char16_t* source)
{
    do {
        *destination++ = *source;
    } while (*source++ != u'\0');
}

void AppendString(char16_t* destination, const char16_t* source)
{
    while (*destination != u'\0') {
        ++destination;
    }
    CopyString(destination, source);
}

size_t StringLength(const char16_t* text)
{
    const char16_t* end = text;
    while (*end != u'\0') {
        ++end;
    }
    return static_cast<size_t>(end - text);
}

bool EqualStrings(const char16_t* left, const char16_t* right)
{
    while (*left == *right) {
        if (*left == u'\0') {
            return true;
        }
        ++left;
        ++right;
    }
    return false;
}

const char16_t* ExtensionForType(uint32_t type)
{
    switch (type) {
    case ResourceAudio:
        return u".sau";
    case ResourceObject:
        return u".soj";
    case ResourceAnimation:
        return u".sai";
    case ResourceTexture:
        return u".stx";
    default:
        return nullptr;
    }
}

Resource* CreateResource(uint32_t type)
{
    switch (type) {
    case ResourceAudio:
        return new (std::nothrow) AudioData();
    case ResourceObject:
        return new (std::nothrow) ObjectRes();
    case ResourceAnimation:
        return new (std::nothrow) Animation();
    case ResourceTexture:
        return new (std::nothrow) TextureRes();
    default:
        return nullptr;
    }
}

} // namespace

struct s3dResManager::Cache {
    struct Node {
        uint32_t hash;
        char16_t key[64];
        Resource* resource;
        Node* next;
    };

    struct Bucket {
        Node* head;
    };

    explicit Cache(uint32_t count)
        : bucketCount(count)
        , buckets(new Bucket[count])
    {
        for (uint32_t i = 0; i < bucketCount; ++i) {
            buckets[i].head = nullptr;
        }
    }

    ~Cache()
    {
        ClearNodes();
        delete[] buckets;
    }

    uint32_t Hash(const char16_t* key) const
    {
        uint32_t hash = 0;
        while (*key != u'\0') {
            hash = hash * 0x21 + static_cast<uint16_t>(*key++);
        }
        return hash % bucketCount;
    }

    Resource* Find(const char16_t* key) const
    {
        Node* node = buckets[Hash(key)].head;
        while (node != nullptr) {
            if (EqualStrings(node->key, key)) {
                return node->resource;
            }
            node = node->next;
        }
        return nullptr;
    }

    bool Insert(const char16_t* key, Resource* resource)
    {
        Node* node = new (std::nothrow) Node{};
        if (node == nullptr) {
            return false;
        }
        uint32_t hash = Hash(key);
        Node* previous = nullptr;
        Node* current = buckets[hash].head;
        while (current != nullptr) {
            if (EqualStrings(current->key, key)) {
                delete node;
                return false;
            }
            previous = current;
            current = current->next;
        }
        node->hash = hash;
        node->resource = resource;
        CopyString(node->key, key);
        if (previous == nullptr) {
            buckets[hash].head = node;
        } else {
            previous->next = node;
        }
        return true;
    }

    bool Remove(const char16_t* key)
    {
        uint32_t hash = Hash(key);
        Node* previous = nullptr;
        Node* node = buckets[hash].head;
        while (node != nullptr) {
            if (EqualStrings(node->key, key)) {
                Node* next = node->next;
                delete node;
                if (previous == nullptr) {
                    buckets[hash].head = next;
                } else {
                    previous->next = next;
                }
                return true;
            }
            previous = node;
            node = node->next;
        }
        return false;
    }

    void AppendResources(std::vector<Resource*>& resources) const
    {
        for (uint32_t i = 0; i < bucketCount; ++i) {
            for (Node* node = buckets[i].head; node != nullptr; node = node->next) {
                resources.push_back(node->resource);
            }
        }
    }

    void ClearNodes()
    {
        for (uint32_t i = 0; i < bucketCount; ++i) {
            Node* node = buckets[i].head;
            while (node != nullptr) {
                Node* next = node->next;
                delete node;
                node = next;
            }
            buckets[i].head = nullptr;
        }
    }

    uint32_t bucketCount;
    Bucket* buckets;
};

s3dResManager::s3dResManager()
    : caches(new Cache[4]{Cache(256), Cache(256), Cache(256), Cache(256)})
{
}

s3dResManager::~s3dResManager()
{
    for (int i = 0; i < 4; ++i) {
        ClearCache(i);
    }
    filePak.Close();
    delete[] caches;
}

Resource* s3dResManager::GetResource(uint32_t type, int cacheLevel, const char16_t* name,
                                     const char16_t* resourceDirectory)
{
    if (name == nullptr || StringLength(name) == 0) {
        return nullptr;
    }
    const char16_t* extension = ExtensionForType(type);
    if (extension == nullptr) {
        return nullptr;
    }

    char16_t fileName[32];
    CopyString(fileName, name);
    AppendString(fileName, extension);

    char16_t directory[64];
    if (resourceDirectory == nullptr) {
        CopyString(directory, u".\\common\\");
    } else {
        if (resourceDirectory[0] == u'.') {
            CopyString(directory, resourceDirectory);
        } else if (resourceDirectory[0] == u'\\') {
            CopyString(directory, u".");
            AppendString(directory, resourceDirectory);
        } else {
            CopyString(directory, u".\\");
            AppendString(directory, resourceDirectory);
        }
        if (resourceDirectory[StringLength(resourceDirectory) - 1] != u'\\') {
            AppendString(directory, u"\\");
        }
    }

    char16_t path[128];
    CopyString(path, directory);
    AppendString(path, fileName);

    Resource* resource = nullptr;
    for (int i = 0; i < 4; ++i) {
        resource = caches[i].Find(path);
        if (resource != nullptr) {
            if (cacheLevel < i) {
                caches[i].Remove(path);
                caches[cacheLevel].Insert(path, resource);
            }
            return resource;
        }
    }

    resource = CreateResource(type);
    if (resource == nullptr) {
        return nullptr;
    }
    resource->SetEngine(engine);
    if (resource->Initialize(fileName, directory, &filePak) == S3D_OK) {
        caches[cacheLevel].Insert(path, resource);
        return resource;
    }
    delete resource;
    return nullptr;
}

void s3dResManager::ReleaseResource(Resource* resource)
{
    if (resource == nullptr) {
        return;
    }
    const char16_t* path = resource->GetPath();
    for (int i = 0; i < 4; ++i) {
        if (caches[i].Remove(path)) {
            break;
        }
    }
    delete resource;
}

void s3dResManager::ClearCache(int cacheLevel)
{
    std::vector<Resource*> resources;
    caches[cacheLevel].AppendResources(resources);
    for (Resource* resource : resources) {
        delete resource;
    }
    caches[cacheLevel].ClearNodes();
}

s3dFilePak* s3dResManager::GetFilePak()
{
    return &filePak;
}

s3dFilePak* GetFilePakFromManager(s3dResManager* manager)
{
    return manager->GetFilePak();
}

void OpenGameResourceArchive(s3dResManager* manager, const char16_t* fileName)
{
    manager->GetFilePak()->Open(fileName, 0);
}

} // namespace soft3d
