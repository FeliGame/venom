#ifndef LEVEL_GENERATOR_H
#define LEVEL_GENERATOR_H

#include <chunk.h>

#define GENERATE_VEIN false
#define GENERATE_CAVE true
#define GENERATE_SKYBLOCK false
#define GENERATE_BUILDING false


static Chunk *generate_chunk(int cx, int cy, int cz, bool constructing);

// 石头地基生成
static inline int terrain_base_height(int x, int z)
{
    int base = 35;
    int amp = 35;    // 越大地形起伏越大
    float f = 0.015; // 越大地形变化越快
    float noise = perlinNoise(glm::vec2(x * f, z * f));
    // 下面每个噪声参数都要乘权重，加起来要等于1
    float n1 = spline::continent(noise) * 0.5;
    float n2 = spline::peak_valley(noise) * 0.1;
    float n3 = spline::erosion(noise) * 0.4;
    return base + amp * (n1 + n2 + n3);
}

// 土地生成
static int __terrain_heights[CHUNK_LEN * CHUNK_MAX_XZ][CHUNK_LEN * CHUNK_MAX_XZ]; // 指定x/z轴生成的y轴（地表）高度
static inline void init_terrain_heights()
{
    memset(__terrain_heights, 0, sizeof(__terrain_heights));
}

static void generate_terrain_height(glm::vec2 xz)
{
    if (__terrain_heights[(int)xz.x][(int)xz.y] > 0)
        return; // 已经在之前生成过了，就不浪费时间了
                //    int base = 40;
                //    int amp = 40;   // 越大地形起伏越大
                //    float f = 0.015; // 越大地形变化越快
                //    float noise = perlinNoise(glm::vec2(x * f, z * f));
                //    // 下面每个噪声参数都要乘权重，加起来要等于1
                //    float n1 = spline::continent(noise) * 0.5;
                //    float n2 = spline::peak_valley(noise) * 0.3;
                //    float n3 = spline::erosion(noise) * 0.2;
                //    return base + amp * (n1 + n2 + n3);

    // Noise generation
    float n1 = 0, n2 = 0, n3 = 0, n4 = 0;
    float ran = 0, ran_1 = 0, ran_2 = 0, ran_3 = 0;
    // Flat terrain，平原地形是若干噪声函数的叠加
    float a = 0, b = 1, c = 1, d = 0.1;
    n1 = perlinNoise((xz + vec2(1000)) / 60.f); // 低频柏林
    n1 = 1 - abs(n1);
    n2 = 0.5 * (perlinNoise((xz + vec2(100)) / 150.f) + 1.f); // 高频柏林
    n3 = fbm((xz) / 250.f);
    n4 = worleyNoise(xz / 100.f); // 细胞噪声
    ran_1 = n1 * a + n2 * b + n3 * c + n4 * d;
    ran_1 = ran_1 / (a + b + c + d); // 齐次化
    ran_1 = 40 * pow(ran_1, 2.5);    // 放大化

    // Mountainous terrain，山脉地形更为崎岖陡峭，引入傅立叶变换和更多的fbm
    a = 4, b = 3, c = 10, d = 0;

    n1 = 0;
    float amp = 0.5;
    float freq = 64;
    for (int j = 0; j < 4; ++j) // 傅立叶变换叠加
    {
        float h1 = perlinNoise(xz / freq);
        h1 = 1 - abs(h1);

        n1 += h1 * amp;

        freq *= 0.5;
        amp *= 0.5;
    }

    n2 = 0.5 * (perlinNoise((xz + vec2(100)) / 60.f) + 1.f); // 噪声偏移
    n3 = fbm((xz) / 200.f);
    n4 = fbm((xz + vec2(50)) / 400.f);

    ran_2 = n1 * a + n2 * b + n3 * c + n4 * d;
    ran_2 = ran_2 / (a + b + c + d);
    ran_2 = (140 * pow(ran_2, 1) + 100 * (worleyNoise(vec2(xz / 200.f)) + fbm(xz / 500.f))) / 2;

    // Mountain vs plain mixing 混合权重生成
    a = 1, b = 0, c = 1, d = 1;

    n1 = worleyNoise((xz + vec2(10)) / 200.0f);
    n2 = perlinNoise((xz + vec2(1000)) / 300.0f);
    n2 = 1 - abs(n2);
    n3 = fbm((xz) / 400.f);
    n4 = fbm((xz) / 200.f);

    ran_3 = n1 * a + n2 * b + n3 * c + n4 * d;
    ran_3 = ran_3 / (a + b + c + d);

    float exp = worleyNoise((xz + vec2(3000)) / 400.f);
    ran_3 = 1 - pow(ran_3, 4 * exp);

    // Terrain combination 依据上面的权重整合地形
    ran = (1 - ran_3) * ran_2 + ran_3 * ran_1;
    __terrain_heights[(int)xz.x][(int)xz.y] = floor(ran); // 可以加上地基高度
}

