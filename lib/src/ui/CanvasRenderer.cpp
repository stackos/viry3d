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

#include "CanvasRenderer.h"
#include "View.h"
#include "Debug.h"
#include "Input.h"
#include "Engine.h"
#include "graphics/Mesh.h"
#include "graphics/Shader.h"
#include "graphics/Material.h"
#include "graphics/Camera.h"
#include "graphics/Material.h"
#include "graphics/Texture.h"
#include "graphics/Image.h"
#include "memory/Memory.h"
#include "container/List.h"

#define ATLAS_SIZE 2048
#define PADDING_SIZE 1

namespace Viry3D
{
	CanvasRenderer::CanvasRenderer(FilterMode filter_mode):
		m_canvas_dirty(true),
        m_filter_mode(filter_mode)
	{
		this->CreateMaterial();
        this->NewAtlasTextureLayer();
	}

	CanvasRenderer::~CanvasRenderer()
	{
        for (int i = 0; i < m_atlas_tree.Size(); ++i)
        {
            this->ReleaseAtlasTreeNode(m_atlas_tree[i]);
            delete m_atlas_tree[i];
        }
        m_atlas_tree.Clear();
	}

    void CanvasRenderer::ReleaseAtlasTreeNode(AtlasTreeNode* node)
    {
        for (int i = 0; i < node->children.Size(); ++i)
        {
            this->ReleaseAtlasTreeNode(node->children[i]);
            delete node->children[i];
        }
        node->children.Clear();
    }

    void CanvasRenderer::CreateMaterial()
    {
        auto material = RefMake<Material>(Shader::Find("UI"));
        material->SetColor(MaterialProperty::COLOR, Color(1, 1, 1, 1));

        this->SetMaterial(material);
    }

    void CanvasRenderer::NewAtlasTextureLayer()
    {
        ByteBuffer buffer(ATLAS_SIZE * ATLAS_SIZE * 4);
        Memory::Set(&buffer[0], 0, buffer.Size());

        auto atlas = Texture::CreateTexture2DFromMemory(
            buffer,
            ATLAS_SIZE,
            ATLAS_SIZE,
            TextureFormat::R8G8B8A8,
            m_filter_mode,
            SamplerAddressMode::ClampToEdge,
            false);
        m_atlases.Add(atlas);

        AtlasTreeNode* layer = new AtlasTreeNode();
        layer->rect.x = 0;
        layer->rect.y = 0;
        layer->rect.w = ATLAS_SIZE;
        layer->rect.h = ATLAS_SIZE;
        layer->layer = m_atlas_tree.Size();
        m_atlas_tree.Add(layer);
    }

	void CanvasRenderer::Prepare()
	{
        this->HandleTouchEvent();
        
        for (int i = 0; i < m_views.Size(); ++i)
        {
            m_views[i]->Update();
        }

		if (m_canvas_dirty)
		{
			m_canvas_dirty = false;

            this->GetCamera()->SetNearClip(-1000);
            this->GetCamera()->SetFarClip(1000);
            this->GetCamera()->SetOrthographic(true);
            this->GetCamera()->SetOrthographicSize(this->GetCamera()->GetTargetHeight() / 2.0f);

            // set custom projection matrix,
            // make camera position in left top of view rect instead center,
            // to avoid half pixel problem.
            {
                float view_width = this->GetCamera()->GetTargetWidth() * this->GetCamera()->GetViewportRect().w;
                float view_height = this->GetCamera()->GetTargetHeight() * this->GetCamera()->GetViewportRect().h;

                float top = 0;
                float bottom = (float) -this->GetCamera()->GetTargetHeight();
                float left = 0;
                float right = (float) this->GetCamera()->GetTargetHeight() * view_width / view_height;
                auto projection_matrix = Matrix4x4::Ortho(left, right, bottom, top, this->GetCamera()->GetNearClip(), this->GetCamera()->GetFarClip());
                
                this->GetCamera()->SetProjectionMatrixExternal(projection_matrix);
            }

            this->UpdateCanvas();
		}
        
        MeshRenderer::Prepare();
	}

