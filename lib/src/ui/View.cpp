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

#include "View.h"
#include "CanvasRenderer.h"
#include "Debug.h"
#include "memory/Memory.h"
#include "graphics/Camera.h"
#include "graphics/Texture.h"
#include "graphics/Image.h"

namespace Viry3D
{
    int ViewMesh::GetTextureOrImageWidth() const
    {
        return texture ? texture->GetWidth() : image->width;
    }

    int ViewMesh::GetTextureOrImageHeight() const
    {
        return texture ? texture->GetHeight() : image->height;
    }

	View::View():
		m_canvas(nullptr),
        m_parent_view(nullptr),
		m_color(1, 1, 1, 1),
		m_alignment(ViewAlignment::HCenter | ViewAlignment::VCenter),
		m_pivot(0.5f, 0.5f),
		m_size(100, 100),
		m_offset(0, 0),
        m_margin(0, 0, 0, 0),
        m_local_rotation(0, 0, 0),
        m_local_scale(1, 1),
        m_clip_rect(false),
        m_rect(0, 0, 0, 0),
        m_vertex_matrix(Matrix4x4::Identity())
	{
	
	}

	View::~View()
	{
	
	}

	void View::OnAddToCanvas(CanvasRenderer* canvas)
	{
		assert(m_canvas == nullptr);
		m_canvas = canvas;
	}

	void View::OnRemoveFromCanvas(CanvasRenderer* canvas)
	{
		assert(m_canvas == canvas);
		m_canvas = nullptr;
	}

    CanvasRenderer* View::GetCanvas() const
    {
        if (m_canvas)
        {
            return m_canvas; 
        }
        else if (m_parent_view)
        {
            return m_parent_view->GetCanvas();
        }

        return nullptr;
    }

    void View::MarkCanvasDirty() const
    {
        CanvasRenderer* canvas = this->GetCanvas();
        if (canvas)
        {
            canvas->MarkCanvasDirty();
        }
    }

    void View::AddSubview(const Ref<View>& view)
    {
        assert(view->m_parent_view == nullptr);
        view->m_parent_view = this;

        m_subviews.Add(view);

        this->MarkCanvasDirty();
    }

    void View::RemoveSubview(const Ref<View>& view)
    {
        assert(view->m_parent_view == this);
        view->m_parent_view = nullptr;
        
        m_subviews.Remove(view);

        this->MarkCanvasDirty();
    }

    void View::ClearSubviews()
    {
        Vector<Ref<View>> subviews;
        for (int i = 0; i < this->GetSubviewCount(); ++i)
        {
            subviews.Add(this->GetSubview(i));
        }
        for (int i = 0; i < subviews.Size(); ++i)
        {
            this->RemoveSubview(subviews[i]);
        }
    }

	void View::SetColor(const Color& color)
	{
		m_color = color;
        this->MarkCanvasDirty();
	}

	void View::SetAlignment(int alignment)
	{
		m_alignment = alignment;
        this->MarkCanvasDirty();
	}

	void View::SetPivot(const Vector2& pivot)
	{
		m_pivot = pivot;
        this->MarkCanvasDirty();
	}

	void View::SetSize(const Vector2i& size)
	{
		m_size = size;
        this->MarkCanvasDirty();
	}

    Vector2i View::GetCalculatedSize()
    {
        Vector2i size = m_size;
        
        if (size.x == VIEW_SIZE_FILL_PARENT ||
            size.y == VIEW_SIZE_FILL_PARENT)
        {
            Vector2i parent_size;

            if (m_parent_view)
            {
                parent_size = m_parent_view->GetCalculatedSize();
            }
            else
            {
                parent_size.x = m_canvas->GetCamera()->GetTargetWidth();
                parent_size.y = m_canvas->GetCamera()->GetTargetHeight();
            }

            if (size.x == VIEW_SIZE_FILL_PARENT)
            {
                size.x = parent_size.x - (int) (m_margin.x + m_margin.z);
            }

            if (size.y == VIEW_SIZE_FILL_PARENT)
            {
                size.y = parent_size.y - (int) (m_margin.y + m_margin.w);
            }
        }

        return size;
    }

