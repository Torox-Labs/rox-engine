// Updated By the ROX_ENGINE
// Copyright © 2024 Torox Project
// Portions Copyright © 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#include "RoxFileResourcesProvider.h"
#include "RoxMemory/RoxPool.h"
#include "RoxMemory/RoxLru.h"

#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <dirent.h>
#endif

#include <sys/stat.h>

#ifndef S_ISDIR
#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#endif

namespace RoxResources
{
	class RoxFileReference
	{
	public:
		void init(const char* name) { m_name.assign(name ? name : ""); }

		FILE* access()
		{
			RoxMemory::RoxLockGuard lock(m_mutex);
			return getLru().access(m_name.c_str());
		}

		void free()
		{
			RoxMemory::RoxLockGuard lock(m_mutex);
			getLru().free(m_name.c_str());
		}

		class RoxLru : public RoxMemory::RoxLru<FILE*, 64>
		{
			bool onAccess(const char* name, FILE*& f) override
			{
#ifdef _WIN32
				if (!name)
					return false;

				const int len = MultiByteToWideChar(CP_UTF8, 0, name, -1, 0, 0);
				if (!len)
					return false;

				WCHAR* wname = new WCHAR[len];
				MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, len);
				f = _wfopen(wname, L"rb");
				delete wname;
				return f != 0;
#else
                return name ? (f = fopen(name, "rb")) != 0 : false;
#endif
			}

			bool onFree(const char* name, FILE*& f) override { return fclose(f) == 0; }
		};

		static RoxLru& getLru()
		{
			static RoxLru* cache = new RoxLru();
			return *cache;
		}

	private:
		std::string m_name;
		RoxMemory::RoxMutex m_mutex;
	};

	class RoxFileResource : public IRoxResourceData
	{
	public:
		size_t getSize() override { return m_size; }

		bool readAll(void* data) override;
		bool readChunk(void* data, size_t size, size_t offset) override;

	public:
		bool open(const char* file_name);
		void release() override;

		RoxFileResource() : m_size(0)
		{
		}

		//~RoxFileResource() { release(); }

	private:
		RoxFileReference m_file;
		size_t m_size;
	};
}

namespace RoxResources
{
	IRoxResourceData* RoxFileResourcesProvider::access(const char* resource_name)
	{
		if (!resource_name)
		{
			RoxLogger::log() << "unable to access file: invalid name\n";
			return 0;
		}

		RoxFileResource* file = new RoxFileResource;

		RoxMemory::RoxLockGuardRead lock(m_mutex);

		std::string file_name = m_path + resource_name;
		for (size_t i = m_path.size(); i < file_name.size(); ++i)
		{
			if (file_name[i] == '\\')
				file_name[i] = '/';
		}

		if (!file->open(file_name.c_str()))
		{
			RoxLogger::log() << "unable to access file: " << file_name.c_str() + m_path.size()
				<< " at path " << m_path.c_str() << "\n";
			file->release();
			return 0;
		}

		return file;
	}

	bool RoxFileResourcesProvider::has(const char* name)
	{
		if (!name)
			return false;

		RoxMemory::RoxLockGuardRead lock(m_mutex);

		std::string file_name = m_path + name;
		for (size_t i = m_path.size(); i < file_name.size(); ++i)
		{
			if (file_name[i] == '\\')
				file_name[i] = '/';
		}


#ifdef _WIN32
		const int len = MultiByteToWideChar(CP_UTF8, 0, file_name.c_str(), -1, 0, 0);
		if (!len)
			return false;

		WCHAR* wname = new WCHAR[len];
		MultiByteToWideChar(CP_UTF8, 0, file_name.c_str(), -1, wname, len);
		struct _stat sb;
		bool result = _wstat(wname, &sb) == 0;
		delete wname;
		return result;
#else
        struct stat sb;
        return stat(file_name.c_str(), &sb) == 0;
#endif
	}

	bool RoxFileResourcesProvider::setFolder(const char* folder_name, bool recursive, bool ignore_non_existent)
	{
		RoxMemory::RoxLockGuardWrite lock(m_mutex);

		m_recursive = recursive;
		m_update_names = true;

		if (!folder_name)
		{
			m_path.erase();
			return false;
		}

		m_path.assign(folder_name);
		for (size_t i = 0; i < m_path.size(); ++i)
		{
			if (m_path[i] == '\\')
				m_path[i] = '/';
		}

		if (m_path.length() > 2 && m_path[0] == '.' && m_path[1] == '/')
			m_path = m_path.substr(2);

		while (m_path.length() > 1 && m_path[m_path.length() - 1] == '/')
			m_path.resize(m_path.length() - 1);

		if (m_path.empty())
			return true;

		if (!ignore_non_existent)
		{
			struct stat sb;
			if (stat(m_path.c_str(), &sb) == -1)
			{
				RoxLogger::log() << "warning: unable to stat at path " << folder_name << ", probably does not exist\n";
				m_path.push_back('/');
				return false;
			}
			else if (!S_ISDIR(sb.st_mode))
			{
				RoxLogger::log() << "warning: specified path is not a directory " << folder_name << "\n";
				m_path.push_back('/');
				return false;
			}
		}

		m_path.push_back('/');
		return true;
	}