// 生成洞穴
static BLOCK_ENUM generate_cave_block(int x, int y, int z)
{
    // 洞，hole_f和下面的f都是频率
    float hole_f = 0.15f, thres = -0.2f;
    bool isCave = perlinNoise3D(glm::vec3(x * hole_f, y * hole_f, z * hole_f)) < thres;

    // 连通洞穴之间的“管道”
    float f = 0.2f, pipe_max = -0.25f, pipe_min = -0.2f; // f越大管道越扭曲同时也越窄小，pipe_max和pipe_min决定管径
    float pipe_noise = perlinNoise3D(glm::vec3(x * f, y * f, z * f));
    bool isPipe = (pipe_noise < pipe_max) && (pipe_noise > pipe_min);
    if (isCave || isPipe)
    {
        return BLOCK_AIR;
    }
    return BLOCK_NULL;
}

// 矿脉生成
static inline BLOCK_ENUM generate_vein_block(int x, int y, int z)
{
    float f = 0.2f, thres = -0.45f; // thres是生成阈值
    float n1 = perlinNoise3D(glm::vec3(x * f, y * f, z * f));
    bool isOre = n1 < thres;
    if (isOre)
    {
        return BLOCK_IRON_ORE;
    }
    return BLOCK_NULL;
}

// 生成空岛
static inline BLOCK_ENUM generate_skyblock(int x, int y, int z)
{
    float f = 0.05, thres = 0.35f; // f越大生成数量越多但体积也减小
    if (perlinNoise3D(glm::vec3(x * f, y * f, z * f)) > thres)
    {
        return BLOCK_DIRT;
    }
    return BLOCK_NULL;
}

// 首次启动时导入模型数据
std::unordered_map<std::string, std::vector<tinyobj::shape_t>> __model_shapes; // mesh.indices存储面的网格数据（顶点/法线/纹理索引）
std::unordered_map<std::string, tinyobj::attrib_t> __model_attributes;         // 存储顶点信息、法线信息、纹理映射信息等
std::unordered_map<std::string, std::vector<BLOCK_ENUM>> __model_block_kinds;  // png图片存储的方块种类信息

// 加载.obj模型文件和材质png
static void import_model_resources()
{
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    std::string model_name = "monu1"; // 【应当从配置文件读取】
    if (!tinyobj::LoadObj(&__model_attributes[model_name], &__model_shapes[model_name], &materials, &warn, &err, (std::string(MODEL_DIR) + model_name + ".obj").c_str(), MODEL_DIR))
    {
        throw std::runtime_error(err);
    }
    std::cout << "Loaded obj warning: " << warn;
    printf(" and vertices cnt: %lu\n", __model_attributes[model_name].vertices.size());

    // 从专用模型纹理png读取颜色值（分辨率必须是256*1），然后转换为Block Kind
    std::string png_file = std::string(MODEL_DIR) + model_name + ".png";
    int texWidth, texHeight, texChannels;
    auto pixels = stbi_load(png_file.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels)
    {
        std::cout << "Failed to load model png " << png_file << std::endl;
        return;
    }
    if (texWidth != 256 || texHeight != 1)
    { // 检查图片分辨率是否符合要求
        printf("Model file %s 's size should be 256 * 1, but it's %d * %d\n", png_file.c_str(), texWidth, texHeight);
        return;
    }

    if (!RANDOM_BUILDING_TEXTURE)
        init_fixed_argb2block();                                         // 建立固定颜色->Block种类映射，如果使用随机映射则可以省略该行
    int uv_count = __model_attributes[model_name].texcoords.size() >> 1; // 读取模型uv对的数量
    __model_block_kinds[model_name].resize(uv_count);
    for (int i = 0; i < uv_count; ++i)
    {
        int pixelIndex = int(256 * __model_attributes[model_name].texcoords[i << 1]) << 2; // texcoords奇数时固定为0.5；模型png为1像素高，256像素宽，四通道
        unsigned char r = pixels[pixelIndex + 0];
        unsigned char g = pixels[pixelIndex + 1];
        unsigned char b = pixels[pixelIndex + 2];
        unsigned char a = pixels[pixelIndex + 3];
        printf("uv index %d 's diffuse color is: %d %d %d %d\n", i, r, g, b, a);
        unsigned argb = (a << 24) | (r << 16) | (g << 8) | b;
        __model_block_kinds[model_name][i] = get_argb2block(argb, RANDOM_BUILDING_TEXTURE);
    }
}

