// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Update the render api interface to check Metal 1th.
//
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#include "RoxShader.h"

#include <iostream>

#include "RoxShaderCodeParser.h"
#include "RoxRender.h"
#include "IRoxRenderApi.h"

namespace RoxRender
{
	bool RoxShader::addProgram(PROGRAM_TYPE type, const char* code)
	{
		if (type >= PROGRAM_TYPES_COUNT)
		{
			log() << "Unable to add shader program: invalid shader type\n";
			return false;
		}

		if (!code || !code[0])
		{
			log() << "Unable to add shader program: invalid code\n";
			return false;
		}

		m_code[type] = code;
     
        if (!m_code[VERTEX].empty() && !m_code[PIXEL].empty())
		{
			release();
			m_shdr = getApiInterface().createShader(m_code[VERTEX].c_str(), m_code[PIXEL].c_str());
			if (m_shdr < 0)
				return false;

			m_buf = getApiInterface().createUniformBuffer(m_shdr);

			m_uniforms.resize(getApiInterface().getUniformsCount(m_shdr));
			for (int i = 0; i < (int)m_uniforms.size(); ++i)
				m_uniforms[i] = getApiInterface().getUniform(m_shdr, i);
			return true;
		}

		return true;
	}

void RoxShader::bind() const { getApiState().shader= m_shdr; getApiState().uniform_buffer=m_buf; }
void RoxShader::unbind() { getApiState().shader= -1; getApiState().uniform_buffer= -1; }

int RoxShader::getSamplerLayer(const char *name) const
{
    if(!name || !name[0])
        return -1;

    for(int i=0,layer=0;i<(int)m_uniforms.size();++i)
    {
        const Uniform &u=m_uniforms[i];
        if(u.type!=UNIFORM_SAMPLER2D && u.type!=UNIFORM_SAMPLER_CUBE)
            continue;

        if(u.name==name)
            return layer;

        ++layer;
    }

    return -1;
}

int RoxShader::findUniform(const char *name) const
{
    if(!name || !name[0])
        return -1;

    for(int i=0;i<(int)m_uniforms.size();++i)
    {
        if(m_uniforms[i].name==name)
            return i;
    }

    return -1;
}

void RoxShader::setUniform(int i,float f0,float f1,float f2,float f3) const
{
    if(i<0 || i>=(int)m_uniforms.size())
        return;

    const float f[]={f0,f1,f2,f3};
    getApiInterface().setUniform(m_shdr,i,f,4);
}

void RoxShader::setUniform3Array(int i,const float *f,unsigned int count) const
{
    if(!f || i<0 || i>=(int)m_uniforms.size())
        return;

    const Uniform &u=m_uniforms[i];
    if(count>=u.array_size)
        count=u.array_size;

    if(!count)
        return;

    getApiInterface().setUniform(m_shdr,i,f,count*3);
}

void RoxShader::setUniform4Array(int i,const float *f,unsigned int count) const
{
    if(!f || i<0 || i>=(int)m_uniforms.size())
        return;

    if(!count)
        return;

    const Uniform &u=m_uniforms[i];
    if(u.type<UNIFORM_MAT4)
    {
        if(count>=u.array_size)
            count=u.array_size;

        getApiInterface().setUniform(m_shdr,i,f,count*4);
    }
    else if(u.type==UNIFORM_MAT4)
    {
        count/=4;
        if(count>=u.array_size)
            count=u.array_size;

        getApiInterface().setUniform(m_shdr,i,f,count*16);
    }
}

void RoxShader::setUniform16Array(int i,const float *f,unsigned int count) const
{
    if(!f || i<0 || i>=(int)m_uniforms.size())
        return;

    const Uniform &u=m_uniforms[i];
    if(u.type!=UNIFORM_MAT4)
        return;

    if(count>=u.array_size)
        count=u.array_size;

    if(!count)
        return;

    getApiInterface().setUniform(m_shdr,i,f,count*16);
}

int RoxShader::getUniformsCount() const { return (int)m_uniforms.size(); }

const char *RoxShader::getUniformName(int idx) const
{
    return idx>=0 && idx<(int)m_uniforms.size()?m_uniforms[idx].name.c_str():0;
}

RoxShader::UNIFORM_TYPE RoxShader::getUniformType(int idx) const
{
    return idx>=0 && idx<(int)m_uniforms.size()?m_uniforms[idx].type:UNIFORM_NOT_FOUND;
}

unsigned int RoxShader::getUniformArraySize(int idx) const
{
    return idx>=0 && idx<(int)m_uniforms.size()?m_uniforms[idx].array_size:0;
}

void RoxShader::release()
{
    if(m_shdr>=0)
    {
        if(getApiState().shader ==m_shdr)
            getApiState().shader =-1;
        getApiInterface().removeShader(m_shdr);
    }

    if(m_buf>=0)
    {
        if(getApiState().uniform_buffer==m_buf)
            getApiState().uniform_buffer=-1;
        getApiInterface().removeUniformBuffer(m_buf);
    }

    m_shdr=m_buf=-1;
    m_uniforms.clear();
}

}