    void CanvasRenderer::OnResize(int width, int height)
    {
        for (int i = 0; i < m_views.Size(); ++i)
        {
            m_views[i]->OnResize(width, height);
        }

        this->MarkCanvasDirty();
    }

	void CanvasRenderer::AddView(const Ref<View>& view)
	{
		m_views.Add(view);
		view->OnAddToCanvas(this);
		this->MarkCanvasDirty();
	}

	void CanvasRenderer::RemoveView(const Ref<View>& view)
	{
		m_views.Remove(view);
		view->OnRemoveFromCanvas(this);
		this->MarkCanvasDirty();
	}

    void CanvasRenderer::RemoveAllViews()
    {
        Vector<Ref<View>> views = m_views;
        for (const auto& i : views)
        {
            this->RemoveView(i);
        }
    }

	void CanvasRenderer::MarkCanvasDirty()
	{
		m_canvas_dirty = true;
	}

    void CanvasRenderer::UpdateCanvas()
    {
        m_view_meshes.Clear();

        for (int i = 0; i < m_views.Size(); ++i)
        {
            m_views[i]->UpdateLayout();
            m_views[i]->FillMeshes(m_view_meshes, Rect(0, 0, 1, 1));
        }

        List<ViewMesh*> mesh_list;

        for (int i = 0; i < m_view_meshes.Size(); ++i)
        {
            mesh_list.AddLast(&m_view_meshes[i]);
        }

        mesh_list.Sort([](const ViewMesh* a, const ViewMesh* b) {
            if (!a->HasTextureOrImage() && b->HasTextureOrImage())
            {
                return true;
            }
            else if (a->HasTextureOrImage() && !b->HasTextureOrImage())
            {
                return false;
            }
            else if (!a->HasTextureOrImage() && !b->HasTextureOrImage())
            {
                return false;
            }
            else
            {
                if (a->GetTextureOrImageWidth() == b->GetTextureOrImageWidth())
                {
                    return a->GetTextureOrImageHeight() > b->GetTextureOrImageHeight();
                }
                else
                {
                    return a->GetTextureOrImageWidth() > b->GetTextureOrImageWidth();
                }
            }
        });

        bool atlas_updated = false;
        for (auto i : mesh_list)
        {
            if (i->texture || i->image)
            {
                bool updated;
                this->UpdateAtlas(*i, updated);

                if (updated)
                {
                    atlas_updated = true;
                }
            }
        }

        Vector<Mesh::Submesh> submeshes;
        Vector<int> texture_layers;
        Vector<Rect> clip_rects;
        Vector<Mesh::Vertex> vertices;
        Vector<unsigned int> indices;

        for (const auto& i : m_view_meshes)
        {
            if (i.vertices.Size() > 0 && i.indices.Size() > 0 && (i.texture || i.image))
            {
                int index_offset = vertices.Size();
                
                if (clip_rects.Size() == 0 || i.clip_rect != clip_rects[clip_rects.Size() - 1])
                {
                    clip_rects.Add(i.clip_rect);

                    Mesh::Submesh submesh;
                    submesh.index_first = indices.Size();
                    submesh.index_count = i.indices.Size();
                    submeshes.Add(submesh);
                    
                    texture_layers.Add((int) i.vertices[0].uv2.x);
                }
                else
                {
                    if ((int) i.vertices[0].uv2.x != texture_layers[texture_layers.Size() - 1])
                    {
                        clip_rects.Add(i.clip_rect);

                        Mesh::Submesh submesh;
                        submesh.index_first = indices.Size();
                        submesh.index_count = i.indices.Size();
                        submeshes.Add(submesh);
                        
                        texture_layers.Add((int) i.vertices[0].uv2.x);
                    }
                    else
                    {
                        submeshes[submeshes.Size() - 1].index_count += i.indices.Size();
                    }
                }

                vertices.AddRange(i.vertices);

                for (int j = 0; j < i.indices.Size(); ++j)
                {
                    indices.Add(index_offset + i.indices[j]);
                }
            }
        }

        auto mesh = this->GetMesh();
        if (vertices.Size() > 0 && indices.Size() > 0)
        {
            if (!mesh || vertices.Size() > mesh->GetVertices().Size() || indices.Size() > mesh->GetIndices().Size())
            {
                mesh = RefMake<Mesh>(std::move(vertices), std::move(indices), submeshes, false, true);
                this->SetMesh(mesh);
            }
            else
            {
                mesh->Update(std::move(vertices), std::move(indices), submeshes);
            }
        }
        else
        {
            if (mesh)
            {
                mesh.reset();
                this->SetMesh(mesh);
            }
        }

        // update materials
        if (this->GetMaterials().Size() != submeshes.Size())
        {
            Vector<Ref<Material>> materials(submeshes.Size());
            for (int i = 0; i < materials.Size(); ++i)
            {
                materials[i] = RefMake<Material>(Shader::Find("UI"));
				materials[i]->SetScissorRect(clip_rects[i]);
                materials[i]->SetTexture(MaterialProperty::TEXTURE, m_atlases[texture_layers[i]]);
			}
            this->SetMaterials(materials);
        }

        {
            const auto& materials = this->GetMaterials();
            for (int i = 0; i < materials.Size(); ++i)
            {
                if (clip_rects[i] != materials[i]->GetScissorRect())
                {
					materials[i]->SetScissorRect(clip_rects[i]);
                }
                materials[i]->SetTexture(MaterialProperty::TEXTURE, m_atlases[texture_layers[i]]);
            }
        }

#if 0
        // test output atlas texture
        if (atlas_updated)
        {
            for (int i = 0; i < m_atlases.Size(); i++)
            {
                ByteBuffer pixels(ATLAS_SIZE * ATLAS_SIZE * 4);
                m_atlases[i]->CopyToMemory(pixels, 0, 0, 0, 0, ATLAS_SIZE, ATLAS_SIZE, [i](const ByteBuffer& buffer) {
                    auto image = RefMake<Image>();
                    image->width = ATLAS_SIZE;
                    image->height = ATLAS_SIZE;
                    image->format = ImageFormat::R8G8B8A8;
                    image->data = buffer;
                    image->EncodeToPNG(String::Format("%s/atlas_%d.png", Engine::Instance()->GetSavePath().CString(), i));
                });
            }
        }
#endif
    }