// 生成平行于x或z轴的道路
static void generate_straight_road(glm::ivec2 xz_start, glm::ivec2 xz_end, int half_width)
{
    if (xz_start.x == xz_end.x)
    { // z方向
        int z_min, z_max, road_x = xz_start.x;
        if (xz_start.y < xz_end.y)
        {
            z_min = xz_start.y;
            z_max = xz_end.y;
        }
        else
        {
            z_min = xz_end.y;
            z_max = xz_start.y;
        }
        for (int x = road_x - half_width; x <= road_x + half_width; ++x)
        {
            for (int z = z_min; z <= z_max; ++z)
            {
                auto pos = glm::ivec3(x, __terrain_heights[road_x][z], z); // 路宽方向上的高度是一样的
                int cx = pos.x / CHUNK_LEN;
                int cy = pos.y / CHUNK_LEN;
                int cz = pos.z / CHUNK_LEN;
                Chunk *chunk = generate_chunk(cx, cy, cz, true);
                if (!chunk)
                {
                    std::cerr << "Out of border!\n";
                }
                // 再填充方块
                create_block(pos, BLOCK_COBBLE_STONE, true, chunk);
            }
        }
    }
    if (xz_start.y == xz_end.y)
    { // x方向
        int x_min, x_max, road_z = xz_start.y;
        if (xz_start.x < xz_end.x)
        {
            x_min = xz_start.x;
            x_max = xz_end.x;
        }
        else
        {
            x_min = xz_end.x;
            x_max = xz_start.x;
        }
        for (int z = road_z - half_width; z <= road_z + half_width; ++z)
        {
            for (int x = x_min; x <= x_max; ++x)
            {
                auto pos = glm::ivec3(x, __terrain_heights[x][road_z], z); // 路宽方向上的高度是一样的
                int cx = pos.x / CHUNK_LEN;
                int cy = pos.y / CHUNK_LEN;
                int cz = pos.z / CHUNK_LEN;
                Chunk *chunk = generate_chunk(cx, cy, cz, true);
                if (!chunk)
                {
                    std::cerr << "Out of border!\n";
                }
                // 再填充方块
                create_block(pos, BLOCK_COBBLE_STONE, true, chunk);
            }
        }
    }
    printf("Generating Road Error: Expected a straight road!\n");
    return;
}

