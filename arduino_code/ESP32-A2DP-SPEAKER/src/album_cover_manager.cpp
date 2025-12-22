/**
 * 专辑封面管理模块实现
 */

#include "album_cover_manager.h"

// ==================== 包含所有专辑封面图片 ====================
// 在这里添加新的图片头文件
#include "../emos/xingkong.h"
#include "../emos/dianjuren1.h"
#include "../emos/dianjuren2.h"
#include "../emos/gaoda.h"
#include "../emos/labubu.h"
// TODO: 添加更多图片时，在这里 include
// 例如：
// #include "../emos/cover_rock.c"
// #include "../emos/cover_pop.c"
// #include "../emos/cover_jazz.c"

// ==================== 专辑封面映射表 ====================
/**
 * 专辑封面数组
 * 
 * 添加新封面的步骤：
 * 1. 在上方 include 新的图片文件
 * 2. 在下方数组中添加新的 AlbumCover 条目
 * 
 * 格式：{"封面名称", &图片变量名}
 */
static const AlbumCover album_covers[] = {
    {"default", &xingkong},      // 默认封面
    {"xingkong", &xingkong},   // 示例封面1
    {"dianjuren1", &dianjuren1}, 
    {"dianjuren2", &dianjuren2}, 
    {"gaoda", &gaoda}, 
    {"labubu", &labubu}, 

    // TODO: 在这里添加更多封面
    // 例如：
    // {"rock", &cover_rock},
    // {"pop", &cover_pop},
    // {"jazz", &cover_jazz},
};

// 专辑封面数量
static const int album_cover_count = sizeof(album_covers) / sizeof(album_covers[0]);

// 默认封面索引
static const int default_cover_index = 0;

// 当前封面索引（用于切换曲目时循环切换）
static int current_cover_index = 0;

// ==================== 函数实现 ====================

/**
 * 初始化专辑封面管理器
 */
void initAlbumCoverManager() {
    Serial.println("========== 专辑封面管理器 ==========");
    Serial.printf("已加载 %d 个专辑封面\n", album_cover_count);
    printAvailableCovers();
    Serial.println("===================================");
}

/**
 * 通过名称获取专辑封面
 */
const lv_img_dsc_t* getAlbumCoverByName(const char* name) {
    if (name == nullptr || strlen(name) == 0) {
        return getDefaultAlbumCover();
    }

    // 遍历查找匹配的封面
    for (int i = 0; i < album_cover_count; i++) {
        if (strcmp(album_covers[i].name, name) == 0) {
            return album_covers[i].image;
        }
    }

    // 未找到，返回默认封面
    return getDefaultAlbumCover();
}

/**
 * 通过索引获取专辑封面
 */
const lv_img_dsc_t* getAlbumCoverByIndex(int index) {
    if (index < 0 || index >= album_cover_count) {
        return getDefaultAlbumCover();
    }

    return album_covers[index].image;
}

/**
 * 获取默认专辑封面
 */
const lv_img_dsc_t* getDefaultAlbumCover() {
    return album_covers[default_cover_index].image;
}

/**
 * 获取专辑封面总数
 */
int getAlbumCoverCount() {
    return album_cover_count;
}

/**
 * 打印所有可用的专辑封面名称
 */
void printAvailableCovers() {
    Serial.println("可用的专辑封面:");
    for (int i = 0; i < album_cover_count; i++) {
        Serial.printf("  [%d] %s%s\n",
                      i,
                      album_covers[i].name,
                      (i == default_cover_index) ? " (默认)" : "");
    }
}

/**
 * 切换到下一个封面（循环）
 */
const lv_img_dsc_t* nextAlbumCover() {
    current_cover_index++;
    if (current_cover_index >= album_cover_count) {
        current_cover_index = 0;  // 循环到第一个
    }

    return album_covers[current_cover_index].image;
}

/**
 * 切换到上一个封面（循环）
 */
const lv_img_dsc_t* previousAlbumCover() {
    current_cover_index--;
    if (current_cover_index < 0) {
        current_cover_index = album_cover_count - 1;  // 循环到最后一个
    }

    return album_covers[current_cover_index].image;
}

/**
 * 获取当前封面索引
 */
int getCurrentCoverIndex() {
    return current_cover_index;
}

/**
 * 获取当前封面
 */
const lv_img_dsc_t* getCurrentAlbumCover() {
    return album_covers[current_cover_index].image;
}

