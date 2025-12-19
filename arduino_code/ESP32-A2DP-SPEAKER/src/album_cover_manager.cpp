/**
 * 专辑封面管理模块实现
 */

#include "album_cover_manager.h"

// ==================== 包含所有专辑封面图片 ====================
// 在这里添加新的图片头文件
#include "../emos/animimg001.h"

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
    {"default", &animimg001},      // 默认封面
    {"animimg001", &animimg001},   // 示例封面1
    
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
            Serial.printf("找到封面: %s (索引 %d)\n", name, i);
            return album_covers[i].image;
        }
    }
    
    // 未找到，返回默认封面
    Serial.printf("未找到封面: %s，使用默认封面\n", name);
    return getDefaultAlbumCover();
}

/**
 * 通过索引获取专辑封面
 */
const lv_img_dsc_t* getAlbumCoverByIndex(int index) {
    if (index < 0 || index >= album_cover_count) {
        Serial.printf("封面索引超出范围: %d，使用默认封面\n", index);
        return getDefaultAlbumCover();
    }
    
    Serial.printf("获取封面: %s (索引 %d)\n", album_covers[index].name, index);
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

