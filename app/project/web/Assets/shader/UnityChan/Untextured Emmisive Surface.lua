local vs = [[
VK_UNIFORM_BINDING(0) uniform PerView
{
	mat4 u_view_matrix;
    mat4 u_projection_matrix;
};
VK_UNIFORM_BINDING(1) uniform PerRenderer
{
	mat4 u_model_matrix;
};
layout(location = 0) in vec4 i_vertex;
layout(location = 4) in vec3 i_normal;
VK_LAYOUT_LOCATION(0) out vec3 v_pos;
VK_LAYOUT_LOCATION(2) out vec3 v_normal;

void main()
{
    mat4 model_matrix = u_model_matrix;

	vec4 world_pos = vec4(i_vertex.xyz, 1.0) * model_matrix;
	gl_Position = world_pos * u_view_matrix * u_projection_matrix;
	v_pos = world_pos.xyz;
    v_normal = (vec4(i_normal, 0.0) * model_matrix).xyz;

	vk_convert();
}
]]

local fs = [[
#ifndef LIGHT_ADD_ON
	#define LIGHT_ADD_ON 0
#endif

precision highp float;
VK_UNIFORM_BINDING(4) uniform PerMaterialFragment
{
    vec4 _Color;
    vec4 _Emission;
    vec4 _Amplitude;
};
VK_UNIFORM_BINDING(6) uniform PerLightFragment
{
	vec4 u_ambient_color;
    vec4 u_light_pos;
    vec4 u_light_color;
	vec4 u_light_atten;
	vec4 u_spot_light_dir;
};
VK_LAYOUT_LOCATION(0) in vec3 v_pos;
VK_LAYOUT_LOCATION(2) in vec3 v_normal;

layout(location = 0) out vec4 o_color;
void main()
{
    vec3 normal = normalize(v_normal);
	vec3 to_light = u_light_pos.xyz - v_pos * u_light_pos.w;
	vec3 light_dir = normalize(to_light);
    float nl = max(dot(normal, light_dir), 0.0);
	vec4 c = _Color;

	float sqr_len = dot(to_light, to_light);
	float atten = 1.0 - sqr_len * u_light_atten.z;
	int light_type = int(u_light_color.a);
	if (light_type == 1)
	{
		float theta = dot(light_dir, u_spot_light_dir.xyz);
		if (theta > u_light_atten.x)
		{
			atten *= clamp((u_light_atten.x - theta) * u_light_atten.y, 0.0, 1.0);
		}
		else
		{
			atten = 0.0;
		}
	}

	vec3 diffuse = c.rgb * nl * u_light_color.rgb * atten;

#if (LIGHT_ADD_ON == 1)
	c.rgb = diffuse;
#else
	vec3 emission = _Emission.rgb * _Amplitude.x;

	vec3 ambient = c.rgb * u_ambient_color.rgb;
	c.rgb = ambient + diffuse + emission;
#endif

	c.a = 1.0;
    o_color = c;
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
	LightMode
		None | Forward
]]

local rs = {
    Cull = Back,
    ZTest = LEqual,
    ZWrite = On,
    SrcBlendMode = One,
    DstBlendMode = Zero,
	CWrite = On,
    Queue = Geometry,
	LightMode = Forward,
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
            name = "PerMaterialFragment",
            binding = 4,
            members = {
                {
                    name = "_Color",
                    size = 16,
                },
                {
                    name = "_Emission",
                    size = 16,
                },
                {
                    name = "_Amplitude",
                    size = 16,
                },
            },
        },
        {
            name = "PerLightFragment",
            binding = 6,
            members = {
				{
                    name = "u_ambient_color",
                    size = 16,
                },
                {
                    name = "u_light_pos",
                    size = 16,
                },
                {
                    name = "u_light_color",
                    size = 16,
                },
				{
                    name = "u_light_atten",
                    size = 16,
                },
				{
                    name = "u_spot_light_dir",
                    size = 16,
                },
            },
        },
	},
	samplers = {
	},
}

-- return pass array
return {
    pass
}
