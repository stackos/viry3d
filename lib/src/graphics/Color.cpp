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

#include "Color.h"
#include "math/Mathf.h"

namespace Viry3D
{
    const Color& Color::White()
    {
        static const Color s_color(1, 1, 1, 1);
        return s_color;
    }

    const Color& Color::Black()
    {
        static const Color s_color(0, 0, 0, 1);
        return s_color;
    }

    const Color& Color::Red()
    {
        static const Color s_color(1, 0, 0, 1);
        return s_color;
    }

    const Color& Color::Green()
    {
        static const Color s_color(0, 1, 0, 1);
        return s_color;
    }

    const Color& Color::Blue()
    {
        static const Color s_color(0, 0, 1, 1);
        return s_color;
    }

	Color::Color(float r, float g, float b, float a):
		r(r), g(g), b(b), a(a)
	{
	}

	bool Color::operator ==(const Color& c) const
	{
		return
			Mathf::FloatEqual(r, c.r) &&
			Mathf::FloatEqual(g, c.g) &&
			Mathf::FloatEqual(b, c.b) &&
			Mathf::FloatEqual(a, c.a);
	}

	bool Color::operator !=(const Color& c) const
	{
		return !(*this == c);
	}

	Color Color::Lerp(const Color& from, const Color& to, float t, bool clamp_01)
	{
		if (clamp_01)
		{
			t = Mathf::Clamp01(t);
		}

		return Color(
			Mathf::Lerp(from.r, to.r, t),
			Mathf::Lerp(from.g, to.g, t),
			Mathf::Lerp(from.b, to.b, t),
			Mathf::Lerp(from.a, to.a, t));
	}

    Color Color::Parse(const String& str)
    {
        auto str_lower = str.ToLower();
        std::stringstream ss;
        ss << std::hex << str_lower.CString();
        unsigned int color_i = 0;
        ss >> color_i;

        int r = (color_i & 0xff000000) >> 24;
        int g = (color_i & 0xff0000) >> 16;
        int b = (color_i & 0xff00) >> 8;
        int a = (color_i & 0xff);

        float div = 1 / 255.f;
        return Color((float) r, (float) g, (float) b, (float) a) * div;
    }

	Color Color::operator *(const Color& c) const
	{
		return Color(r * c.r, g * c.g, b * c.b, a * c.a);
	}

	Color& Color::operator *=(const Color& c)
	{
		r *= c.r;
		g *= c.g;
		b *= c.b;
		a *= c.a;
		return *this;
	}

	Color Color::operator *(float v) const
	{
		return Color(r * v, g * v, b * v, a * v);
	}

	Color Color::operator /(float v) const
	{
		return *this * (1 / v);
	}
}