	void RoxFileResourcesProvider::enumerateFolder(const char* folder_name)
	{
		if (!folder_name)
			return;

		std::string folder_name_str(folder_name);

		while (!folder_name_str.empty() && folder_name_str[folder_name_str.length() - 1] == '/')
			folder_name_str.resize(folder_name_str.length() - 1);

		std::string first_dir = (m_path + folder_name_str);
		if (first_dir.empty())
			return;

		while (!first_dir.empty() && first_dir[first_dir.length() - 1] == '/')
			first_dir.resize(first_dir.length() - 1);

#ifdef _WIN32
		_finddata_t data;
		intptr_t hdl = _findfirst((first_dir + "/*").c_str(), &data);
		if (!hdl)
#else
        DIR* dirp = opendir(first_dir.c_str());
        if (!dirp)
#endif
		{
			RoxLogger::log() << "unable to enumerate folder " << (m_path + folder_name_str).c_str() << "\n";
			return;
		}

#ifdef _WIN32
		for (intptr_t it = hdl; it >= 0; it = _findnext(hdl, &data))
		{
			const char* name = data.name;
#else
        while (dirent* dp = readdir(dirp))
        {
            const char* name = dp->d_name;
#endif
			if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
				continue;

			std::string entry_name;
			if (!folder_name_str.empty() && folder_name_str != ".")
				entry_name = folder_name_str + '/';
			entry_name.append(name);

#ifdef _WIN32
			const std::string full_path_str = m_path + entry_name;
			struct stat stat_buf;
			if (stat(full_path_str.c_str(), &stat_buf) < 0)
			{
				RoxLogger::log() << "unable to read " << full_path_str.c_str() << "\n";
				continue;
			}

			if ((stat_buf.st_mode & S_IFDIR) == S_IFDIR && m_recursive)
#else
            if (dp->d_type == DT_DIR && m_recursive)
#endif
			{
				enumerateFolder(entry_name.c_str());
				continue;
			}

			m_resource_names.push_back(entry_name);
		}
#ifdef _WIN32
		_findclose(hdl);
#else
        closedir(dirp);
#endif
	}

	void RoxFileResourcesProvider::updateNames()
	{
		m_resource_names.clear();
		enumerateFolder(m_path.empty() ? "." : "");
		m_update_names = false;
	}

	int RoxFileResourcesProvider::getResourcesCount()
	{
		if (m_update_names)
			updateNames();

		return (int)m_resource_names.size();
	}

	const char* RoxFileResourcesProvider::getResourceName(int idx)
	{
		if (idx < 0 || idx >= getResourcesCount())
			return 0;

		return m_resource_names[idx].c_str();
	}

	void RoxFileResourcesProvider::lock()
	{
		IRoxResourcesProvider::lock();

		if (m_update_names)
		{
			IRoxResourcesProvider::unlock();
			m_mutex.lockWrite();
			if (m_update_names)
				updateNames();
			m_mutex.unlockWrite();
			lock();
		}
	}

	bool RoxFileResource::readAll(void* data)
	{
		if (!data)
		{
			if (m_size > 0)
				RoxLogger::log() << "unable to read file data: invalid data pointer\n";
			return false;
		}

		FILE* file = m_file.access();
		if (!file)
		{
			RoxLogger::log() << "unable to read file data: no such file\n";
			return false;
		}

		if (fseek(file, 0, SEEK_SET) != 0)
		{
			RoxLogger::log() << "unable to read file data: seek_set failed\n";
			return false;
		}

		if (fread(data, 1, m_size, file) != m_size)
		{
			RoxLogger::log() << "unable to read file data: unexpected size of readen data\n";
			return false;
		}

		return true;
	}

	bool RoxFileResource::readChunk(void* data, size_t size, size_t offset)
	{
		if (!data)
		{
			if (size > 0)
				RoxLogger::log() << "unable to read file data chunk: invalid data pointer\n";
			return false;
		}

		FILE* file = m_file.access();
		if (!file)
		{
			RoxLogger::log() << "unable to read file data: no such file\n";
			return false;
		}

		if (offset + size > m_size || !size)
		{
			RoxLogger::log() << "unable to read file data chunk: invalid size\n";
			return false;
		}

		if (fseek(file, (long)offset, SEEK_SET) != 0)
		{
			RoxLogger::log() << "unable to read file data chunk: seek_set failed\n";
			return false;
		}

		if (fread(data, 1, size, file) != size)
		{
			RoxLogger::log() << "unable to read file data chunk: unexpected size of reader data\n";
			return false;
		}

		return true;
	}

	bool RoxFileResource::open(const char* file_name)
	{
		m_file.free();

		m_size = 0;

		if (!file_name)
			return false;

		m_file.init(file_name);
		FILE* file = m_file.access();
		if (!file)
			return false;

		if (fseek(file, 0, SEEK_END) != 0)
			return false;

		m_size = ftell(file);

		return true;
	}

	void RoxFileResource::release()
	{
		m_file.free();
		delete this;
	}

}
