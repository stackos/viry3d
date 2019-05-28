local vs = [[
#ifndef SKIN_ON
	#define SKIN_ON 0
#endif

VK_UNIFORM_BINDING(0) uniform PerView
{
	mat4 u_view_matrix;
    mat4 u_projection_matrix;
	vec4 u_camera_pos;
};
VK_UNIFORM_BINDING(1) uniform PerRenderer
{
	mat4 u_model_matrix;
	vec4 u_lightmap_scale_offset;
	int u_lightmap_index;
};
VK_UNIFORM_BINDING(5) uniform PerMaterialInstance
{
	vec4 u_texture_scale_offset;
};
layout(location = 0) in vec4 i_vertex;
layout(location = 2) in vec2 i_uv;
VK_LAYOUT_LOCATION(0) out vec2 v_uv;

#if (SKIN_ON == 1)
	VK_UNIFORM_BINDING(2) uniform PerRendererBones
	{
		vec4 u_bones[210];
	};
	layout(location = 6) in vec4 i_bone_weights;
	layout(location = 7) in vec4 i_bone_indices;
	mat4 skin_mat()
	{
		int index_0 = int(i_bone_indices.x);
		int index_1 = int(i_bone_indices.y);
		int index_2 = int(i_bone_indices.z);
		int index_3 = int(i_bone_indices.w);
		float weights_0 = i_bone_weights.x;
		float weights_1 = i_bone_weights.y;
		float weights_2 = i_bone_weights.z;
		float weights_3 = i_bone_weights.w;
		mat4 bone_0 = mat4(u_bones[index_0*3], u_bones[index_0*3+1], u_bones[index_0*3+2], vec4(0, 0, 0, 1));
		mat4 bone_1 = mat4(u_bones[index_1*3], u_bones[index_1*3+1], u_bones[index_1*3+2], vec4(0, 0, 0, 1));
		mat4 bone_2 = mat4(u_bones[index_2*3], u_bones[index_2*3+1], u_bones[index_2*3+2], vec4(0, 0, 0, 1));
		mat4 bone_3 = mat4(u_bones[index_3*3], u_bones[index_3*3+1], u_bones[index_3*3+2], vec4(0, 0, 0, 1));
		return bone_0 * weights_0 + bone_1 * weights_1 + bone_2 * weights_2 + bone_3 * weights_3;
	}
#endif

void main()
{
#if (SKIN_ON == 1)
    mat4 model_matrix = skin_mat();
#else
    mat4 model_matrix = u_model_matrix;
#endif
	gl_Position = i_vertex * model_matrix * u_view_matrix * u_projection_matrix;
	v_uv = i_uv * u_texture_scale_offset.xy + u_texture_scale_offset.zw;

	vk_convert();
}
]]

local fs = [[
precision highp float;
VK_SAMPLER_BINDING(0) uniform sampler2D u_texture;
VK_LAYOUT_LOCATION(0) in vec2 v_uv;
layout(location = 0) out vec4 o_color;
void main()
{
	o_color = texture(u_texture, v_uv);
}
]]

--[[
    Cull
	    Back | Front | Off
    ZTest
	    Less | Greater | LEqual | GEqual | Equal | NotEqual | Always
    ZWrite
	    On | Off
    SrcBlendMode
	DstBlendMode
	    One | Zero | SrcColor | SrcAlpha | DstColor | DstAlpha
		| OneMinusSrcColor | OneMinusSrcAlpha | OneMinusDstColor | OneMinusDstAlpha
	CWrite
		On | Off
	Queue
		Background | Geometry | AlphaTest | Transparent | Overlay
]]

local rs = {
    Cull = Back,
    ZTest = LEqual,
    ZWrite = On,
    SrcBlendMode = One,
    DstBlendMode = Zero,
	CWrite = On,
    Queue = Geometry,
}

local pass = {
    vs = vs,
    fs = fs,
    rs = rs,
	uniforms = {
		{
			name = "PerView",
			binding = 0,
			members = {
				{
					name = "u_view_matrix",
					size = 64,
				},
                {
                    name = "u_projection_matrix",
                    size = 64,
                },
				{
                    name = "u_camera_pos",
                    size = 16,
                },
			},
		},
		{
			name = "PerRenderer",
			binding = 1,
			members = {
				{
					name = "u_model_matrix",
					size = 64,
				},
				{
					name = "u_lightmap_scale_offset",
					size = 16,
				},
				{
					name = "u_lightmap_index",
					size = 4,
				},
			},
		},
        {
            name = "PerRendererBones",
            binding = 2,
            members = {
                {
                    name = "u_bones",
                    size = 16 * 210,
                },
            },
        },
		{
            name = "PerMaterialInstance",
            binding = 5,
            members = {
                {
                    name = "u_texture_scale_offset",
                    size = 16,
                },
            },
        },
	},
	samplers = {
		{
			name = "u_texture",
			binding = 0,
		},
	},
}

-- return pass array
return {
    pass
}