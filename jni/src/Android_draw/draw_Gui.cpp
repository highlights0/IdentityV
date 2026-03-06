#include "draw.h"
#include <thread>
#include <cstdint>
#include <stdio.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <math.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <cmath> 
#include <linux/input.h>

// ================== 图片加载支持 ==================
#include <GLES2/gl2.h>
#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"
// ================================================

// 项目头文件
#include "My_font/zh_Font.h"
#include "My_font/fontawesome-brands.h"
#include "My_font/fontawesome-regular.h"
#include "My_font/fontawesome-solid.h"
#include "My_font/gui_icon.h"
#include "kerneldriver-qxqd.hpp"
#include "DrawTool.h"
#include "Name.h"
#include "Offsets.h"

// ============== 结构体定义 ==============
struct Vector3A {
    float X, Y, Z;
    Vector3A(float x = 0, float y = 0, float z = 0) : X(x), Y(y), Z(z) {}
};

struct DataStruct {
    long int obj = 0;
    char str[256] = { 0 };
    int 阵营 = 0; // 1:监管, 2:求生, 3:场景, 4:道具
    char 类名[256] = { 0 };
    long int objcoor = 0;
};

struct GameDataBuffer {
    DataStruct data[2000];
    int 数量 = 0;
};

struct MapConfig {
    float minX, maxX, minY, maxY;
    bool isVerticalMap;
    const char* name;
    int gridX, gridY;
};

// ============== 前置声明 ==============
void Draw_UnifiedMap(ImDrawList* Draw, const MapConfig& cfg);
void Draw_Main(ImDrawList* Draw);
void DrawCenteredText(ImDrawList* Draw, const ImVec2& pos, const ImColor& color, const char* text);
bool GetEntityPosition(const DataStruct& obj, Vector3A& pos);
bool ShouldSkipEntity(const DataStruct& obj, const Vector3A& pos);
void LoadMapTexture(int index);

// ============== 常量与全局配置 ==============
const ImColor COL_RED(250, 0, 0, 255);
const ImColor COL_GREEN(0, 255, 0, 255);
const ImColor COL_BLUE(0, 0, 255, 255);
const ImColor COL_YELLOW(255, 255, 0, 255);
const ImColor COL_PURPLE(255, 0, 255, 255);
const ImColor COL_BLACK(0, 0, 0, 255);
const ImColor COL_LIGHT_BLUE(0, 191, 255, 255);
const ImColor COL_WHITE(255, 255, 255, 255);
const ImColor COL_ORANGE(255, 165, 0, 255);

// ================== 强化后的过滤逻辑 ==================
static const std::vector<std::string> g_filter_keywords = {
    "creature", "girl_page", "skill_hudie", "joseph_camera",
    "burke_console", "heijin_yizi", "qiutu_box", "weapon",
    "nvyao", "detective", "dress_ghost", "part", "effect",
    "sound", "_lod", "_shadow", "_ui_", "_indicator",
    "collision", "mesh", "_ttds_", "_em_", "h55_survivor_m_ttds_hu",
    "phantom", "pet", "summon", "decoy", "trap",
    "crow", "em_crow", "butcher_em", "zbs"
};

inline bool IsFakeHunter(const char* name) {
    const char* fakes[] = {
        "mirror", "crow", "patroller", "peeper", "tentacle",
        "note", "robot", "phantom", "clone", "decoy", "trap", "pet",
        "em_crow", "em65", "butcher_em",
        // 新增以下关键词：
        "umbrella", "parasol", "buzz", "bird"
    };
    for (const char* f : fakes) {
        if (strstr(name, f)) return true;
    }
    return false;
}

GameDataBuffer g_buffer[3];
std::atomic<int> g_read_index{ 0 };
std::atomic<int> g_write_index{ 1 };

bool permeate_record = false;
bool permeate_record_ini = false;
Last_ImRect LastCoordinate = { 0, 0, 0, 0 };
static uint32_t orientation = -1;
ANativeWindow* window = nullptr;
ImGuiWindow* g_window = nullptr;
std::unique_ptr<AndroidImgui> graphics;

android::ANativeWindowCreator::DisplayInfo displayInfo;
int abs_ScreenX, abs_ScreenY;
int native_window_screen_x, native_window_screen_y;
ImFont* zh_font = nullptr;

