/*
* Viry3D
* Copyright 2014-2019 by Stack - stackos@qq.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "Light.h"
#include "Engine.h"
#include "Material.h"
#include "GameObject.h"

namespace Viry3D
{
	List<Light*> Light::m_lights;
	Color Light::m_ambient_color(0, 0, 0, 0);

	void Light::SetAmbientColor(const Color& color)
	{
		m_ambient_color = color;
		for (auto i : m_lights)
		{
			i->m_dirty = true;
		}
	}

    Light::Light():
		m_dirty(true),
        m_type(LightType::Directional),
		m_color(1, 1, 1, 1),
		m_intensity(1.0f),
		m_culling_mask(0xffffffff)
    {
		m_lights.AddLast(this);
    }

	Light::~Light()
    {
		auto& driver = Engine::Instance()->GetDriverApi();

		if (m_light_uniform_buffer)
		{
			driver.destroyUniformBuffer(m_light_uniform_buffer);
			m_light_uniform_buffer.clear();
		}

		m_lights.Remove(this);
    }

	void Light::OnTransformDirty()
	{
		m_dirty = true;
	}

	void Light::SetType(LightType type)
	{
		m_type = type;
		m_dirty = true;
	}

	void Light::SetColor(const Color& color)
	{
		m_color = color;
		m_dirty = true;
	}

	void Light::SetIntensity(float intensity)
	{
		m_intensity = intensity;
		m_dirty = true;
	}

	void Light::SetCullingMask(uint32_t mask)
	{
		m_culling_mask = mask;
	}

	void Light::Prepare()
	{
		if (!m_dirty)
		{
			return;
		}
		m_dirty = false;

		auto& driver = Engine::Instance()->GetDriverApi();
		if (!m_light_uniform_buffer)
		{
			m_light_uniform_buffer = driver.createUniformBuffer(sizeof(LightUniforms), filament::backend::BufferUsage::DYNAMIC);
		}

		LightUniforms light_uniforms;
		light_uniforms.ambient_color = this->GetAmbientColor();
		if (this->GetType() == LightType::Directional)
		{
			light_uniforms.light_pos = -(this->GetTransform()->GetForward());
		}
		else
		{
			light_uniforms.light_pos = this->GetTransform()->GetPosition();
		}
		light_uniforms.light_color = this->GetColor();
		light_uniforms.light_intensity = this->GetIntensity();

		void* buffer = driver.allocate(sizeof(LightUniforms));
		Memory::Copy(buffer, &light_uniforms, sizeof(LightUniforms));
		driver.loadUniformBuffer(m_light_uniform_buffer, filament::backend::BufferDescriptor(buffer, sizeof(LightUniforms)));
	}
}