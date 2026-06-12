#ifndef GAME_CONFIG_HPP
#define GAME_CONFIG_HPP

namespace Config {
    // --- 顯示轉換比例 ---
    constexpr float PIXELS_PER_MM = 8.0f;     // 核心設定：1mm = 8 pixels

    // --- 網格設定 ---
    constexpr int GRID_ROWS = 9;    
    constexpr int GRID_COLS = 7;    
    constexpr float SPACING = 0.5f; 

    // --- 尺寸設定 (單位: mm) ---
    constexpr float PROP_DIAMETER = 6.0f;         // 道具直徑
    constexpr float BRICK_SQUARE_SIZE = 16.0f;    // 方形磚塊邊長
    constexpr float BRICK_GIANT_SIZE = 33.0f;     // 巨型方形磚塊邊長

    // --- 球類尺寸 (單位: mm，以直徑計算) ---
    constexpr float BALL_BIG_DIAMETER = 11.0f;    
    constexpr float BALL_NORMAL_DIAMETER = 4.0f;  
    constexpr float BALL_SMALL_DIAMETER = 0.75f;  
    constexpr float BALL_MINI_DIAMETER = 0.125f;  

    // --- 機率設定 (%) ---
    constexpr int PROB_EMPTY = 50;            // 空曠處機率 
    constexpr int PROB_BRICK_GROUP = 85;      // 生成磚塊的總機率 
    constexpr int PROB_PROP_GROUP = 15;       // 生成道具的總機率 
    
    // 磚塊內部機率 (佔 BRICK_GROUP 的比例)
    constexpr int PROB_BRICK_NORMAL = 35;     // 普通方形 
    constexpr int PROB_BRICK_TRIANGLE = 25;   // 三角形 
    constexpr int PROB_BRICK_SPECIAL = 25;    // 其他特殊磚塊 

    // --- 遊戲數值 ---
    constexpr float BALL_SPEED = 120.0f;      // 球速：每秒 120mm 
}

// 磚塊種類列舉
enum class BrickType {
    Normal,         // 普通方形 
    HighDensity,    // 高密度 (回合x2) 
    Triangle,       // 三角形 
    Linked,         // 連鎖 
    Spiked,         // 尖刺 (扣血+吃球) 
    Split,          // 分裂 
    ThickBottom,    // 厚底 (底部無敵) 
    Giant,          // 巨型 (2x2) 
    Invisible,      // 隱形 
    Falling,        // 下墜 
};

// 道具種類列舉
enum class PropType {
    AddBall,        // 加一球 [cite: 73]
    Coin,           // 金幣 [cite: 75]
    Scatter,        // 發散 [cite: 66]
    HorizontalLaser,// 橫向雷射 [cite: 68]
    VerticalLaser   // 直向雷射 [cite: 70]
};

// 商店道具列舉
enum class ShopItem {
    Drill,              // 鑽頭 (5金) [cite: 78]
    RainbowBall,        // 彩虹球 (8金) [cite: 81]
    Dynamite,           // 炸藥 (5金) [cite: 84]
    HorizontalDestroy   // 橫向摧毀雷射 (4金) [cite: 87]
};

#endif