uintptr_t libbase = 0;
uintptr_t Matrix = 0, Arrayaddr = 0;
uintptr_t 自身 = 0;

float px = 0.0f;
float py = 0.0f;
long int MatrixOffset = 0;
long int ArrayaddrOffset = 0;
float 距离比例 = 11.886f;
float matrix[16];
Vector3A Z;
int 自身阵营 = 0;
char 自身名字[64] = "None";

// ================== 默认状态 ==================
static bool show_draw_line = false;
static bool show_draw_Prop = false;
static bool show_draw_prophet = true;
static bool show_draw_secret_mechine = false;
static bool enable_arms_factory = false;
static bool Debugging = false;
static bool inform_ghost = true;
static bool show_window = true;
static bool voice = true;

static float map_display_size = 380.0f;
static float map_pos_x = 80.0f;
static float map_pos_y = 20.0f;
static int g_manual_map_index = 0;
GLuint g_map_texture_id = 0;
int g_loaded_map_index = -1;
char g_map_status_text[256] = "等待加载...";

static const MapConfig Maps[] = {
    { 743.045f, 2300.048f, 610.952f, 2392.805f, true, "军工厂", 3, 3 },
    { -675.63f, 807.0f, -730.0f, 743.0f, false, "红教堂", 3, 3 },
    { -675.0f, 782.0f, -856.0f, 709.0f, true, "圣心医院", 3, 3 },
    { -910.0f, 980.0f, -856.0f, 958.0f, false, "湖景村", 4, 3 },
    { -1058.0f, 1052.0f, -610.0f, 618.0f, false, "月亮河公园", 4, 3 },
    { 765.0f, 2317.0f, 600.0f, 2391.0f, true, "里奥的回忆", 3, 3 },
    { -46.0f, 1458.0f, -1187.0f, 605.0f, true, "永眠镇", 4, 3 },
    { -785.0f, 805.0f, -723.0f, 709.0f, false, "唐人街", 3, 3 },
    { -753.0f, 750.0f, -700.0f, 730.0f, false, "不归林", 3, 3 }
};

char 监管者预知[1024] = { 0 };
char extractedString[64] = { 0 };
int 状态 = 0;
int 遍历次数 = 0;

size_t get_memory_usage_kb() {
    FILE* file = fopen("/proc/self/statm", "r");
    if (!file) return 0;
    size_t size = 0, resident = 0;
    fscanf(file, "%zu %zu", &size, &resident);
    fclose(file);
    return resident * 4;
}

inline bool should_filter(const std::string& name) {
    for (const auto& keyword : g_filter_keywords) {
        if (name.find(keyword) != std::string::npos) return true;
    }
    return false;
}

void screen_config() {
    ::displayInfo = android::ANativeWindowCreator::GetDisplayInfo();
}

long get_module_base(int pid, const char* module_name) {
    char filename[64], line[1024];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    FILE* fp = fopen(filename, "r");
    long addr = 0;
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, module_name)) {
                sscanf(line, "%lx-%*lx", &addr);
                break;
            }
        }
        fclose(fp);
    }
    return addr;
}

struct ModuleBssInfo { unsigned long addr; unsigned long taddr; };
ModuleBssInfo get_module_bss(int pid, const char* module_name) {
    char filename[64], line[1024];
    snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    ModuleBssInfo info = { 0, 0 };
    FILE* fp = fopen(filename, "r");
    if (!fp) return info;
    bool found_module = false;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, module_name)) found_module = true;
        if (found_module && strstr(line, "rw") && strlen(line) < 86) {
            long addr, taddr;
            if (sscanf(line, "%lx-%lx", &addr, &taddr) == 2) {
                if ((taddr - addr) / 4096 >= 2800) {
                    info.addr = addr; info.taddr = taddr;
                    break;
                }
            }
        }
    }
    fclose(fp);
    return info;
}

