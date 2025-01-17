local vs = [[
#ifndef SKIN_ON
	#define SKIN_ON 0
#endif
#ifndef BLEND_SHAPE_ON
	#define BLEND_SHAPE_ON 0
#endif

attribute vec4 i_vertex;
attribute vec2 i_uv;
varying vec2 v_uv;

#if (SKIN_ON == 1)
	uniform vec4 u_bones[210];
	attribute vec4 i_bone_weights;
	attribute vec4 i_bone_indices;
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

#if (BLEND_SHAPE_ON == 1)
    uniform vec4 u_bones[210];
    uniform sampler2D u_blend_shape_texture;
    vec4 blend_shape()
    {
        vec4 vertex = i_vertex;

        const int weight_count_max = 100;
        int weight_count = int(u_bones[0].x) - 2;
        int vertex_count = int(u_bones[0].y);
        int blend_shape_count = int(u_bones[0].z);
        int texture_width = int(u_bones[1].x);
        int texture_height = int(u_bones[1].y);
        int vertex_index = int(vertex.w);

        for (int i = 0; i < weight_count_max; ++i)
        {
            if (i >= weight_count)
            {
                break;
            }

            int blend_shape_index = int(u_bones[2 + i].x);
            float weight = u_bones[2 + i].y;

            int vector_index = vertex_count * blend_shape_index + vertex_index;
            float x = 0.0;
            if (texture_width > 1)
            {
                x = float(mod(float(vector_index), float(texture_width))) / float(texture_width - 1);
            }
            float y = 0.0;
            if (texture_height > 1)
            {
                y = float(vector_index / texture_width) / float(texture_height - 1);
            }
            vec4 vertex_offset = texture2D(u_blend_shape_texture, vec2(x, y));
            vertex.xyz += vertex_offset.xyz * weight;
        }

        return vertex;
    }
#endif

uniform mat4 u_view_matrix;
uniform mat4 u_projection_matrix;
uniform mat4 u_model_matrix;
uniform vec4 u_texture_scale_offset;

void main()
{
#if (SKIN_ON == 1)
    mat4 model_matrix = skin_mat();
#else
    mat4 model_matrix = u_model_matrix;
#endif
#if (BLEND_SHAPE_ON == 1)
    vec4 vertex = blend_shape();
#else
    vec4 vertex = i_vertex;
#endif
	gl_Position = vec4(vertex.xyz, 1.0) * model_matrix * u_view_matrix * u_projection_matrix;
	v_uv = i_uv * u_texture_scale_offset.xy + u_texture_scale_offset.zw;

	vk_convert();
}
]]

local fs = [[
precision highp float;
uniform sampler2D u_texture;
uniform vec4 u_color;

varying vec2 v_uv;

void main()
{
	vec4 c = texture2D(u_texture, v_uv) * u_color;
	c.a = 0.0;
	gl_FragColor = c;
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
            name = "PerMaterialVertex",
            binding = 3,
            members = {
                {
                    name = "u_texture_scale_offset",
                    size = 16,
                },
            },
        },
		{
            name = "PerMaterialFragment",
            binding = 4,
            members = {
                {
                    name = "u_color",
                    size = 16,
                },
            },
        },
	},
	samplers = {
		{
			name = "PerRendererBones",
			binding = 2,
			samplers = {
				{
					name = "u_blend_shape_texture",
					binding = 1,
				},
			},
		},
		{
			name = "PerMaterialFragment",
			binding = 4,
			samplers = {
				{
					name = "u_texture",
					binding = 0,
				},
			},
		},
	},
}

-- return pass array
return {
    pass
}
