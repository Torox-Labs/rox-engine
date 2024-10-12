//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "apk_resources_provider.h"

#ifdef __ANDROID__
    #include <jni.h>
    #include <android/asset_manager.h>
#endif

namespace nya_resources
{

namespace { AAssetManager *asset_mgr=0; }

void apk_resources_provider::set_asset_manager(AAssetManager *mgr) { asset_mgr=mgr; }

bool apk_resources_provider::set_folder(const char *folder)
{
    nya_memory::lock_guard_write lock(m_mutex);

    m_update_names=true;

    if(!asset_mgr || !folder)
    {
        m_folder.clear();
        return false;
    }

    m_folder.assign(folder);

#ifdef __ANDROID__
    AAssetDir *asset_dir=AAssetManager_openDir(asset_mgr,folder);
    if(!asset_dir)
        return false;

    AAssetDir_close(asset_dir);
#endif

    return true;
}

bool apk_resources_provider::has(const char *name)
{
    if(!name)
        return false;

    nya_memory::lock_guard_read lock(m_mutex);

#ifdef __ANDROID__
    if(!asset_mgr)
        return false;

    AAsset *asset=AAssetManager_open(asset_mgr,name,AASSET_MODE_UNKNOWN);
    if(!asset)
        return false;

    AAsset_close(asset);
#endif

    return true;
}

void apk_resources_provider::lock()
{
    resources_provider::lock();

    if(m_update_names)
    {
        resources_provider::unlock();
        m_mutex.lock_write();
        if(m_update_names)
            update_names();
        m_mutex.unlock_write();
        lock();
    }
}

void apk_resources_provider::update_names()
{
    m_names.clear();
    if(!asset_mgr)
        return;

    m_update_names=false;

#ifdef __ANDROID__
    AAssetDir *asset_dir=AAssetManager_openDir(asset_mgr,m_folder.c_str());
    if(!asset_dir)
        return;

    const char *filename = 0;
    while((filename=AAssetDir_getNextFileName(asset_dir)))
        m_names.push_back(filename);

    AAssetDir_close(asset_dir);
#endif
}

int apk_resources_provider::get_resources_count()
{
    if(m_update_names)
        update_names();

    return (int)m_names.size();
}

const char *apk_resources_provider::get_resource_name(int idx)
{
    if(idx<0 || idx>=get_resources_count())
        return 0;

    return m_names[idx].c_str();
}

#ifdef __ANDROID__
namespace
{

class apk_resource: public resource_data
{
public:
    apk_resource(AAsset *asset): m_asset(asset) { m_size=AAsset_getLength(m_asset); }

    void release() { AAsset_close(m_asset); delete this; }

    size_t get_size() { return m_size; }

    bool read_all(void *data)
    {
        if(!data)
            return false;

        AAsset_seek(m_asset,0,SEEK_SET);
        return AAsset_read(m_asset,data,m_size)==m_size;
    }

    bool read_chunk(void *data,size_t size,size_t offset)
    {
        if(!data)
            return false;

        const off_t pos=AAsset_seek(m_asset,offset,SEEK_SET);
        if(pos== -1 || pos!=offset)
            return false;

        return AAsset_read(m_asset,data,size)==size;
    }

private:
    AAsset *m_asset;
    size_t m_size;
};

}
#endif

resource_data *apk_resources_provider::access(const char *name)
{
    if(!asset_mgr || !name)
        return 0;

    nya_memory::lock_guard_read lock(m_mutex);

#ifdef __ANDROID__
    AAsset *asset=AAssetManager_open(asset_mgr,name,AASSET_MODE_UNKNOWN);
    if(!asset)
        return 0;

    return new apk_resource(asset);
#else
    return 0;
#endif
}

}