    void CanvasRenderer::UpdateAtlas(ViewMesh& mesh, bool& updated)
    {
        int texture_width = mesh.GetTextureOrImageWidth();
        int texture_height = mesh.GetTextureOrImageHeight();

        assert(texture_width <= ATLAS_SIZE - PADDING_SIZE && texture_height <= ATLAS_SIZE - PADDING_SIZE);

        AtlasTreeNode* node = nullptr;

        AtlasTreeNode** node_ptr;
        if ((mesh.texture && m_atlas_cache.TryGet(mesh.texture->GetId(), &node_ptr)) ||
            (mesh.image && m_atlas_cache.TryGet(mesh.image->GetId(), &node_ptr)))
        {
            node = *node_ptr;

            updated = false;
        }
        else
        {
            for (int i = 0; i < m_atlas_tree.Size(); ++i)
            {
                node = this->FindAtlasTreeNodeToInsert(texture_width, texture_height, m_atlas_tree[i]);
                if (node)
                {
                    break;
                }
            }

            if (node == nullptr)
            {
                this->NewAtlasTextureLayer();

                node = this->FindAtlasTreeNodeToInsert(texture_width, texture_height, m_atlas_tree[m_atlas_tree.Size() - 1]);
            }

            assert(node);

            // split node
            AtlasTreeNode* left = new AtlasTreeNode();
            AtlasTreeNode* right = new AtlasTreeNode();

            int remain_w = node->rect.w - texture_width - PADDING_SIZE;
            int remain_h = node->rect.h - texture_height - PADDING_SIZE;

            if (remain_w <= remain_h)
            {
                left->rect.x = node->rect.x + texture_width + PADDING_SIZE;
                left->rect.y = node->rect.y;
                left->rect.w = remain_w;
                left->rect.h = texture_height;
                left->layer = node->layer;

                right->rect.x = node->rect.x;
                right->rect.y = node->rect.y + texture_height + PADDING_SIZE;
                right->rect.w = node->rect.w;
                right->rect.h = remain_h;
                right->layer = node->layer;
            }
            else
            {
                left->rect.x = node->rect.x;
                left->rect.y = node->rect.y + texture_height + PADDING_SIZE;
                left->rect.w = texture_width;
                left->rect.h = remain_h;
                left->layer = node->layer;

                right->rect.x = node->rect.x + texture_width + PADDING_SIZE;
                right->rect.y = node->rect.y;
                right->rect.w = remain_w;
                right->rect.h = node->rect.h;
                right->layer = node->layer;
            }

            node->rect.w = texture_width;
            node->rect.h = texture_height;
            node->children.Resize(2);
            node->children[0] = left;
            node->children[1] = right;

            // copy texture to atlas
            // add cache
            if (mesh.texture)
            {
                m_atlases[node->layer]->CopyTexture(
					0, 0,
					node->rect.x, node->rect.y,
					node->rect.w, node->rect.h,
                    mesh.texture,
                    0, 0,
                    0, 0,
                    mesh.texture->GetWidth(), mesh.texture->GetHeight(),
					FilterMode::None);

                m_atlas_cache.Add(mesh.texture->GetId(), node);
            }
            else if (mesh.image)
            {
                m_atlases[node->layer]->UpdateTexture(
                    mesh.image->data,
					0, 0,
                    node->rect.x, node->rect.y,
                    node->rect.w, node->rect.h);

                m_atlas_cache.Add(mesh.image->GetId(), node);
            }

            updated = true;
        }

        // update uv
        Vector2 uv_offset(node->rect.x / (float) ATLAS_SIZE, node->rect.y / (float) ATLAS_SIZE);
        Vector2 uv_scale(node->rect.w / (float) ATLAS_SIZE, node->rect.h / (float) ATLAS_SIZE);

        for (int i = 0; i < mesh.vertices.Size(); ++i)
        {
            mesh.vertices[i].uv.x = mesh.vertices[i].uv.x * uv_scale.x + uv_offset.x;
            mesh.vertices[i].uv.y = mesh.vertices[i].uv.y * uv_scale.y + uv_offset.y;
            mesh.vertices[i].uv2.x = (float) node->layer;
        }
    }