int get_name_pid1(const char* packageName) {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;
    struct dirent* entry;
    char filename[64], cmdline[64];
    int id = -1;
    while ((entry = readdir(dir)) != NULL) {
        id = atoi(entry->d_name);
        if (id == 0) continue;
        snprintf(filename, sizeof(filename), "/proc/%d/cmdline", id);
        FILE* fp = fopen(filename, "r");
        if (fp) {
            if (fgets(cmdline, sizeof(cmdline), fp)) {
                if ((strstr(cmdline, packageName) != NULL || strstr(cmdline, "com.netease.idv") != NULL) &&
                    strstr(cmdline, "com") != NULL && !strstr(cmdline, "PushService") && !strstr(cmdline, "gcsdk")) {
                    snprintf(extractedString, sizeof(extractedString), "%s", cmdline);
                    fclose(fp);
                    closedir(dir);
                    return id;
                }
            }
            fclose(fp);
        }
    }
    closedir(dir);
    return -1;
}

bool GetEntityPosition(const DataStruct& obj, Vector3A& pos) {
    pos.X = getFloat(obj.objcoor + 0xA0);
    pos.Y = getFloat(obj.objcoor + 0xA8);
    pos.Z = getFloat(obj.objcoor + 0xA4);
    if (fabs(pos.X) < 1e-4 && fabs(pos.Y) < 1e-4 && fabs(pos.Z) < 1e-4) return false;
    return true;
}

bool ShouldSkipEntity(const DataStruct& obj, const Vector3A& pos) {
    if (pos.Z <= -300.0f) return true;
    if (should_filter(obj.类名)) return true;
    int checkVal = getDword(obj.obj + Offsets::OBJ_CHECK_OFFSET);       // 0x70
    float checkFloat = getFloat(obj.obj + Offsets::OBJ_CHECK_OFFSET_2); // 0x1A0

    // 2. 定义什么是幽灵：如果不等于有效值(0x1000000) 或 浮点数不对(450.0)，则视为幽灵/无效
    // 引用源码：if ( (_DWORD)v130 != 0x1000000 ) goto LABEL_67; [cite: 144]
    // 引用源码：if ( *(float *)&v130 != 450.0 ) goto LABEL_67; [cite: 148]
    bool is_ghost_obj = (checkVal != Offsets::VALID_ACTOR_VALUE || checkFloat != Offsets::VALID_ACTOR_FLOAT);

    // 3. 替换原本的 if (jxpd == Offsets::GHOST_VALUE)
    if (is_ghost_obj) {
        // 逻辑 A: 如果没开启“显示幽灵”功能，直接返回 true (过滤掉)
        if (!inform_ghost) return true;

        // 逻辑 B: 即使开启了显示幽灵，如果是以下特定的监管者，依然返回 true (过滤掉)
        // (保留了你原版的特殊过滤列表)
        if (strstr(obj.str, "红蝶") || strstr(obj.str, "无常") ||
            strstr(obj.str, "歌剧") || strstr(obj.str, "破轮") || strstr(obj.str, "木偶") || strstr(obj.str, "冒险家")) {
            return true;
        }
    }
    return false;
}

bool M_Android_LoadFont(float SizePixels) {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.MergeMode = false;
    ::zh_font = io.Fonts->AddFontFromMemoryTTF((void*)OPPOSans_H, OPPOSans_H_size, SizePixels, &config, io.Fonts->GetGlyphRangesChineseFull());
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromMemoryCompressedTTF((const void*)&font_awesome_brands_compressed_data, sizeof(font_awesome_brands_compressed_data), SizePixels, &icons_config, icons_ranges);
    io.Fonts->AddFontFromMemoryCompressedTTF((const void*)&font_awesome_regular_compressed_data, sizeof(font_awesome_regular_compressed_data), SizePixels, &icons_config, icons_ranges);
    io.Fonts->AddFontFromMemoryCompressedTTF((const void*)&font_awesome_solid_compressed_data, sizeof(font_awesome_solid_compressed_data), SizePixels, &icons_config, icons_ranges);
    return zh_font != nullptr;
}

void init_My_drawdata() {
    ImGui::My_Android_LoadSystemFont(25.0f);
    M_Android_LoadFont(25.0f);
}

void ResetMapTextureState() {
    g_map_texture_id = 0;
    g_loaded_map_index = -1;
    snprintf(g_map_status_text, sizeof(g_map_status_text), "上下文已重置");
}

