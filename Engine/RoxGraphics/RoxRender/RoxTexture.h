// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
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

#pragma once

namespace RoxMemory
{
	class RoxTmpBufferRef;
}

struct ID3D11Texture2D;

namespace RoxRender
{
	class RoxTexture
	{
		friend class RoxFBO;

	public:
		enum COLOR_FORMAT
		{
			COLOR_RGB,
			COLOR_RGBA,
			COLOR_BGRA,
			//color_r,
			GREYSCALE,

			COLOR_R32F,
			COLOR_RGB32F,
			COLOR_RGBA32F,

			DEPTH16,
			DEPTH24,
			DEPTH32,

			DXT1,
			DXT3,
			DXT5,

			DXT2 = DXT3,
			DXT4 = DXT5,

			ETC1,
			ETC2,
			ETC2_EAC,
			ETC2_A1,

			PVR_RGB2B,
			PVR_RGB4B,
			PVR_RGBA2B,
			PVR_RGBA4B
		};

		static bool isDxtSupported();

		typedef unsigned int uint;

	public:
		//mip_count= -1 means "generate mipmaps". You have to provide a single mip or a complete mipmap pyramid instead
		//resulting format may vary on different platforms, allways use get_color_format() to get actual texture format
		bool buildTexture(const void* data, uint width, uint height, COLOR_FORMAT format,
		                   int mip_count = -1);

		//order: positive_x,negative_x,positive_y,negative_y,positive_z,negative_z
		bool buildCubemap(const void* data[6], uint width, uint height, COLOR_FORMAT format,
		                   int mip_count = -1);

	public:
		//mip= -1 means "update all mipmaps"
		bool updateRegion(const void* data, uint x, uint y, uint width, uint height, int mip = -1);

		bool copyRegion(const RoxTexture& src, uint src_x, uint src_y, uint src_width, uint src_height, uint dst_x,
		                 uint dst_y);

	public:
		void bind(uint layer) const;
		static void unbind(uint layer);

	public:
		enum WRAP
		{
			WRAP_CLAMP,
			WRAP_REPEAT,
			WRAP_REPEAT_MIRROR
		};

		void setWrap(WRAP s, WRAP t);

		enum FILTER
		{
			FILTER_NEAREST,
			FILTER_LINEAR
		};
		
		void setFilter(FILTER minification, FILTER magnification, FILTER mipmap);
		void setAniso(uint level);

		static void setDefaultWrap(WRAP s, WRAP t);
		static void setDefaultFilter(FILTER minification, FILTER magnification, FILTER mipmap);
		static void setDefaultAniso(uint level);

	public:
		bool getData(RoxMemory::RoxTmpBufferRef& data) const;
		bool getData(RoxMemory::RoxTmpBufferRef& data, uint x, uint y, uint w, uint h) const;
		uint getWidth() const;
		uint getHeight() const;
		COLOR_FORMAT getColorFormat() const;
		bool isCubemap() const;

		static void getDefaultWrap(WRAP& s, WRAP& t);
		static void getDefaultFilter(FILTER& minification, FILTER& magnification, FILTER& mipmap);
		static uint getDefaultAniso();

		static uint getMaxDimension();

		static uint getFormatBpp(COLOR_FORMAT f);

	public:
		void release();

	public:
		static uint getUsedVmemSize();

	public:
		uint getGlTexID() const;
		ID3D11Texture2D* getDx11TexID() const;

	public:
		RoxTexture(): m_tex(-1), m_width(0), m_height(0), m_is_cubemap(false), m_filter_set(false), m_aniso_set(false),
		           m_aniso(0), m_filter_min(FILTER_LINEAR), m_filter_mag(FILTER_LINEAR), m_filter_mip(FILTER_LINEAR)
		{
		}

	private:
		bool buildTexture(const void* data[6], bool is_cubemap, uint width, uint height,
		                   COLOR_FORMAT format, int mip_count = -1);

	private:
		int m_tex;
		uint m_width, m_height;
		COLOR_FORMAT m_format;
		bool m_is_cubemap;
		bool m_filter_set;
		bool m_aniso_set;
		uint m_aniso;
		FILTER m_filter_min, m_filter_mag, m_filter_mip;
	};
}
