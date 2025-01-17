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

#include "json/json.h"
#include "io/File.h"
#include "io/MemoryStream.h"
#include "memory/Memory.h"
#include "graphics/Image.h"
#include "math/Mathf.h"
#include "Compressonator.h"

using namespace Viry3D;

#define COMPRESSED_RGB_S3TC_DXT1 0x83F0
#define COMPRESSED_RGBA_S3TC_DXT1 0x83F1
#define COMPRESSED_RGBA_S3TC_DXT3 0x83F2
#define COMPRESSED_RGBA_S3TC_DXT5 0x83F3

#define COMPRESSED_RGB8_ETC2 0x9274
#define COMPRESSED_SRGB8_ETC2 0x9275
#define COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#define COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#define COMPRESSED_RGBA8_ETC2_EAC 0x9278
#define COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279

#define COMPRESSED_RGBA_ASTC_4x4 0x93B0
#define COMPRESSED_RGBA_ASTC_5x4 0x93B1
#define COMPRESSED_RGBA_ASTC_5x5 0x93B2
#define COMPRESSED_RGBA_ASTC_6x5 0x93B3
#define COMPRESSED_RGBA_ASTC_6x6 0x93B4
#define COMPRESSED_RGBA_ASTC_8x5 0x93B5
#define COMPRESSED_RGBA_ASTC_8x6 0x93B6
#define COMPRESSED_RGBA_ASTC_8x8 0x93B7
#define COMPRESSED_RGBA_ASTC_10x5 0x93B8
#define COMPRESSED_RGBA_ASTC_10x6 0x93B9
#define COMPRESSED_RGBA_ASTC_10x8 0x93BA
#define COMPRESSED_RGBA_ASTC_10x10 0x93BB
#define COMPRESSED_RGBA_ASTC_12x10 0x93BC
#define COMPRESSED_RGBA_ASTC_12x12 0x93BD
#define COMPRESSED_SRGB8_ALPHA8_ASTC_4x4 0x93D0
#define COMPRESSED_SRGB8_ALPHA8_ASTC_5x4 0x93D1
#define COMPRESSED_SRGB8_ALPHA8_ASTC_5x5 0x93D2
#define COMPRESSED_SRGB8_ALPHA8_ASTC_6x5 0x93D3
#define COMPRESSED_SRGB8_ALPHA8_ASTC_6x6 0x93D4
#define COMPRESSED_SRGB8_ALPHA8_ASTC_8x5 0x93D5
#define COMPRESSED_SRGB8_ALPHA8_ASTC_8x6 0x93D6
#define COMPRESSED_SRGB8_ALPHA8_ASTC_8x8 0x93D7
#define COMPRESSED_SRGB8_ALPHA8_ASTC_10x5 0x93D8
#define COMPRESSED_SRGB8_ALPHA8_ASTC_10x6 0x93D9
#define COMPRESSED_SRGB8_ALPHA8_ASTC_10x8 0x93DA
#define COMPRESSED_SRGB8_ALPHA8_ASTC_10x10 0x93DB
#define COMPRESSED_SRGB8_ALPHA8_ASTC_12x10 0x93DC
#define COMPRESSED_SRGB8_ALPHA8_ASTC_12x12 0x93DD

#define FMT_RGB 0x1907
#define FMT_RGBA 0x1908

