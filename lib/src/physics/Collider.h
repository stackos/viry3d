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

#pragma once

#include "Component.h"

namespace Viry3D
{
    struct Vector3;

    class Collider : public Component
    {
    public:
        virtual ~Collider();
        bool IsRigidbody() const { return m_is_rigidbody; }
        virtual void SetIsRigidbody(bool value);
        void SetRollingFriction(float f); // ����Ħ��
        void SetFriction(float f); // Ħ��
        void SetRestitution(float r); // ����
        void ApplyCentralImpulse(const Vector3& impulse);

    protected:
        Collider() :
            m_collider(nullptr),
            m_in_world(false),
            m_is_rigidbody(false)
        {
        }
        virtual void OnEnable(bool enable);
        virtual void OnGameObjectLayerChanged();

    protected:
        void* m_collider;
        bool m_in_world;
        bool m_is_rigidbody;
        float m_mass = 1;
        float m_rolling_friction = 0.1f;
        float m_friction = 0.1f;
        float m_restitution = 0.1f;
    };
}
