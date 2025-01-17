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

#include "graphics/MeshRenderer.h"
#include "graphics/Texture.h"
#include "container/Vector.h"
#include "container/Map.h"
#include "math/Recti.h"
#include "View.h"

namespace Viry3D
{
	class View;
	class Mesh;
	class Camera;
    struct Touch;

    struct AtlasTreeNode
    {
        Recti rect;
        int layer;
        Vector<AtlasTreeNode*> children;
    };

    class CanvasRenderer : public MeshRenderer
	{
	public:
		CanvasRenderer(FilterMode filter_mode);
		virtual ~CanvasRenderer();
		void AddView(const Ref<View>& view);
		void RemoveView(const Ref<View>& view);
        void RemoveAllViews();
        const Vector<Ref<View>>& GetViews() const { return m_views; }
		void MarkCanvasDirty();
		Ref<Camera> GetCamera() const { return m_camera.lock(); }
		void SetCamera(const Ref<Camera>& camera) { m_camera = camera; }

	protected:
		virtual void Prepare();
		virtual void OnResize(int width, int height);

	private:
        void CreateMaterial();
        void NewAtlasTextureLayer();
        void UpdateCanvas();
        void UpdateAtlas(ViewMesh& mesh, bool& updated);
        AtlasTreeNode* FindAtlasTreeNodeToInsert(int w, int h, AtlasTreeNode* node);
        void ReleaseAtlasTreeNode(AtlasTreeNode* node);
        void HandleTouchEvent();
        void HitViews(const Touch& t);

	private:
		Vector<Ref<View>> m_views;
		bool m_canvas_dirty;
        Vector<Ref<Texture>> m_atlases;
        Vector<AtlasTreeNode*> m_atlas_tree;
        Map<uint32_t, AtlasTreeNode*> m_atlas_cache;
        Vector<ViewMesh> m_view_meshes;
        Map<int, List<View*>> m_touch_down_views;
        FilterMode m_filter_mode;
		WeakRef<Camera> m_camera;
	};
}