// 生成建筑物，chance为该位置建筑的随机生成概率，值域[0，1)
static void generate_building(int x, int y, int z, const char *model_name, float chance)
{
    if (std::rand() % 100 > 100 * chance)
        return; // 按概率抽奖

    glm::vec3 base_pos = glm::vec3(x, y, z);
    // 接收模型的自身坐标系后，将其转换为Blocks
    for (const auto &shape : __model_shapes[model_name])
    {
        // 一个shape即整个模型
        glm::vec3 tri_vertices[3];
        int i = 0; // 遍历到这个面的第几个顶点了？
        for (const auto &index : shape.mesh.indices)
        {
            // 通过一个顶点的索引定位出坐标，
            glm::vec3 pos = MODEL_MAGNIFICATION * glm::vec3(
                                                      __model_attributes[model_name].vertices[3 * index.vertex_index + 0],
                                                      __model_attributes[model_name].vertices[3 * index.vertex_index + 1],
                                                      __model_attributes[model_name].vertices[3 * index.vertex_index + 2]);

            tri_vertices[i] = pos;
            if (i == 2)
            { // 收集完三个顶点，就绘制一次面
                // 执行三角形遍历算法，三角形内任意一点可以表示成：P(x,y,z)=aOA+bOB+cOC，O是三角形重心，且 a+b+c=1，控制好迭代i,j,k的步长即可。
                auto centroid = (tri_vertices[0] + tri_vertices[1] + tri_vertices[2]) / 3.0f; // 重心坐标
                auto oa = tri_vertices[0] - centroid;
                auto ob = tri_vertices[1] - centroid;
                auto oc = tri_vertices[2] - centroid;

                auto ab = tri_vertices[1] - tri_vertices[0];
                auto ac = tri_vertices[2] - tri_vertices[0];
                auto bc = tri_vertices[2] - tri_vertices[1];
                auto step = 1 / std::max(std::max(glm::length(ab), glm::length(ac)), glm::length(bc)); // 由于系数a的范围为[0, 1]，要能够遍及每个方块，因此要步长应为：1/线度
                for (float a = 0; a <= 1.0f; a += step)
                {
                    for (float b = 0; b <= 1.0f - a; b += step)
                    {
                        auto w_pos = base_pos + centroid + a * oa + b * ob + (1.0f - a - b) * oc; // 世界位置，不要忘了+centroid！
                        // 检查区块是否生成，并标记区块
                        int cx = w_pos.x / CHUNK_LEN;
                        int cy = w_pos.y / CHUNK_LEN;
                        int cz = w_pos.z / CHUNK_LEN;
                        Chunk *chunk = generate_chunk(cx, cy, cz, true); // 先生成自然环境，再告知建筑用地
                        if (!chunk)
                        { // 区块越界无法被加载
                            std::cerr << "Out of border!\n";
                        }
                        // 再填充方块
                        create_block(glm::ivec3(w_pos), __model_block_kinds[model_name][index.texcoord_index], false, chunk); // 不替换方块，允许自然地形和其他建筑侵入
                    }
                }
            }
            i = (i + 1) % 3;
        }
    }
}

// 生成小镇，xyz小镇中心位置，max_r小镇最大半径，gap 最小间距
static void generate_town(int x, int z, int max_r, int gap)
{
    // 噪声阈值判断能否建城
    float density = 0.45f;
    float noise = perlinNoise(glm::vec2(x, z) * density + vec2(1000));
    if (noise < 0.25f)
        return; // 限定生成噪声阈值，以控制小镇稀有度
    // 生成建筑
    int max_d = max_r << 1;
    for (int i = -max_r; i <= max_r; i += gap)
    {
        for (int j = -max_r; j <= max_r; j += gap)
        {
            generate_building(x + i, __terrain_heights[x + i][z + j], z + j, "monu1", std::cos(M_PI * (abs(i) + abs(j)) / max_d)); // 生成概率和到中心距离成正比
        }
    }
    // 生成z方向道路
    for (int i = -max_r; i < max_r; i += gap)
    {
        int road_x = x + i + (gap >> 1); // 道路应当在建筑夹缝间
        generate_straight_road(glm::ivec2(road_x, z - max_r), glm::ivec2(road_x, z + max_r), 1);
    }
    // 生成x方向道路
    for (int i = -max_r; i < max_r; i += gap)
    {
        int road_z = z + i + (gap >> 1); // 道路应当在建筑夹缝间
        generate_straight_road(glm::ivec2(x - max_r, road_z), glm::ivec2(x + max_r, road_z), 1);
    }
}