void drawBegin() {
    screen_config();
    if (::orientation != displayInfo.orientation) {
        ::orientation = displayInfo.orientation;
        Touch::setOrientation(displayInfo.orientation);
        if (g_window) { g_window->Pos.x = 100; g_window->Pos.y = 125; }
    }
    if (permeate_record_ini) {
        LastCoordinate = { g_window->Pos.x, g_window->Pos.y, g_window->Size.x, g_window->Size.y };
        graphics->Shutdown();
        android::ANativeWindowCreator::Destroy(window);
        window = android::ANativeWindowCreator::Create("Acid5!", displayInfo.width, displayInfo.height, permeate_record);
        graphics->Init_Render(window, displayInfo.width, displayInfo.height);
        ResetMapTextureState();
        init_My_drawdata();
    }
}

// ================== 核心扫描线程 (恢复原始特征) ==================
void read_thread(long int PD1, long int PD2, long int PD3) {
    pid = -1;
    while (pid == -1) {
        pid = get_name_pid1("dwrg");
        if (pid == -1) sleep(1);
    }
    get_name_pid(extractedString);
    ModuleBssInfo result;
    if (strstr(extractedString, "com.netease.idv") != NULL) {
        libbase = get_module_base(pid, "."); result = get_module_bss(pid, ".");
    }
    else {
        libbase = get_module_base(pid, "libclient.so"); result = get_module_bss(pid, "libclient.so");
    }
    int c = (result.taddr - result.addr) / 4096;
    unsigned long buff[512];
    while (MatrixOffset == 0 || ArrayaddrOffset == 0) {
        状态 = 1;
        for (int i = 0; i < c; i++) {
            vm_readv(result.addr + (i * 4096), &buff, 0x1000);
            for (int ii = 0; ii < 512; ii++) {
                unsigned long val = buff[ii];
                long int CurrentAddr = result.addr + (i * 4096) + (ii * 8);
                if (val == 0x656A624F72655028) {
                    if (getDword(CurrentAddr + 304) == 257) MatrixOffset = CurrentAddr - libbase + 768;
                }
                if (val == 16384) {
                    if (getFloat(CurrentAddr - 16) == 1.0f && getDword(CurrentAddr - 8) == 257)
                        ArrayaddrOffset = CurrentAddr - libbase + 56;
                }
            }
        }
        if (MatrixOffset != 0 && ArrayaddrOffset != 0) { 状态 = 2; break; }
        遍历次数++; sleep(2);
    }
    char temp_name[128]; std::string s_prophet;
    while (true) {
        uintptr_t ArrayStruct = libbase + ArrayaddrOffset;
        uintptr_t StartPtr = getPtr64(ArrayStruct);
        uintptr_t EndPtr = getPtr64(ArrayStruct + 8);
        long count = 0;
        if (StartPtr > 0 && EndPtr > StartPtr) count = (EndPtr - StartPtr) / 8;
        if (count > 2000) count = 2000;
        int read_idx = g_read_index.load(std::memory_order_acquire);
        int last_write = g_write_index.load(std::memory_order_relaxed);
        int free_idx = 0;
        for (int i = 0; i < 3; i++) if (i != read_idx && i != last_write) { free_idx = i; break; }
        GameDataBuffer& buffer = g_buffer[free_idx];
        int count_valid = 0; s_prophet.clear();
        for (int ii = 0; ii < count; ii++) {
            if (count_valid >= 1999) break;
            uintptr_t objPtr = getPtr64(StartPtr + ii * 8);
            if (objPtr == 0) continue;
            uintptr_t component_ptr = getPtr64(objPtr + 0x28);
            if (component_ptr == 0) continue;
            uintptr_t name_struct = getPtr64(getPtr64(getPtr64(getPtr64(getPtr64(objPtr + 0xF8) + 0x0) + 0x8) + 0x20) + 0x20);
            if (name_struct == 0) continue;
            int name_len = getDword(name_struct + 0x10);
            uintptr_t name_val_ptr = getPtr64(name_struct + 0x8);
            if (name_val_ptr != 0 && name_len > 0 && name_len < 128) {
                vm_readv(name_val_ptr, temp_name, name_len); temp_name[name_len] = '\0';
            }
            else continue;
            if (should_filter(temp_name)) continue;
            DataStruct& item = buffer.data[count_valid];
            item.obj = objPtr; item.objcoor = component_ptr;
            strcpy(item.类名, temp_name); item.阵营 = 0; bool added = false;
            if (strstr(temp_name, "boss")) {
                if (IsFakeHunter(temp_name)) continue;
                strcpy(item.str, getboss(temp_name)); item.阵营 = 1; added = true;
                if (s_prophet.find(item.str) == std::string::npos && !strstr(item.str, "butcher")) s_prophet += std::string(item.str) + " ";
            }
            else if (strstr(temp_name, "player")) {
                strcpy(item.str, getplayer(temp_name)); item.阵营 = 2; added = true;
            }
            else if (strstr(temp_name, "scene_sender") && strstr(temp_name, "low")) {
                item.阵营 = 3; strcpy(item.str, "密码机"); added = true;
            }
            else if (strstr(temp_name, "prop_76")) {
                item.阵营 = 3; strcpy(item.str, "地窖"); added = true;
            }
            else if (strstr(temp_name, "prop")) {
                item.阵营 = 4; strcpy(item.str, "道具"); added = true;
            }
            if (added) count_valid++;
        }
        snprintf(监管者预知, sizeof(监管者预知), "%s", s_prophet.c_str());
        buffer.数量 = count_valid;
        g_write_index.store(free_idx, std::memory_order_relaxed);
        g_read_index.store(free_idx, std::memory_order_release);
        usleep(15000);
    }
}