	void View::SetOffset(const Vector2i& offset)
	{
		m_offset = offset;
        this->MarkCanvasDirty();
	}

    void View::SetMargin(const Vector4& margin)
    {
        m_margin = margin;
        this->MarkCanvasDirty();
    }

    void View::SetLocalRotation(const Quaternion& rot)
    {
        m_local_rotation = rot;
        this->MarkCanvasDirty();
    }

    void View::SetLocalScale(const Vector2& scale)
    {
        m_local_scale = scale;
        this->MarkCanvasDirty();
    }

    void View::EnableClipRect(bool enable)
    {
        m_clip_rect = enable;
        this->MarkCanvasDirty();
    }

    Rect View::GetClipRect() const
    {
        if (this->IsClipRect())
        {
            Rect rect = Rect((float) m_rect.x, (float) -m_rect.y, (float) m_rect.w, (float) m_rect.h);

            Vector3 vs[4];
            vs[0] = Vector3(rect.x, rect.y, 0);
            vs[1] = Vector3(rect.x, rect.y - rect.h, 0);
            vs[2] = Vector3(rect.x + rect.w, rect.y - rect.h, 0);
            vs[3] = Vector3(rect.x + rect.w, rect.y, 0);

            for (int i = 0; i < 4; ++i)
            {
                vs[i] = m_vertex_matrix.MultiplyPoint3x4(vs[i]);
            }

            float x = vs[0].x;
            float y = -vs[0].y;
            float w = vs[3].x - vs[0].x;
            float h = vs[0].y - vs[1].y;
            float canvas_w = (float) this->GetCanvas()->GetCamera()->GetTargetWidth();
            float canvas_h = (float) this->GetCanvas()->GetCamera()->GetTargetHeight();

            return Rect(x / canvas_w, y / canvas_h, w / canvas_w, h / canvas_h);
        }
        else
        {
            return Rect(0, 0, 1, 1);
        }
    }

    void View::Update()
    {
        for (auto& i : m_subviews)
        {
            i->Update();
        }
    }

    void View::UpdateLayout()
    {
        Recti parent_rect;

        if (m_parent_view)
        {
            parent_rect = m_parent_view->GetRect();
        }
        else
        {
            parent_rect = Recti(0, 0, m_canvas->GetCamera()->GetTargetWidth(), m_canvas->GetCamera()->GetTargetHeight());
        }

        Vector2i local_pos;

        if (m_alignment & ViewAlignment::Left)
        {
            local_pos.x = 0;
        }
        else if (m_alignment & ViewAlignment::HCenter)
        {
            local_pos.x = parent_rect.w / 2;
        }
        else if (m_alignment & ViewAlignment::Right)
        {
            local_pos.x = parent_rect.w;
        }

        if (m_alignment & ViewAlignment::Top)
        {
            local_pos.y = 0;
        }
        else if (m_alignment & ViewAlignment::VCenter)
        {
            local_pos.y = parent_rect.h / 2;
        }
        else if (m_alignment & ViewAlignment::Bottom)
        {
            local_pos.y = parent_rect.h;
        }

        local_pos += m_offset;

        Vector2i size = this->GetSize();

        if (size.x == VIEW_SIZE_FILL_PARENT)
        {
            m_rect.x = parent_rect.x + (int) m_margin.x;
            m_rect.w = parent_rect.w - (int) (m_margin.x + m_margin.z);
        }
        else
        {
            m_rect.x = parent_rect.x + local_pos.x - Mathf::RoundToInt(m_pivot.x * size.x);
            m_rect.w = size.x;
        }

        if (size.y == VIEW_SIZE_FILL_PARENT)
        {
            m_rect.y = parent_rect.y + (int) m_margin.y;
            m_rect.h = parent_rect.h - (int) (m_margin.y + m_margin.w);
        }
        else
        {
            m_rect.y = parent_rect.y + local_pos.y - Mathf::RoundToInt(m_pivot.y * size.y);
            m_rect.h = size.y;
        }

        this->ComputeVerticesMatrix();

        for (auto& i : m_subviews)
        {
            i->UpdateLayout();
        }
    }

