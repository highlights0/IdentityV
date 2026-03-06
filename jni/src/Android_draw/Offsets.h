// Offsets.h - 适配游戏新版本的偏移管理
#pragma once
#include <cstdint>

namespace Offsets {

    // ===== 特征值（已根据“新版.cpp”更新）=====
    // 矩阵特征码：新版使用 64 位长整型特征
    constexpr unsigned long MATRIX_FEATURE_HEX = 0x656A624F72655028;
    constexpr int MATRIX_CHECK_VALUE = 257;        // 偏移 +304 处的校验值

    // 数组特征值
    constexpr int ARRAY_FEATURE_1 = 16384;        // 基础特征
    constexpr int ARRAY_FEATURE_2 = 257;          // 校验值
    constexpr float ARRAY_CHECK_FLOAT = 1.0f;     // 新版增加的浮点校验

    // ===== 类名指针链（已更新）=====
    // 路径：对象 -> +0xF8 -> +0x0 -> +0x8 -> +0x20 -> +0x20 -> +0x8 (Name)
    constexpr uintptr_t CLASSNAME_BASE = 0xF8;    // 从 0xE0 更新为 0xF8
    constexpr uintptr_t CLASSNAME_STEP1 = 0x0;
    constexpr uintptr_t CLASSNAME_STEP2 = 0x8;
    constexpr uintptr_t CLASSNAME_STEP3 = 0x20;
    constexpr uintptr_t CLASSNAME_STEP4 = 0x20;
    constexpr uintptr_t CLASSNAME_FINAL = 0x8;
    constexpr uintptr_t CLASSNAME_LEN = 0x10;     // 长度偏移从 0x8 更新为 0x10

    // ===== 矩阵指针链（根据新版逻辑重组）=====
    // 逻辑：Ptr1 = *(Base + Offset); Ptr2 = *(Ptr1 + 0xA58); Matrix = Ptr2 + 0x2C0
    constexpr uintptr_t MATRIX_PTR_1 = 0x0;       // 第一级直接读取 Offset 处内容
    constexpr uintptr_t MATRIX_PTR_2 = 0xA58;     // 第二级偏移
    constexpr uintptr_t MATRIX_OFFSET_2 = 0x2C0;  // 最终矩阵地址偏移

    // 以下保留原版占位，用于兼容旧代码结构
    constexpr uintptr_t MATRIX_OFFSET_1 = 0x0;
    constexpr uintptr_t MATRIX_CAMERA = 0x290;    // 建议后续在调试台寻找新相机偏移

    // ===== 数组搜索相关（已更新）=====
    constexpr uintptr_t ARRAY_ADDR_OFFSET = 0x38;    // 新版计算结果：+56 (0x38)
    constexpr uintptr_t ARRAY_ELEMENT_SIZE = 0x8;
    constexpr uintptr_t ARRAY_PREV_CHECK = 0x8;

    // ===== 对象结构（已更新）=====
    constexpr uintptr_t OBJ_COORD_PTR = 0x28;     // 从 0x40 更新为 0x28

    // 坐标偏移 (注意：新版 0xA4 是 Y/Up，0xA8 是 Z/Forward，可能需要根据绘图逻辑互换)
    constexpr uintptr_t COORD_X = 0xA0;           //
    constexpr uintptr_t COORD_Y = 0xA4;           //
    constexpr uintptr_t COORD_Z = 0xA8;           //

    // ===== 状态判断 =====
    // 阵营偏移
    constexpr uintptr_t OBJ_CAMP = 0x64;          // 从 0x150 更新为 0x64
    constexpr float CAMP_VALUE = 450.0f;          // 阵营特征值保留原版

    // 鬼魂状态（新版未提供，暂时保留原版）
    constexpr uintptr_t OBJ_GHOST = 0x78;
    constexpr int GHOST_VALUE = 65150;

    constexpr uintptr_t OBJ_CHECK_OFFSET = 0x70; // 对应源码中的 v31 + 112
    constexpr uintptr_t OBJ_CHECK_OFFSET_2 = 0x1A0; // 对应源码中的 v31 + 416

    // 更新特征值
    constexpr int VALID_ACTOR_VALUE = 0x1000000; // 十进制 16777216
    constexpr float VALID_ACTOR_FLOAT = 450.0f;

    // 备用坐标（新版未提供，暂时保留原版）
    constexpr uintptr_t COORD_ALT_X = 0xE0;
    constexpr uintptr_t COORD_ALT_Z = 0xE4;
    constexpr uintptr_t COORD_ALT_Y = 0xE8;

    // ===== 辅助宏 =====
    // 使用新版 0xF8 链条获取类名
#define GET_CLASSNAME_PTR(obj) \
        (getPtr64(getPtr64(getPtr64(getPtr64(getPtr64((obj) + Offsets::CLASSNAME_BASE) + Offsets::CLASSNAME_STEP1) + Offsets::CLASSNAME_STEP2) + Offsets::CLASSNAME_STEP3) + Offsets::CLASSNAME_STEP4) + Offsets::CLASSNAME_FINAL)

} // namespace Offsets