// 首次创建区块并填充方块
static Chunk *generate_chunk(int cx, int cy, int cz, bool constructing)
{
    if (cx < 0 || cy < 0 || cz < 0)
        return nullptr;
    Chunk *chunk = get_chunk(cx, cy, cz);
    if (chunk)
    {
        if (constructing)
        {                        // 自然用地划为建筑用地
            chunk->built = true; // 配合建完
        }
        return chunk; // 如果已经有区块，则返回
    }

    // 创建区块，新建的地事先声明用作建筑用地
    chunk = set_chunk(cx, cy, cz, new Chunk(cx, cy, cz, constructing));

    int x_min = cx * CHUNK_LEN;
    int y_min = cy * CHUNK_LEN;
    int z_min = cz * CHUNK_LEN;
    int x_max = (cx + 1) * CHUNK_LEN;
    int y_max = (cy + 1) * CHUNK_LEN;
    int z_max = (cz + 1) * CHUNK_LEN;

    // 石质地基
    for (int x = x_min; x < x_max; ++x)
    {
        for (int z = z_min; z < z_max; ++z)
        {
            int ym = std::min(y_max, terrain_base_height(x, z));
            for (int y = cy * CHUNK_LEN; y < ym; ++y)
            {
                create_block(glm::ivec3(x, y, z), BLOCK_STONE, false, chunk);
            }
        }
    }

    // 泥土地基
    for (int x = x_min; x < x_max; ++x)
    {
        for (int z = z_min; z < z_max; ++z)
        {
            generate_terrain_height(glm::vec2(x, z));
            int th = __terrain_heights[x][z];
            int ym = std::min(y_max, th);
            for (int y = cy * CHUNK_LEN; y < ym; ++y)
            {
                if (y >= th - 1)
                { // 覆草【应当在挖空的逻辑之后做！】
                    create_block(glm::ivec3(x, y, z), BLOCK_GRASS, true, chunk);
                    break;
                }
                create_block(glm::ivec3(x, y, z), BLOCK_DIRT, false, chunk);
            }
        }
    }

    // 从下到上依次覆盖生成，注意最低高度至少为1（0为基岩）
    for (int x = x_min; x < x_max; ++x)
    {
        for (int z = z_min; z < z_max; ++z)
        {
            if (GENERATE_VEIN)
            {
                // 矿脉生成
                int vein_y_min = std::max(y_min, 1), vein_y_max = std::min(y_max, 20);
                for (int y = vein_y_min; y < vein_y_max; ++y)
                {
                    create_block(glm::ivec3(x, y, z),
                                 generate_vein_block(x, y, z), true, chunk);
                }
            }
            if (GENERATE_CAVE)
            {
                // 洞穴生成
                int cave_y_min = std::max(y_min, 1), cave_y_max = std::min(y_max, 50);
                for (int y = cave_y_min; y < cave_y_max; ++y)
                {
                    create_block(glm::ivec3(x, y, z),
                                 generate_cave_block(x, y, z), true, chunk);
                }
            }
            if (GENERATE_SKYBLOCK)
            {
                // 空岛生成
                int skyblock_y_min = std::max(y_min, 80), skyblock_y_max = std::min(y_max, 128);
                for (int y = skyblock_y_min; y < skyblock_y_max; ++y)
                {
                    create_block(glm::ivec3(x, y, z),
                                 generate_skyblock(x, y, z), true, chunk);
                }
            }
        }
    }

    // 该位置没有建筑冲突，就尝试生成模型导入的建筑，每个区块至多一个建筑
    if (GENERATE_BUILDING && !chunk->built)
    {
        generate_town(x_min, z_min, 50, 30);
    }

    // 基岩铺底
    if (cy == 0)
    {
        for (int x = x_min; x < x_max; ++x)
            for (int z = z_min; z < z_max; ++z)
                create_block(glm::ivec3(x, 0, z), BLOCK_BEDROCK, true, chunk);
    }

    // 开辟该区块的剩余所有方块
    for (int i = 0; i < CHUNK_LEN; ++i)
    {
        for (int j = 0; j < CHUNK_LEN; ++j)
        {
            for (int k = 0; k < CHUNK_LEN; ++k)
            {
                if (!chunk->blocks[i][j][k])
                {
                    chunk->blocks[i][j][k] = new Block(x_min + i, y_min + j, z_min + k, BLOCK_AIR);
                }
            }
        }
    }

    return chunk;
}

#endif /* LEVEL_GENERATOR_H */