    AtlasTreeNode* CanvasRenderer::FindAtlasTreeNodeToInsert(int w, int h, AtlasTreeNode* node)
    {
        if (node->children.Size() == 0)
        {
            if (node->rect.w - PADDING_SIZE >= w && node->rect.h - PADDING_SIZE >= h)
            {
                return node;
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            AtlasTreeNode* left = this->FindAtlasTreeNodeToInsert(w, h, node->children[0]);
            if (left)
            {
                return left;
            }
            else
            {
                return this->FindAtlasTreeNodeToInsert(w, h, node->children[1]);
            }
        }
    }

    void CanvasRenderer::HandleTouchEvent()
    {
        int touch_count = Input::GetTouchCount();
        for (int i = 0; i < touch_count; ++i)
        {
            this->HitViews(Input::GetTouch(i));
        }
    }

    static bool IsPointInView(const Vector2i& pos, const Vector<Mesh::Vertex>& vertices)
    {
        // ax + by + c = 0
        // a = y1 - y0
        // b = x0 - x1
        // c = x1y0 - x0y1
        float x0 = vertices[0].vertex.x;
        float y0 = vertices[0].vertex.y;
        float x1 = vertices[1].vertex.x;
        float y1 = vertices[1].vertex.y;
        float x2 = vertices[2].vertex.x;
        float y2 = vertices[2].vertex.y;
        float x3 = vertices[3].vertex.x;
        float y3 = vertices[3].vertex.y;
        Vector3 lines[4];
        lines[0] = Vector3(y1 - y0, x0 - x1, x1 * y0 - x0 * y1);
        lines[1] = Vector3(y2 - y1, x1 - x2, x2 * y1 - x1 * y2);
        lines[2] = Vector3(y3 - y2, x2 - x3, x3 * y2 - x2 * y3);
        lines[3] = Vector3(y0 - y3, x3 - x0, x0 * y3 - x3 * y0);

        bool all_positive = true;
        bool all_negative = true;

        for (int i = 0; i < 4; ++i)
        {
            float sign = lines[i].x * pos.x + lines[i].y * pos.y + lines[i].z;
            if (sign >= 0)
            {
                all_negative = false;
            }
            if (sign <= 0)
            {
                all_positive = false;
            }
        }

        return all_positive || all_negative;
    }

    void CanvasRenderer::HitViews(const Touch& t)
    {
        Vector2i pos = Vector2i((int) t.position.x, (int) t.position.y);
        pos.y -= this->GetCamera()->GetTargetHeight();

        if (t.phase == TouchPhase::Began)
        {
            for (int i = m_view_meshes.Size() - 1; i >= 0; --i)
            {
                if (m_view_meshes[i].base_view)
                {
                    if (IsPointInView(pos, m_view_meshes[i].vertices))
                    {
                        View* view = m_view_meshes[i].view;

                        List<View*>* touch_down_views_ptr;
                        if (m_touch_down_views.TryGet(t.fingerId, &touch_down_views_ptr))
                        {
                            touch_down_views_ptr->AddLast(view);
                        }
                        else
                        {
                            List<View*> views;
                            views.AddLast(view);
                            m_touch_down_views.Add(t.fingerId, views);
                        }

                        bool block_event = view->OnTouchDownInside(pos);

                        if (block_event)
                        {
                            break;
                        }
                    }
                }
            }
        }
        else if (t.phase == TouchPhase::Moved)
        {
            List<View*>* touch_down_views_ptr;
            if (m_touch_down_views.TryGet(t.fingerId, &touch_down_views_ptr))
            {
                for (View* j : *touch_down_views_ptr)
                {
                    bool block_event = j->OnTouchDrag(pos);

                    if (block_event)
                    {
                        break;
                    }
                }
            }

            for (int i = m_view_meshes.Size() - 1; i >= 0; --i)
            {
                if (m_view_meshes[i].base_view)
                {
                    if (IsPointInView(pos, m_view_meshes[i].vertices))
                    {
                        View* view = m_view_meshes[i].view;

                        bool block_event = view->OnTouchMoveInside(pos);

                        if (block_event)
                        {
                            break;
                        }
                    }
                }
            }
        }
        else if (t.phase == TouchPhase::Ended)
        {
            bool blocked = false;
            for (int i = m_view_meshes.Size() - 1; i >= 0; --i)
            {
                if (m_view_meshes[i].base_view)
                {
                    View* view = m_view_meshes[i].view;

                    if (!blocked)
                    {
                        if (IsPointInView(pos, m_view_meshes[i].vertices))
                        {
                            bool block_event = view->OnTouchUpInside(pos);

                            if (block_event)
                            {
                                blocked = true;
                            }
                        }
                    }
                    else
                    {
                        view->OnTouchUpOutside(pos);
                    }
                }
            }

            m_touch_down_views.Remove(t.fingerId);
        }
    }
}
