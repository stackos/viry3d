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

#include "Object.h"
#include "Color.h"
#include "container/Vector.h"
#include "math/Vector2.h"
#include "math/Matrix4x4.h"
#include "math/Bounds.h"
#include "private/backend/DriverApi.h"

namespace Viry3D
{
    class Texture;

    class Mesh : public Object
    {
    public:
        struct Vertex
        {
            Vector4 vertex;
            Color color;
            Vector2 uv;
            Vector2 uv2;
            Vector3 normal;
            Vector4 tangent;
            Vector4 bone_weights;
            Vector4 bone_indices;
        };
        
        struct Submesh
        {
            int index_first;
            int index_count;
        };
        
        struct BlendShapeFrame
        {
            Vector<Vector3> vertices;
            Vector<Vector3> normals;
            Vector<Vector3> tangents;
        };
        
        struct BlendShape
        {
            String name;
            BlendShapeFrame frame;
        };

    public:
		static void Init();
		static void Done();
		static const Ref<Mesh>& GetSharedQuadMesh();
        static const Ref<Mesh>& GetSharedBoundsMesh();
        static Ref<Mesh> LoadFromFile(const String& path);
        Mesh(Vector<Vertex>&& vertices, Vector<unsigned int>&& indices, const Vector<Submesh>& submeshes = Vector<Submesh>(), bool uint32_index = false, bool dynamic = false, filament::backend::PrimitiveType primitive_type = filament::backend::PrimitiveType::TRIANGLES);
        virtual ~Mesh();
        void Update(Vector<Vertex>&& vertices, Vector<unsigned int>&& indices, const Vector<Submesh>& submeshes = Vector<Submesh>());
        const Vector<Vertex>& GetVertices() const { return m_vertices; }
        const Vector<unsigned int>& GetIndices() const { return m_indices; }
        const Vector<Submesh>& GetSubmeshes() const { return m_submeshes; }
        const Vector<Matrix4x4>& GetBindposes() const { return m_bindposes; }
        const Vector<BlendShape>& GetBlendShapes() const { return m_blend_shapes; }
        const Ref<Texture>& GetBlendShapeTexture() const { return m_blend_shape_texture; }
        const Bounds& GetBounds() const { return m_bounds; }
		const filament::backend::AttributeArray& GetAttributes() const { return m_attributes; }
		uint32_t GetEnabledAttributes() const { return m_enabled_attributes; }
		const filament::backend::VertexBufferHandle& GetVertexBuffer() const { return m_vb; }
		const filament::backend::IndexBufferHandle& GetIndexBuffer() const { return m_ib; }
		const Vector<filament::backend::RenderPrimitiveHandle>& GetPrimitives() const { return m_primitives; }

    private:
        void SetBindposes(Vector<Matrix4x4>&& bindposes) { m_bindposes = std::move(bindposes); }
        void SetBlendShapes(Vector<BlendShape>&& blend_shapes);
        
    private:
		static Ref<Mesh> m_shared_quad_mesh;
        static Ref<Mesh> m_shared_bounds_mesh;
        Vector<Vertex> m_vertices;
        Vector<unsigned int> m_indices;
        int m_buffer_vertex_count;
        int m_buffer_index_count;
        Vector<Submesh> m_submeshes;
        Vector<Matrix4x4> m_bindposes;
        Vector<BlendShape> m_blend_shapes;
        Ref<Texture> m_blend_shape_texture;
        Bounds m_bounds;
        bool m_uint32_index;
		filament::backend::AttributeArray m_attributes;
		uint32_t m_enabled_attributes;
        filament::backend::VertexBufferHandle m_vb;
        filament::backend::IndexBufferHandle m_ib;
        filament::backend::PrimitiveType m_primitive_type;
        Vector<filament::backend::RenderPrimitiveHandle> m_primitives;
    };
}
