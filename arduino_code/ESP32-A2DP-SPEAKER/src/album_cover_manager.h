/**
 *  * Copyright (c) 2026 Cyberware Workshop
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
 *
 * Commercial use of this software requires prior written authorization from the Licensor.
 * 请注意：将 Cyberware Workshop 代码用于商业用途需要事先获得许可方的授权。
 * 删除与修改版权属于侵权行为，请尊重作者版权，避免产生不必要的纠纷。
 *
 * @author Cyberware Workshop Team
 * @date 2026
 
 * 专辑封面管理模块
 * 
 * 功能：
 * - 管理多个专辑封面图片
 * - 支持通过名称或索引切换封面
 * - 预留扩展接口，方便添加新图片
 * 
 * 使用方法：
 * 1. 在 emos/ 目录下添加新的图片文件（.c格式）
 * 2. 在 album_cover_manager.cpp 中 include 新图片
 * 3. 在 album_covers 数组中添加新图片的映射
 * 4. 调用 getAlbumCover() 获取图片指针
 */

#ifndef ALBUM_COVER_MANAGER_H
#define ALBUM_COVER_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>

/**
 * 专辑封面结构体
 */
struct AlbumCover {
    const char* name;              // 封面名称
    const lv_img_dsc_t* image;     // 图片数据指针
};

/**
 * 初始化专辑封面管理器
 */
void initAlbumCoverManager();

/**
 * 通过名称获取专辑封面
 * 
 * @param name 封面名称
 * @return 图片数据指针，如果未找到返回默认封面
 */
const lv_img_dsc_t* getAlbumCoverByName(const char* name);

/**
 * 通过索引获取专辑封面
 * 
 * @param index 封面索引（0开始）
 * @return 图片数据指针，如果索引超出范围返回默认封面
 */
const lv_img_dsc_t* getAlbumCoverByIndex(int index);

/**
 * 获取默认专辑封面
 * 
 * @return 默认图片数据指针
 */
const lv_img_dsc_t* getDefaultAlbumCover();

/**
 * 获取专辑封面总数
 *
 * @return 封面数量
 */
int getAlbumCoverCount();

/**
 * 打印所有可用的专辑封面名称
 */
void printAvailableCovers();

/**
 * 切换到下一个封面（循环）
 *
 * @return 新的封面图片数据指针
 */
const lv_img_dsc_t* nextAlbumCover();

/**
 * 切换到上一个封面（循环）
 *
 * @return 新的封面图片数据指针
 */
const lv_img_dsc_t* previousAlbumCover();

/**
 * 获取当前封面索引
 *
 * @return 当前封面索引
 */
int getCurrentCoverIndex();

/**
 * 获取当前封面
 *
 * @return 当前封面图片数据指针
 */
const lv_img_dsc_t* getCurrentAlbumCover();

#endif // ALBUM_COVER_MANAGER_H

