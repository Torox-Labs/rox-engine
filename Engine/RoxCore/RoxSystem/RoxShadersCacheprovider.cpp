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

#include "RoxShadersCacheprovider.h"
#include "RoxResources/RoxResources.h"

#include <cstring>
#include <cstdio>

//ToDo: hash collisions

namespace RoxSystem
{
	bool RoxShaderCacheProvider::get(const char* text, RoxRender::RoxCompiledShader& shader)
	{
		shader = RoxRender::RoxCompiledShader();

		if (!text)
			return false;

		RoxResources::IRoxResourceData* data =
			RoxResources::getResourcesProvider().access((m_load_path + crc(text) + ".rsc").c_str());

		if (!data)
			return false;

		shader = RoxRender::RoxCompiledShader(data->getSize());
		data->readAll(shader.getData());
		data->release();

		return true;
	}

	bool RoxShaderCacheProvider::set(const char* text, const RoxRender::RoxCompiledShader& shader)
	{
		if (!text)
			return false;

		const void* data = shader.getData();
		if (!data)
			return false;

		FILE* f = fopen((m_save_path + crc(text) + ".rsc").c_str(), "wb");
		if (!f)
			return false;

		fwrite(data, shader.getSize(), 1, f);
		fclose(f);

		return true;
	}

	std::string RoxShaderCacheProvider::crc(const char* text)
	{
		if (!text)
			return "";

		static unsigned int crc_table[256];
		static bool initialised = false;
		if (!initialised)
		{
			for (int i = 0; i < 256; i++)
			{
				unsigned int crc = i;
				for (int j = 0; j < 8; j++)
					crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;

				crc_table[i] = crc;
			};

			initialised = true;
		}

		unsigned int crc = 0xFFFFFFFFUL;
		size_t len = strlen(text);
		unsigned char* buf = (unsigned char*)text;
		while (len--)
			crc = crc_table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);

		char tmp[256];
		//sprintf(tmp, "%u", crc);
		snprintf(tmp, sizeof(tmp), "%u", crc);

		return std::string(tmp);
	}

}