void DrawCenteredText(ImDrawList* Draw, const ImVec2& pos, const ImColor& color, const char* text) {
    if (!text || text[0] == '\0') return;
    ImVec2 textSize = ImGui::CalcTextSize(text);
    Draw->AddText({ pos.x - textSize.x / 2.0f, pos.y }, color, text);
}

void LoadMapTexture(int index) {
    if (index == g_loaded_map_index && g_map_texture_id != 0) return;
    if (g_map_texture_id != 0) { glDeleteTextures(1, &g_map_texture_id); g_map_texture_id = 0; }
    char path[256]; snprintf(path, sizeof(path), "/sdcard/maps/%d.png", index);
    int w, h, n; unsigned char* data = stbi_load(path, &w, &h, &n, 4);
    if (data) {
        glGenTextures(1, &g_map_texture_id);
        if (g_map_texture_id != 0) {
            glBindTexture(GL_TEXTURE_2D, g_map_texture_id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data); g_loaded_map_index = index;
            snprintf(g_map_status_text, sizeof(g_map_status_text), "加载成功");
        }
        else stbi_image_free(data);
    }
    else snprintf(g_map_status_text, sizeof(g_map_status_text), "文件未找到");
}

void Draw_UnifiedMap(ImDrawList* Draw, const MapConfig& cfg) {
    if (!enable_arms_factory) return;
    float worldXSpan = cfg.maxX - cfg.minX, worldYSpan = cfg.maxY - cfg.minY;
    float map_h = map_display_size, map_w = cfg.isVerticalMap ? (map_h * (worldYSpan / worldXSpan)) : (map_h * (worldXSpan / worldYSpan));
    ImVec2 map_pos = { map_pos_x, map_pos_y }, map_end = { map_pos.x + map_w, map_pos.y + map_h };
    LoadMapTexture(g_manual_map_index);
    if (g_map_texture_id != 0) Draw->AddImage((void*)(intptr_t)g_map_texture_id, map_pos, map_end);
    else Draw->AddRectFilled(map_pos, map_end, ImColor(0, 0, 0, 200), 5.0f);
    Draw->AddRect(map_pos, map_end, ImColor(255, 255, 255, 120), 5.0f, 0, 1.2f);
    int read_idx = g_read_index.load(std::memory_order_acquire);
    const GameDataBuffer& buf = g_buffer[read_idx];
    auto GetMapPos = [&](const Vector3A& p) -> ImVec2 {
        float nX = cfg.isVerticalMap ? (cfg.maxY - p.Y) / worldYSpan : (p.X - cfg.minX) / worldXSpan;
        float nY = cfg.isVerticalMap ? (cfg.maxX - p.X) / worldXSpan : (cfg.maxY - p.Y) / worldYSpan;
        return { map_pos.x + nX * map_w, map_pos.y + nY * map_h };
        };
    for (int layer = 1; layer <= 3; layer++) {
        for (int i = 0; i < buf.数量; i++) {
            const DataStruct& obj = buf.data[i]; Vector3A pos;
            if (!GetEntityPosition(obj, pos)) continue;
            if (obj.obj != 自身 && ShouldSkipEntity(obj, pos)) continue;
            ImVec2 p = GetMapPos(pos);
            if (layer == 1 && obj.阵营 == 3 && strstr(obj.类名, "sender")) Draw->AddRect({ p.x - 10, p.y - 10 }, { p.x + 10, p.y + 10 }, COL_YELLOW, 0, 0, 2.0f);
            else if (layer == 2 && obj.阵营 == 3 && strstr(obj.类名, "prop_76")) Draw->AddRect({ p.x - 8, p.y - 8 }, { p.x + 8, p.y + 8 }, COL_PURPLE, 0, 0, 1.5f);
            else if (layer == 3) {
                if (obj.obj == 自身) {
                    float angle = atan2f(matrix[10], matrix[8]) + (cfg.isVerticalMap ? 1.57f : 3.14f);
                    Draw->PathLineTo(p); Draw->PathArcTo(p, 180.0f, angle - 0.7f, angle + 0.7f, 30);
                    Draw->PathFillConvex(ImColor(0, 191, 255, 30));
                    static Vector3A last_pos = { 0, 0, 0 }; static float move_angle = 0.0f;
                    float dist_sq = pow(pos.X - last_pos.X, 2) + pow(pos.Y - last_pos.Y, 2);
                    if (dist_sq > 0.8f) {
                        ImVec2 p_now = GetMapPos(pos), p_last = GetMapPos(last_pos);
                        float mdx = p_now.x - p_last.x, mdy = p_now.y - p_last.y;
                        if (mdx * mdx + mdy * mdy > 0.01f) move_angle = atan2f(mdy, mdx);
                        last_pos = pos;
                    }
                    float arrow_size = 20.0f;
                    ImVec2 p1 = { p.x + arrow_size * cosf(move_angle), p.y + arrow_size * sinf(move_angle) };
                    ImVec2 p2 = { p.x + arrow_size * 0.7f * cosf(move_angle + 2.5f), p.y + arrow_size * 0.7f * sinf(move_angle + 2.5f) };
                    ImVec2 p3 = { p.x + arrow_size * 0.7f * cosf(move_angle - 2.5f), p.y + arrow_size * 0.7f * sinf(move_angle - 2.5f) };
                    Draw->AddTriangleFilled(p1, p2, p3, COL_LIGHT_BLUE); Draw->AddTriangle(p1, p2, p3, COL_WHITE, 1.5f);
                }
                else if (obj.阵营 == 1) Draw->AddCircleFilled(p, 12.0f, COL_RED);
                else if (obj.阵营 == 2) Draw->AddCircleFilled(p, 6.0f, COL_GREEN);
            }
        }
    }
}

