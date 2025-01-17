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

#include "Component.h"
#include "GameObject.h"

namespace Viry3D
{
    Component::Component()
    {

    }
    
    Component::~Component()
    {
        
    }
    
    const Ref<Transform>& Component::GetTransform() const
    {
        return this->GetGameObject()->GetTransform();
    }

    void Component::Enable(bool enable)
    {
        if (m_enable != enable)
        {
            m_enable = enable;

            this->OnEnable(m_enable);
        }
    }
}