    void View::OnResize(int width, int height)
    {
        for (auto& i : m_subviews)
        {
            i->UpdateLayout();
        }
    }

    void View::ComputeVerticesMatrix()
    {
        int x = m_rect.x;
        int y = -m_rect.y;

        Vector3 pivot_pos;
        pivot_pos.x = x + Mathf::Round(m_pivot.x * m_rect.w);
        pivot_pos.y = y - Mathf::Round(m_pivot.y * m_rect.h);
        pivot_pos.z = 0;

        m_vertex_matrix = Matrix4x4::Translation(pivot_pos) * Matrix4x4::Rotation(m_local_rotation) * Matrix4x4::Scaling(Vector3(m_local_scale.x, m_local_scale.y, 1)) * Matrix4x4::Translation(-pivot_pos);

        if (m_parent_view)
        {
            m_vertex_matrix = m_parent_view->GetVertexMatrix() * m_vertex_matrix;
        }
    }

    void View::FillSelfMeshes(Vector<ViewMesh>& meshes, const Rect& clip_rect)
    {
        Rect rect = Rect((float) m_rect.x, (float) -m_rect.y, (float) m_rect.w, (float) m_rect.h);

        Mesh::Vertex vs[4];
        Memory::Zero(&vs[0], sizeof(vs));
        vs[0].vertex = Vector3(rect.x, rect.y, 0);
        vs[1].vertex = Vector3(rect.x, rect.y - rect.h, 0);
        vs[2].vertex = Vector3(rect.x + rect.w, rect.y - rect.h, 0);
        vs[3].vertex = Vector3(rect.x + rect.w, rect.y, 0);
        vs[0].color = m_color;
        vs[1].color = m_color;
        vs[2].color = m_color;
        vs[3].color = m_color;
        vs[0].uv = Vector2(0, 0);
        vs[1].uv = Vector2(0, 1);
        vs[2].uv = Vector2(1, 1);
        vs[3].uv = Vector2(1, 0);

        for (int i = 0; i < 4; ++i)
        {
            vs[i].vertex = m_vertex_matrix.MultiplyPoint3x4(vs[i].vertex);
        }

        ViewMesh mesh;
        mesh.vertices.AddRange({ vs[0], vs[1], vs[2], vs[3] });
        mesh.indices.AddRange({ 0, 1, 2, 0, 2, 3 });
        mesh.view = this;
        mesh.base_view = true;
        mesh.clip_rect = Rect::Min(this->GetClipRect(), clip_rect);

        meshes.Add(mesh);
    }

    void View::FillMeshes(Vector<ViewMesh>& meshes, const Rect& clip_rect)
    {
        this->FillSelfMeshes(meshes, clip_rect);

        Rect clip = Rect::Min(this->GetClipRect(), clip_rect);

        for (auto& i : m_subviews)
        {
            i->FillMeshes(meshes, clip);
        }
    }

    bool View::OnTouchDownInside(const Vector2i& pos) const
    {
        if (m_on_touch_down_inside)
        {
            return m_on_touch_down_inside(pos);
        }
        return false;
    }

    bool View::OnTouchMoveInside(const Vector2i& pos) const
    {
        if (m_on_touch_move_inside)
        {
            return m_on_touch_move_inside(pos);
        }
        return false;
    }
    
    bool View::OnTouchUpInside(const Vector2i& pos) const
    {
        if (m_on_touch_up_inside)
        {
            return m_on_touch_up_inside(pos);
        }
        return false;
    }

    bool View::OnTouchUpOutside(const Vector2i& pos) const
    {
        if (m_on_touch_up_outside)
        {
            return m_on_touch_up_outside(pos);
        }
        return false;
    }

    bool View::OnTouchDrag(const Vector2i& pos) const
    {
        if (m_on_touch_drag)
        {
            return m_on_touch_drag(pos);
        }
        return false;
    }
}