void VolumeKeyHide() {
    std::vector<int> fds; DIR* dir = opendir("/dev/input/");
    if (dir) {
        struct dirent* ptr; while ((ptr = readdir(dir)) != NULL) {
            if (strstr(ptr->d_name, "event")) {
                char path[64]; snprintf(path, sizeof(path), "/dev/input/%s", ptr->d_name);
                int fd = open(path, O_RDWR | O_NONBLOCK); if (fd >= 0) fds.push_back(fd);
            }
        }
        closedir(dir);
    }
    struct input_event ev;
    while (true) {
        for (int fd : fds) {
            while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_KEY && ev.value == 1) { if (ev.code == KEY_VOLUMEDOWN) voice = false; if (ev.code == KEY_VOLUMEUP) voice = true; }
            }
        }
        show_window = voice; usleep(50000);
    }
}

void Draw_Main(ImDrawList* Draw) {
    int read_idx = g_read_index.load(std::memory_order_acquire);
    const GameDataBuffer& buf = g_buffer[read_idx];
    uintptr_t Step1_Addr = libbase + MatrixOffset;
    uintptr_t Ptr1 = getPtr64(Step1_Addr); if (Ptr1 == 0) return;
    uintptr_t Step2_Addr = Ptr1 + 0xA58; uintptr_t Ptr2 = getPtr64(Step2_Addr); if (Ptr2 == 0) return;
    uintptr_t MatrixRealAddr = Ptr2 + 0x2C0; vm_readv(MatrixRealAddr, matrix, 16 * 4);
    if (fabs(matrix[0]) < 0.0001f && fabs(matrix[1]) < 0.0001f) return;

    if (show_draw_prophet && 监管者预知[0] != '\0') DrawCenteredText(Draw, { px, 130 }, COL_RED, 监管者预知);

    // ================== 恢复原本自身锁定逻辑 ==================
// ================== 优化后的自身锁定逻辑 (屏幕中心最近距离算法) ==================
    bool found_self = false;
    float min_center_dist = 9999.0f; // 用于记录离中心最近的距离
    int best_candidate_index = -1;   // 最佳候选人的索引

    for (int i = 0; i < buf.数量; i++) {
        const DataStruct& obj = buf.data[i];

        // 1. 基础过滤：必须是监管者(1)或求生者(2)
        if (obj.阵营 != 2 && obj.阵营 != 1) continue;

        // 2. 获取坐标
        Vector3A pos;
        if (!GetEntityPosition(obj, pos)) continue;

        // 3. 计算相机 Z 深度 (Camera Z)
        float cam_z = matrix[3] * pos.X + matrix[7] * pos.Z + matrix[11] * pos.Y + matrix[15];

        // 4. 宽松的深度过滤：防止把自己过滤掉
        // 正常视角下自身通常在 10~50 左右，但为了防止特殊情况，范围放宽一点
        if (cam_z < 5.0f || cam_z > 80.0f) continue;

        // 5. 读取标志位 (保留原有的校验逻辑，增加安全性)
        int zy = 0;
        vm_readv(obj.obj + 0xAA, &zy, 1);
        if (!(zy & 1)) continue; // 如果校验位不对，跳过

        // 6. 计算屏幕坐标 X (核心修改)
        // 利用矩阵计算该对象在屏幕上的 X 位置
        float r_x = px + (matrix[0] * pos.X + matrix[4] * pos.Z + matrix[8] * pos.Y + matrix[12]) / cam_z * px;

        // 7. 计算离屏幕中心(px)的水平距离绝对值
        float dist_from_center = fabsf(r_x - px);

        // 8. 寻找“最中心”的那个对象
        // 如果当前对象比之前的候选者更接近屏幕中心，则更新候选者
        if (dist_from_center < min_center_dist) {
            // 增加一个阈值限制，只有在屏幕中心左右一定范围内才会被判定为自己 (例如 150像素)
            if (dist_from_center < 150.0f) {
                min_center_dist = dist_from_center;
                best_candidate_index = i;
            }
        }
    }

    // 9. 应用最佳结果
    if (best_candidate_index != -1) {
        const DataStruct& selfObj = buf.data[best_candidate_index];
        GetEntityPosition(selfObj, Z); // 更新 Z 坐标为自身坐标
        自身 = selfObj.obj;
        自身阵营 = selfObj.阵营;
        strcpy(自身名字, selfObj.str);
        found_self = true;
    }

    // 保底逻辑：如果上面的算法没找到（比如正在上椅子视角极偏），才退化为取第一个求生者
    if (!found_self) {
        // 如果之前有锁定的自身且有效，保持不变，否则重置
        if (Z.X == 0) {
            for (int i = 0; i < buf.数量; i++) {
                if (buf.data[i].阵营 == 2) {
                    GetEntityPosition(buf.data[i], Z);
                    break;
                }
            }
        }
    }

    for (int i = 0; i < buf.数量; i++) {
        const DataStruct& obj = buf.data[i]; if (obj.obj == 自身 && found_self) continue;
        Vector3A pos; if (!GetEntityPosition(obj, pos)) continue;
        if (ShouldSkipEntity(obj, pos)) continue;
        float cam_z = matrix[3] * pos.X + matrix[7] * pos.Z + matrix[11] * pos.Y + matrix[15];
        if (cam_z <= 0.1f) continue;
        float dist = sqrt(pow(pos.X - Z.X, 2) + pow(pos.Y - Z.Y, 2) + pow(pos.Z - Z.Z, 2)) / 距离比例;
        float r_x = px + (matrix[0] * pos.X + matrix[4] * pos.Z + matrix[8] * pos.Y + matrix[12]) / cam_z * px;
        float r_y = py - (matrix[1] * pos.X + matrix[5] * (pos.Z + 0.0f) + matrix[9] * pos.Y + matrix[13]) / cam_z * py;
        float r_w = py - (matrix[1] * pos.X + matrix[5] * (pos.Z + 20.0f) + matrix[9] * pos.Y + matrix[13]) / cam_z * py;
        float h = r_y - r_w, w = h / 2.0f;
        if (Debugging && obj.阵营 != 4) {
            char buf_txt[256]; // 加大了缓冲区防止文字过长溢出

            // 格式说明：[类名] X坐标, Y坐标, Z坐标
            // 例如：[BP_Player_Wuchang_C] 105.0, -20.5, 30.0
            snprintf(buf_txt, sizeof(buf_txt), "[%s] %.1f, %.1f, %.1f", obj.类名, pos.X, pos.Y, pos.Z);

            DrawCenteredText(Draw, { r_x, r_y + 15 }, COL_YELLOW, buf_txt);
        }
        if (obj.阵营 == 3 && show_draw_secret_mechine) {
            std::string txt = std::string(obj.str) + " " + std::to_string((int)dist) + "m";
            DrawCenteredText(Draw, { r_x, r_y }, COL_WHITE, txt.c_str());
        }
        else if (obj.阵营 == 4 && show_draw_Prop) {
            std::string txt = std::string(obj.str) + " " + std::to_string((int)dist) + "m";
            DrawCenteredText(Draw, { r_x, r_y }, COL_YELLOW, txt.c_str());
        }
        else if (obj.阵营 == 1 || obj.阵营 == 2) {
            ImColor boxColor = (obj.阵营 == 1) ? COL_RED : COL_GREEN;
            Draw->AddRect({ r_x - w / 2, r_w }, { r_x + w / 2, r_y }, boxColor, 0, 0, 1.8f);
            DrawCenteredText(Draw, { r_x, r_w - 35 }, COL_ORANGE, obj.str);
            std::string dStr = std::to_string((int)dist) + "m";
            DrawCenteredText(Draw, { r_x, r_y + 5 }, COL_ORANGE, dStr.c_str());
            if (show_draw_line) Draw->AddLine({ px, 160 }, { r_x, r_w }, boxColor, 1.5f);
        }
    }
}