struct KTXHeader
{
    byte identifier[12];
    uint32_t endianness;
    uint32_t type;
    uint32_t type_size;
    uint32_t format;
    uint32_t internal_format;
    uint32_t base_internal_format;
    uint32_t pixel_width;
    uint32_t pixel_height;
    uint32_t pixel_depth;
    uint32_t array_size;
    uint32_t face_count;
    uint32_t level_count;
    uint32_t key_value_data_size;
};

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("Usage:\n");
        printf("\tCubeMapCompress.exe input.json\n");
        return 0;
    }

    std::string input = argv[1];
    std::string input_buffer = File::ReadAllText(input.c_str()).CString();

    auto reader = Ref<Json::CharReader>(Json::CharReaderBuilder().newCharReader());
    Json::Value root;
    const char* begin = input_buffer.c_str();
    const char* end = begin + input_buffer.size();
    if (reader->parse(begin, end, &root, nullptr))
    {
        auto cubemap_levels = root["cubemap_levels"];
        auto format = root["format"];
        auto output = root["output"];

        if (cubemap_levels.isArray() && cubemap_levels.size() > 0 && format.isString() && output.isString())
        {
            int base_width = 0;
            int base_height = 0;
            CMP_FORMAT compress_format = CMP_FORMAT_Unknown;
            uint32_t internal_format = 0;
            uint32_t base_internal_format = 0;
            int block_size_x = 0;
            int block_size_y = 0;
            int block_bit_size = 0;
            Vector<Vector<ByteBuffer>> compressed_levels(cubemap_levels.size());

            if (format.asString() == "BC1_RGBA")
            {
                compress_format = CMP_FORMAT_BC1;
                internal_format = COMPRESSED_RGBA_S3TC_DXT1;
                base_internal_format = FMT_RGBA;
                block_size_x = 4;
                block_size_y = 4;
                block_bit_size = 64;
            }
            else
            {
                return 0;
            }

            for (Json::ArrayIndex i = 0; i < cubemap_levels.size(); ++i)
            {
                Json::Value faces = cubemap_levels[i];
                if (faces.size() != 6)
                {
                    return 0;
                }

                compressed_levels[i].Resize(faces.size());
                for (Json::ArrayIndex j = 0; j < faces.size(); ++j)
                {
                    Ref<Image> image = Image::LoadFromFile(faces[j].asCString());
                    if (!image)
                    {
                        return 0;
                    }

                    // RGBA will be treated to BGRA ?
                    // convert to BGRA first.
                    assert(image->format == ImageFormat::R8G8B8A8);
                    for (int y = 0; y < image->height; ++y)
                    {
                        for (int x = 0; x < image->width; ++x)
                        {
                            byte& r = image->data[y * image->width * 4 + x * 4 + 0];
                            byte& b = image->data[y * image->width * 4 + x * 4 + 2];
                            byte t = r;
                            r = b;
                            b = t;
                        }
                    }

                    if (i == 0)
                    {
                        base_width = image->width;
                        base_height = image->height;
                    }

                    CMP_Texture src;
                    Memory::Zero(&src, sizeof(src));
                    src.dwSize = sizeof(src);
                    src.dwWidth = image->width;
                    src.dwHeight = image->height;
                    src.dwPitch = 0;
                    src.format = CMP_FORMAT_RGBA_8888;
                    src.dwDataSize = image->data.Size();
                    src.pData = image->data.Bytes();

                    CMP_Texture dst;
                    Memory::Zero(&dst, sizeof(dst));
                    dst.dwSize = sizeof(dst);
                    dst.dwWidth = src.dwWidth;
                    dst.dwHeight = src.dwHeight;
                    dst.dwPitch = 0;
                    dst.format = compress_format;
                    ByteBuffer compressed(CMP_CalculateBufferSize(&dst));
                    dst.dwDataSize = compressed.Size();
                    dst.pData = compressed.Bytes();

                    CMP_CompressOptions options;
                    Memory::Zero(&options, sizeof(options));
                    options.dwSize = sizeof(options);
                    options.fquality = 0.05f;
                    options.dwnumThreads = 8;

                    CMP_ERROR cmp_status = CMP_ConvertTexture(&src, &dst, &options, nullptr, 0, 0);
                    if (cmp_status != CMP_OK)
                    {
                        return 0;
                    }

                    compressed_levels[i][j] = compressed;
                }
            }

            // write to ktx
            const int identifier_size = 12;
            byte IDENTIFIER[identifier_size] = {
                0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
            };
            const uint32_t ENDIAN = 0x04030201;

            KTXHeader header;
            Memory::Copy(header.identifier, IDENTIFIER, identifier_size);
            header.endianness = ENDIAN;
            header.type = 0;
            header.type_size = 1;
            header.format = 0;
            header.internal_format = internal_format;
            header.base_internal_format = base_internal_format;
            header.pixel_width = base_width;
            header.pixel_height = base_height;
            header.pixel_depth = 1;
            header.array_size = 1;
            header.face_count = 6;
            header.level_count = compressed_levels.Size();
            header.key_value_data_size = 0;

            int buffer_size = sizeof(header);
            for (uint32_t i = 0; i < header.level_count; ++i)
            {
                buffer_size += sizeof(uint32_t);

                uint32_t level_width = Mathf::Max(header.pixel_width >> i, 1u);
                uint32_t level_height = Mathf::Max(header.pixel_height >> i, 1u);
                int block_count_x = Mathf::Max(level_width / block_size_x, 1u);
                int block_count_y = Mathf::Max(level_height / block_size_y, 1u);
                int face_buffer_size = block_bit_size * block_count_x * block_count_y * header.pixel_depth / 8;

                for (uint32_t j = 0; j < header.array_size; ++j)
                {
                    for (uint32_t k = 0; k < header.face_count; ++k)
                    {
                        buffer_size += face_buffer_size;

                        int cube_padding = 3 - ((face_buffer_size + 3) % 4);
                        buffer_size += cube_padding;
                    }
                }
            }

            ByteBuffer out_buffer(buffer_size);
            MemoryStream ms(out_buffer);

            ms.Write(&header, sizeof(header));
            for (uint32_t i = 0; i < header.level_count; ++i)
            {
                uint32_t level_width = Mathf::Max(header.pixel_width >> i, 1u);
                uint32_t level_height = Mathf::Max(header.pixel_height >> i, 1u);
                int block_count_x = Mathf::Max(level_width / block_size_x, 1u);
                int block_count_y = Mathf::Max(level_height / block_size_y, 1u);
                int face_buffer_size = block_bit_size * block_count_x * block_count_y * header.pixel_depth / 8;

                uint32_t image_size = face_buffer_size * header.array_size * header.face_count;
                ms.Write(image_size);

                for (uint32_t j = 0; j < header.array_size; ++j)
                {
                    for (uint32_t k = 0; k < header.face_count; ++k)
                    {
                        const ByteBuffer& compressed = compressed_levels[i][k];
                        assert(compressed.Size() == face_buffer_size);
                        ms.Write(compressed.Bytes(), compressed.Size());

                        int cube_padding = 3 - ((face_buffer_size + 3) % 4);
                        if (cube_padding > 0)
                        {
                            uint32_t padding = 0;
                            ms.Write(&padding, cube_padding);
                        }
                    }
                }
            }

            File::WriteAllBytes(output.asCString(), out_buffer);
        }
    }

    return 0;
}
