/**
 * ESP32 SD卡测试程序
 *
 * 引脚配置 (SPI模式):
 * IO21 - CMD  -> MOSI
 * IO22 - CLK  -> SCK
 * IO23 - DO   -> MISO
 *
 * 注意：如果硬件上没有CS引脚，程序会尝试使用虚拟CS
 * 这种情况下SD卡座的CS需要接地或内部下拉
 */

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// SD卡引脚定义 (SPI模式)
#define SD_MOSI  21   // CMD -> MOSI
#define SD_SCK   22   // CLK -> SCK
#define SD_MISO  23   // DO  -> MISO
#define SD_CS    12    // CS引脚 - 飞线连接到TF卡的CD/DAT3(引脚2)

// 注意: 如果用IO2作为CS，可能会影响ESP32启动，建议用IO5

SPIClass sdSPI(HSPI);

void setup() {
    // 初始化串口
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }

    Serial.println();
    Serial.println("=====================================");
    Serial.println("    ESP32 SD卡信息读取测试程序");
    Serial.println("         (SPI模式)");
    Serial.println("=====================================");
    Serial.println();

    // 打印引脚配置
    Serial.println("【引脚配置】");
    Serial.printf("  MOSI (CMD): GPIO%d\n", SD_MOSI);
    Serial.printf("  SCK  (CLK): GPIO%d\n", SD_SCK);
    Serial.printf("  MISO (DO):  GPIO%d\n", SD_MISO);
    Serial.printf("  CS:         GPIO%d\n", SD_CS);
    Serial.println();

    // 初始化SPI，使用自定义引脚
    Serial.println("【初始化SPI...】");
    sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

    // 初始化SD卡
    Serial.println("【初始化SD卡...】");

    if (!SD.begin(SD_CS, sdSPI)) {
        Serial.println("❌ SD卡初始化失败!");
        Serial.println();
        Serial.println("可能的原因:");
        Serial.println("  1. SD卡未正确插入");
        Serial.println("  2. SD卡损坏或格式不支持 (需FAT32)");
        Serial.println("  3. 引脚连接错误");
        Serial.println("  4. 必须连接CS引脚才能工作");
        Serial.println();
        Serial.println("请检查硬件连接...");
        while (1) {
            delay(1000);
        }
    }

    Serial.println("✅ SD卡初始化成功!");
    Serial.println();

    // 读取并打印SD卡信息
    printSDCardInfo();

    // 列出根目录文件
    listDirectory("/", 0);

    Serial.println();
    Serial.println("=====================================");
    Serial.println("       SD卡信息读取完成");
    Serial.println("=====================================");
}

void loop() {
    // 每10秒重新读取一次SD卡信息
    delay(10000);

    Serial.println();
    Serial.println("--- 刷新SD卡信息 ---");
    printSDCardInfo();
}

/**
 * 打印SD卡详细信息
 */
void printSDCardInfo() {
    Serial.println("【SD卡信息】");

    // 获取卡类型
    uint8_t cardType = SD.cardType();
    Serial.print("  卡类型: ");
    switch (cardType) {
        case CARD_NONE:
            Serial.println("未检测到SD卡");
            return;
        case CARD_MMC:
            Serial.println("MMC");
            break;
        case CARD_SD:
            Serial.println("SDSC (标准容量)");
            break;
        case CARD_SDHC:
            Serial.println("SDHC (高容量)");
            break;
        default:
            Serial.println("未知类型");
            break;
    }

    // 获取容量信息
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);  // MB
    uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);  // MB
    uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);    // MB

    Serial.printf("  卡容量:   %llu MB (%.2f GB)\n", cardSize, cardSize / 1024.0);
    Serial.printf("  总空间:   %llu MB (%.2f GB)\n", totalBytes, totalBytes / 1024.0);
    Serial.printf("  已使用:   %llu MB (%.2f GB)\n", usedBytes, usedBytes / 1024.0);
    Serial.printf("  可用空间: %llu MB (%.2f GB)\n", totalBytes - usedBytes, (totalBytes - usedBytes) / 1024.0);

    // 计算使用率
    if (totalBytes > 0) {
        float usagePercent = (float)usedBytes / totalBytes * 100.0;
        Serial.printf("  使用率:   %.1f%%\n", usagePercent);
    }

    Serial.println();
}

/**
 * 递归列出目录内容
 * @param dirname 目录路径
 * @param level 递归深度
 */
void listDirectory(const char* dirname, uint8_t level) {
    if (level == 0) {
        Serial.println("【目录内容】");
        Serial.printf("  目录: %s\n", dirname);
        Serial.println("  ----------------------------------------");
    }

    File root = SD.open(dirname);
    if (!root) {
        Serial.println("  ❌ 无法打开目录");
        return;
    }

    if (!root.isDirectory()) {
        Serial.println("  ❌ 这不是一个目录");
        root.close();
        return;
    }

    File file = root.openNextFile();
    int fileCount = 0;
    int dirCount = 0;

    while (file) {
        // 打印缩进
        for (uint8_t i = 0; i < level + 1; i++) {
            Serial.print("    ");
        }

        if (file.isDirectory()) {
            Serial.printf("📁 [目录] %s\n", file.name());
            dirCount++;

            // 递归列出子目录 (最多3层)
            if (level < 2) {
                String path = String(dirname);
                if (path != "/") {
                    path += "/";
                }
                path += file.name();
                listDirectory(path.c_str(), level + 1);
            }
        } else {
            // 打印文件信息
            Serial.printf("📄 %s", file.name());

            // 格式化文件大小
            size_t fileSize = file.size();
            if (fileSize < 1024) {
                Serial.printf(" (%d B)\n", fileSize);
            } else if (fileSize < 1024 * 1024) {
                Serial.printf(" (%.1f KB)\n", fileSize / 1024.0);
            } else {
                Serial.printf(" (%.2f MB)\n", fileSize / (1024.0 * 1024.0));
            }
            fileCount++;
        }

        file = root.openNextFile();
    }

    root.close();

    if (level == 0) {
        Serial.println("  ----------------------------------------");
        Serial.printf("  统计: %d 个文件, %d 个文件夹\n", fileCount, dirCount);
    }
}