void Layout_tick_UI(bool* main_thread_flag) {
    static bool init_once = false; if (!init_once) { std::thread(VolumeKeyHide).detach(); init_once = true; }
    px = displayInfo.width / 2.0f; py = displayInfo.height / 2.0f;
    if (enable_arms_factory) Draw_UnifiedMap(ImGui::GetForegroundDrawList(), Maps[g_manual_map_index]);
    Draw_Main(ImGui::GetForegroundDrawList());
    if (show_window) {
        ImGui::Begin("Acid_D5", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        if (ImGui::CollapsingHeader("数据诊断")) {
            ImGui::Text("状态: %s", 状态 == 2 ? "✅ 已连接" : "扫描中...");
            int idx = g_read_index.load(); ImGui::Text("对象数: %d", g_buffer[idx].数量);
            ImGui::Text("SelfPos: (%.1f, %.1f)", Z.X, Z.Y); ImGui::Text("锁定对象: %s", 自身名字);
        }
        if (ImGui::CollapsingHeader("小地图设置")) {
            ImGui::Checkbox("启用小地图", &enable_arms_factory);
            const char* map_names[] = { "军工厂", "红教堂", "圣心医院", "湖景村", "月亮河", "里奥回忆", "永眠镇", "唐人街", "不归林" };
            ImGui::Combo("选择地图", &g_manual_map_index, map_names, IM_ARRAYSIZE(map_names));
            ImGui::SliderFloat("地图缩放", &map_display_size, 100.0f, 800.0f);
            ImGui::SliderFloat("位置 X", &map_pos_x, 0, displayInfo.width);
            ImGui::SliderFloat("位置 Y", &map_pos_y, 0, displayInfo.height);
        }
        if (ImGui::CollapsingHeader("显示设置")) {
            ImGui::Checkbox("显示射线", &show_draw_line);
            ImGui::Checkbox("显示名字/距离", &show_draw_prophet);
            ImGui::Checkbox("显示密码机/地窖", &show_draw_secret_mechine);
            ImGui::Checkbox("显示道具", &show_draw_Prop);
            ImGui::Checkbox("过滤幽灵/隐身", &inform_ghost);
            ImGui::Checkbox("调试模式", &Debugging);
        }
        if (ImGui::Button("退出程序")) exit(0);
        ImGui::End();
    }
}