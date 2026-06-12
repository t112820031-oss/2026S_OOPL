#include "LevelManager.hpp"
#include "DeathEffect.hpp"
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <queue>
#include <set>
#include <unordered_set>

// 建構子
LevelManager::LevelManager(Util::GameObject& root) : m_Root(root) {
    // 初始化隨機數生成器 gen
}

// 建立全域或類別成員的隨機數生成器
static std::random_device rd;
static std::mt19937 gen(rd());

// 主生成邏輯
void LevelManager::SpawnRowEndless(int currentRound) {
    std::vector<int> availableCols = {0, 1, 2, 3, 4, 5, 6};

    // 排除被既有 GIANT 上半部占住的 col（避免新磚塊跟它重疊）
    {
        auto occupied = ComputeOccupied();
        availableCols.erase(
            std::remove_if(availableCols.begin(), availableCols.end(),
                [&](int c) { return occupied.count({1, c}); }),
            availableCols.end()
        );
    }

    std::shuffle(availableCols.begin(), availableCols.end(), gen);

    // Phase 0: 巨大方形磚塊（30 回合後、每 15 回合一次、生 1~2 個）
    if (currentRound >= 30 && currentRound % 15 == 0) {
        int giantCount = 1 + (rand() % 2);  // 1 或 2 個

        // 找所有可放 GIANT 的「起始 col」（c 跟 c+1 都還可用，且 c <= 5）
        std::set<int> availSet(availableCols.begin(), availableCols.end());
        std::vector<int> validStarts;
        for (int c = 0; c <= 5; ++c) {
            if (availSet.count(c) && availSet.count(c + 1)) {
                validStarts.push_back(c);
            }
        }
        std::shuffle(validStarts.begin(), validStarts.end(), gen);

        std::set<int> usedCols;
        int spawned = 0;
        for (int c : validStarts) {
            if (spawned >= giantCount) break;
            // 跟已選的 GIANT 不能相鄰，否則會擠在一起
            if (usedCols.count(c) || usedCols.count(c + 1) ||
                usedCols.count(c - 1)) continue;

            SpawnGiant(c, currentRound);
            usedCols.insert(c);
            usedCols.insert(c + 1);
            spawned++;
        }

        // 從 availableCols 移除被 GIANT 占用的 col
        availableCols.erase(
            std::remove_if(availableCols.begin(), availableCols.end(),
                [&](int c) { return usedCols.count(c); }),
            availableCols.end()
        );
    }

    // 沒位置就略過這回合（GIANT 太多佔光了）
    if (availableCols.size() < 2) return;

    // 必定生成：1個加一球道具
    int addBallCol = availableCols.back();
    availableCols.pop_back();
    
    // 升級 2：統一使用 SpawnBrick，血量傳 0 代表道具，方向傳 0 即可
    SpawnBrick(addBallCol, Brick::BrickType::ADD_BALL, 0, 0);

    // 必定生成：至少1個任意磚塊
    int guaranteedBrickCol = availableCols.back();
    availableCols.pop_back();
    
    Brick::BrickType gType = RollGuaranteedBrick(currentRound);
    int gOrientation = (gType == Brick::BrickType::TRIANGLE) ? (rand() % 4) : 0;
    SpawnBrick(guaranteedBrickCol, gType, currentRound, gOrientation);

    // 金幣邏輯 (每 3 回合保底送一個)
    if (currentRound % 3 == 0 && !availableCols.empty()) {
        int coinCol = availableCols.back();
        availableCols.pop_back();
        
        // 同樣替換為 SpawnBrick
        SpawnBrick(coinCol, Brick::BrickType::COIN, 0, 0);
    }

    // 剩下的欄位 (機率生成區)
    std::uniform_int_distribution<> dis100(1, 100);
    // 普通方形 35% / 三角形 25% / 特殊磚塊 25% / VLASER 5% / HLASER 5% / 散射 5%
    for (int col : availableCols) {
        // 50% 機率該格生成東西（保留外層的稀疏度）
        if (dis100(gen) > 50) continue;

        int roll = dis100(gen); // 1~100

        if (roll <= 35) {
            // 普通方形 35%
            SpawnBrick(col, Brick::BrickType::SQUARE, currentRound, 0);
        }
        else if (roll <= 60) {
            // 三角形 25%
            int orientation = rand() % 4;
            SpawnBrick(col, Brick::BrickType::TRIANGLE, currentRound, orientation);
        }
        else if (roll <= 85) {
            // 特殊磚塊 25%
            Brick::BrickType sType = RollSpecialBrick(currentRound);
            SpawnBrick(col, sType, currentRound, 0);
        }
        else if (roll <= 90) {
            // 直向雷射 5%
            SpawnBrick(col, Brick::BrickType::VLASER, 0, 0);
        }
        else if (roll <= 95) {
            // 橫向雷射 5%
            SpawnBrick(col, Brick::BrickType::HLASER, 0, 0);
        }
        else {
            // 散射 5%
            SpawnBrick(col, Brick::BrickType::SCATTER, 0, 0);
        }
    }
}

void LevelManager::SpawnBrick(int col, Brick::BrickType type, int round, int orientation) {
    int targetRow = 1; 

    auto pos = GetPositionByColumn(col, targetRow); // 使用新的 targetRow
    
    // 把 orientation 傳給 Brick 的建構子
    auto brick = std::make_shared<Brick>(type, round, orientation); 
    brick->SetPosition(pos); 
    brick->SetRowCol(targetRow, col);

    m_Bricks.push_back(brick); 
    m_Root.AddChild(brick);

    // LINKED：把這個 LINKED 的 HP 累加進共享池
    if (type == Brick::BrickType::LINKED) {
        AddLinked(brick->GetHp());
    }
}

void LevelManager::Clear() {
    for (auto& brick : m_Bricks) {
        if (brick) {
            m_Root.RemoveChild(brick);
        }
    }
    // 清空管理陣列
    m_Bricks.clear();
}

void LevelManager::GenerateRow(int currentRound) {
    // 建立一個追蹤 9 個欄位是否被佔用的陣列
    bool slotOccupied[9] = {false};

    if (currentRound >= 30 && currentRound % 15 == 0) {
        int giantCount = (rand() % 2) + 1; 
        
        for (int i = 0; i < giantCount; i++) {
            std::vector<int> possibleStarts;
            for (int c = 0; c < 8; c++) {
                if (!slotOccupied[c] && !slotOccupied[c+1]) {
                    possibleStarts.push_back(c);
                }
            }
            if (!possibleStarts.empty()) {
                int startCol = possibleStarts[rand() % possibleStarts.size()];
                slotOccupied[startCol] = true;
                slotOccupied[startCol + 1] = true;
            }
        }
    }

    // 收集剩餘空位並打亂 (Shuffle)
    std::vector<int> emptySlots;
    for (int i = 0; i < 9; i++) {
        if (!slotOccupied[i]) emptySlots.push_back(i);
    }
    
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(emptySlots.begin(), emptySlots.end(), std::default_random_engine(seed));

    // 必出物件 (消耗 emptySlots)
    if (!emptySlots.empty()) {
        int col = emptySlots.back();
        emptySlots.pop_back();
    }

    if (currentRound % 5 == 0 && !emptySlots.empty()) {
        int col = emptySlots.back();
        emptySlots.pop_back();
    }

    if (!emptySlots.empty()) {
        int col = emptySlots.back();
        emptySlots.pop_back();
        
        int r = rand() % 100;
        if (r < 50) {
        } 
        else if (r < 75) {
        } 
        // 25% 其他特殊磚塊 
        else {
            Brick::BrickType type = RollSpecialBrick(currentRound);
            
        }
    }

    // 剩餘空位抽卡 (50% 機率生成，50% 機率不生成)
    while (!emptySlots.empty()) {
        int col = emptySlots.back();
        emptySlots.pop_back();

        // 50% 什麼都不生成 (空曠處)
        if (rand() % 100 < 50) {
            continue; 
        }

        // 決定生成什麼 (總和 100%)
        int r = rand() % 100;
        
        // 普通方形磚塊: 35%
        if (r < 35) {
            SpawnBrick(col, Brick::BrickType::SQUARE, currentRound, 0); 
        } 
        // 三角形磚塊: 25% (累積到 60)
        else if (r < 60) {
            SpawnBrick(col, Brick::BrickType::TRIANGLE, currentRound, rand() % 4);
        } 
        // 其他特殊磚塊: 25% (累積到 85)
        else if (r < 85) {
            Brick::BrickType type = RollSpecialBrick(currentRound);
            int orientation = (type == Brick::BrickType::TRIANGLE) ? (rand() % 4) : 0;
            SpawnBrick(col, type, currentRound, orientation);
        } 
        // 直向雷射: 5% (累積到 90)
        else if (r < 90) {
            SpawnProp(col, PropType::VerticalLaser);
        } 
        // 橫向雷射: 5% (累積到 95)
        else if (r < 95) {
            SpawnProp(col, PropType::HorizontalLaser);
        } 
        // 發散道具: 5% (累積到 100)
        else {
            SpawnProp(col, PropType::Scatter);
        }
    }
}

void LevelManager::TriggerTNTExplosion(int centerRow, int centerCol, Util::GameObject& root) {
    std::queue<std::pair<int, int>> explosionQueue;
    explosionQueue.push({centerRow, centerCol});

    std::unordered_set<Brick*> victims;             // 蒐集要被刪掉的磚塊
    std::unordered_set<long long> processedCenters; // 避免同個中心被重複 BFS

    auto encode = [](int r, int c) { return (long long)r * 1000 + c; };

    while (!explosionQueue.empty()) {
        auto [cR, cC] = explosionQueue.front();
        explosionQueue.pop();

        if (processedCenters.count(encode(cR, cC))) continue;
        processedCenters.insert(encode(cR, cC));

        // 掃一遍 m_Bricks，看誰落在這個中心的 3×3 範圍內
        for (auto& brick : m_Bricks) {
            if (!brick) continue;
            if (brick->IsProp()) continue;
            if (victims.count(brick.get())) continue;     
            if (brick->GetRow() == cR && brick->GetCol() == cC) continue; // 跳過中心本身

            int rIdx = brick->GetRow();
            int cIdx = brick->GetCol();
            bool inArea = false;

            if (brick->GetType() == Brick::BrickType::GIANT) {
                bool rowOverlap = std::max(rIdx, cR - 1) <= std::min(rIdx + 1, cR + 1);
                bool colOverlap = std::max(cIdx, cC - 1) <= std::min(cIdx + 1, cC + 1);
                inArea = rowOverlap && colOverlap;
            } else {
                inArea = std::abs(rIdx - cR) <= 1 && std::abs(cIdx - cC) <= 1;
            }

            if (inArea) {
                victims.insert(brick.get());

                if (brick->GetType() == Brick::BrickType::TNT) {
                    explosionQueue.push({rIdx, cIdx});
                }
            }
        }
    }

    for (auto it = m_Bricks.begin(); it != m_Bricks.end(); ) {
        if (victims.count(it->get())) {
            int dmg = m_IsLevelMode ? ((*it)->GetHp() + 1) / 2 : 99999;
            bool dead = (*it)->OnHit(dmg, true);

            if (dead && (*it)->GetType() == Brick::BrickType::SPIKE) {
                OnSpikeKilled();
            }

            // Boss 傳送
            {
                auto bt = (*it)->GetType();
                int threshold = 0;
                if (bt == Brick::BrickType::LEVEL10_BOSS) threshold = 10;
                else if (bt == Brick::BrickType::LEVEL15_BOSS) threshold = 15;
                else if (bt == Brick::BrickType::LEVEL20_BOSS) threshold = 10;

                if (threshold > 0 && !dead) {
                    while ((*it)->m_DmgSinceTeleport >= threshold) {
                        (*it)->m_DmgSinceTeleport -= threshold;
                        if (bt == Brick::BrickType::LEVEL10_BOSS)      Level10TeleportBoss();
                        else if (bt == Brick::BrickType::LEVEL15_BOSS) Level15OnBossDamaged();
                        else if (bt == Brick::BrickType::LEVEL20_BOSS) Level20OnBossDamaged();
                    }
                }
            }

            if (dead) {
                root.RemoveChild(*it);
                it = m_Bricks.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}

bool LevelManager::IsGridSlotEmpty(int row, int col) const {
    return ComputeOccupied().count({row, col}) == 0;
}

bool LevelManager::TryPlaceBomb(glm::vec2 mousePos, int& outRow, int& outCol) {
    int targetRow = -1;
    int targetCol = -1;

    if (!ScreenPosToGridIndex(mousePos, targetRow, targetCol)) {
        return false; 
    }

    if (!IsGridSlotEmpty(targetRow, targetCol)) {
        return false;
    }

    auto bombBrick = std::make_shared<Brick>(Brick::BrickType::TNT, 1, 0);

    const float GRID_START_X = -210.0f;
    const float GRID_START_Y = 360.0f;
    const float SLOT_SIZE = 70.0f;

    glm::vec2 exactPos;
    exactPos.x = GRID_START_X + (targetCol * SLOT_SIZE);
    exactPos.y = GRID_START_Y - (targetRow * SLOT_SIZE);

    bombBrick->SetPosition(exactPos);
    bombBrick->SetRowCol(targetRow, targetCol);

    m_Bricks.push_back(bombBrick);
    m_Root.AddChild(bombBrick);
    
    return true;
}

bool LevelManager::UseDrill(Util::GameObject& root) {
    // 檢查場上是否還有磚塊
    bool hasBricks = false;
    for (const auto& brick : m_Bricks) {
        if (!brick->IsProp()) { 
            hasBricks = true;
            break;
        }
    }
    
    if (!hasBricks) return false;

    // 統計每一排 (Col 0 ~ 6) 的總血量
    int colHpSum[7] = {0}; // 初始化為 0

    for (const auto& brick : m_Bricks) {
        if (brick->IsProp()) continue; 

        int c = brick->GetCol();
        int hp = brick->GetHp();

        if (c >= 0 && c < 7) {
            colHpSum[c] += hp;
        }

        if (brick->GetType() == Brick::BrickType::GIANT && (c + 1) < 7) {
            colHpSum[c + 1] += hp;
        }
    }

    int maxCol = 0;
    int maxHp = -1;
    for (int i = 0; i < 7; ++i) {
        if (colHpSum[i] > maxHp) {
            maxHp = colHpSum[i];
            maxCol = i;
        }
    }

    m_LastDrillTargetCol = maxCol;  

    for (auto it = m_Bricks.begin(); it != m_Bricks.end(); ) {
        auto& brick = *it;
        
        if (brick->IsProp()) {
            ++it;
            continue; 
        }

        bool inTargetCol = (brick->GetCol() == maxCol);
        
        if (brick->GetType() == Brick::BrickType::GIANT && (brick->GetCol() + 1) == maxCol) {
            inTargetCol = true;
        }

        if (inTargetCol) {
            int dmg = m_IsLevelMode ? (brick->GetHp() + 1) / 2 : 99999;
            bool dead = brick->OnHit(dmg, true);

            if (dead && brick->GetType() == Brick::BrickType::SPIKE) {
                OnSpikeKilled();
            }

            {
                auto bt = brick->GetType();
                int threshold = 0;
                if (bt == Brick::BrickType::LEVEL10_BOSS) threshold = 10;
                else if (bt == Brick::BrickType::LEVEL15_BOSS) threshold = 15;
                else if (bt == Brick::BrickType::LEVEL20_BOSS) threshold = 10;

                if (threshold > 0 && !dead) {
                    while (brick->m_DmgSinceTeleport >= threshold) {
                        brick->m_DmgSinceTeleport -= threshold;
                        if (bt == Brick::BrickType::LEVEL10_BOSS)      Level10TeleportBoss();
                        else if (bt == Brick::BrickType::LEVEL15_BOSS) Level15OnBossDamaged();
                        else if (bt == Brick::BrickType::LEVEL20_BOSS) Level20OnBossDamaged();
                    }
                }
            }

            if (dead) {
                if (brick->GetType() == Brick::BrickType::TNT) {
                    TriggerTNTExplosion(brick->GetRow(), brick->GetCol(), root);
                }
                auto effect = std::make_shared<DeathEffect>(
                    brick->GetImagePath(), brick->GetPosition(), brick->GetCurrentColor()
                );
                root.RemoveChild(brick);
                it = m_Bricks.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }

    return true; 
}

bool LevelManager::UseHorizontalLaser(Util::GameObject& root) {
    int maxRow = -1;
    bool hasBricks = false;

    // 1. 找出最靠近底線的那一列 (尋找最大值)
    for (const auto& brick : m_Bricks) {
        if (brick->IsProp()) continue; 

        hasBricks = true;
        
        int r = brick->GetRow();
        if (r > maxRow) {
            maxRow = r;
        }

        m_LastLaserTargetRow = maxRow;
        
        // 特例處理：巨大方形磚塊 (佔據 2x2)
        if (brick->GetType() == Brick::BrickType::GIANT) {
            if (r + 1 > maxRow) {
                maxRow = r + 1;
            }
        }
    }

    if (!hasBricks || maxRow == -1) {
        return false;
    }

    for (auto it = m_Bricks.begin(); it != m_Bricks.end(); ) {
        auto& brick = *it;

        if (brick->IsProp()) {
            ++it;
            continue; 
        }

        bool inTargetRow = (brick->GetRow() == maxRow);

        if (brick->GetType() == Brick::BrickType::GIANT) {
            if (brick->GetRow() == maxRow || (brick->GetRow() + 1) == maxRow) {
                inTargetRow = true;
            }
        }

        if (inTargetRow) {
            int dmg = m_IsLevelMode ? (brick->GetHp() + 1) / 2 : 99999;
            bool dead = brick->OnHit(dmg, true);

            if (dead && brick->GetType() == Brick::BrickType::SPIKE) {
                OnSpikeKilled();
            }

            {
                auto bt = brick->GetType();
                int threshold = 0;
                if (bt == Brick::BrickType::LEVEL10_BOSS) threshold = 10;
                else if (bt == Brick::BrickType::LEVEL15_BOSS) threshold = 15;
                else if (bt == Brick::BrickType::LEVEL20_BOSS) threshold = 10;

                if (threshold > 0 && !dead) {
                    while (brick->m_DmgSinceTeleport >= threshold) {
                        brick->m_DmgSinceTeleport -= threshold;
                        if (bt == Brick::BrickType::LEVEL10_BOSS)      Level10TeleportBoss();
                        else if (bt == Brick::BrickType::LEVEL15_BOSS) Level15OnBossDamaged();
                        else if (bt == Brick::BrickType::LEVEL20_BOSS) Level20OnBossDamaged();
                    }
                }
            }

            if (dead) {
                if (brick->GetType() == Brick::BrickType::TNT) {
                    TriggerTNTExplosion(brick->GetRow(), brick->GetCol(), root);
                }
                auto effect = std::make_shared<DeathEffect>(
                    brick->GetImagePath(), brick->GetPosition(), brick->GetCurrentColor()
                );
                root.RemoveChild(brick);
                it = m_Bricks.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }

    return true; 
}

bool LevelManager::ScreenPosToGridIndex(glm::vec2 screenPos, int& outRow, int& outCol) const {
    const float GRID_START_X = -210.0f;
    const float GRID_START_Y = 360.0f;
    const float SLOT_SIZE = 70.0f;

    float offsetX = screenPos.x - GRID_START_X;
    float offsetY = GRID_START_Y - screenPos.y; 

    int col = static_cast<int>(std::floor((offsetX + SLOT_SIZE / 2.0f) / SLOT_SIZE));
    int row = static_cast<int>(std::floor((offsetY + SLOT_SIZE / 2.0f) / SLOT_SIZE));

    // 3. 檢查計算出來的索引是否在合法的 7排 x 9列 範圍內 
    if (col >= 0 && col < 7 && row >= 0 && row < 9) {
        outCol = col;
        outRow = row;
        return true;
    }

    return false; 
}

// 輔助函式 
Brick::BrickType LevelManager::RollGuaranteedBrick(int currentRound) {
    // 固定生成的磚塊機率：普通方形50%、其餘根據正常比例分配
    std::uniform_int_distribution<> dis(1, 100);
    int roll = dis(gen);
    
    if (roll <= 50) return Brick::BrickType::SQUARE;       // 50% 
    if (roll <= 75) return Brick::BrickType::TRIANGLE;     // 25% (等比例分攤剩下50%)
    return RollSpecialBrick(currentRound);          // 25% (特殊磚塊)
}

Brick::BrickType LevelManager::RollRandomBrick(int currentRound) {
    // 隨機磚塊的內部機率分佈 (總和85)：普通35, 三角25, 其他特殊25
    std::uniform_int_distribution<> dis(1, 85);
    int roll = dis(gen);
    
    if (roll <= 35) return Brick::BrickType::SQUARE;       
    if (roll <= 60) return Brick::BrickType::TRIANGLE;     
    return RollSpecialBrick(currentRound);          
}

Brick::BrickType LevelManager::RollRandomProp() {
    int roll = rand() % 3; 
    switch (roll) {
        case 0: return Brick::BrickType::HLASER;
        case 1: return Brick::BrickType::VLASER;
        case 2: return Brick::BrickType::SCATTER;
        default: return Brick::BrickType::HLASER;
    }
}

Brick::BrickType LevelManager::RollSpecialBrick(int currentRound) {
    // 蒐集這個回合「合資格」的特殊磚塊（含上限、回合、奇偶限制）
    std::vector<Brick::BrickType> candidates;

    // GIANT 始終合格（沒有解鎖回合）
    // candidates.push_back(Brick::BrickType::GIANT);

    // 高密度：回合 ≥12 且 round%3==0，場上最多 3 個
    if (currentRound >= 12 && currentRound % 3 == 0
        && GetActiveBrickCount(Brick::BrickType::HIGH_DENSITY) < 3) {
        candidates.push_back(Brick::BrickType::HIGH_DENSITY);
    }

    // 連鎖：回合 ≥15，場上最多 5 個
    if (currentRound >= 15
        && GetActiveBrickCount(Brick::BrickType::LINKED) < 5) {
        candidates.push_back(Brick::BrickType::LINKED);
    }

    // 分裂：回合 ≥30
    if (currentRound >= 30) {
        candidates.push_back(Brick::BrickType::SPLIT);
    }

    // 尖刺：回合 ≥60 且偶數回合
    if (currentRound >= 60 && currentRound % 2 == 0) {
        candidates.push_back(Brick::BrickType::SPIKE);
    }

    // 厚底、隱形：回合 ≥80
    if (currentRound >= 80) {
        candidates.push_back(Brick::BrickType::HARDENED_BOTTOM);
        candidates.push_back(Brick::BrickType::INVISIBLE);
    }

    // 防護罩：回合 ≥90
    if (currentRound >= 90) {
        candidates.push_back(Brick::BrickType::SHIELDED);
    }

    // 下墜：回合 ≥100
    if (currentRound >= 100) {
        candidates.push_back(Brick::BrickType::DOUBLE_DROP);
    }

    if (candidates.empty()) return Brick::BrickType::SQUARE;
    return candidates[rand() % candidates.size()];
}

int LevelManager::GetActiveBrickCount(Brick::BrickType targetType) const {
    int count = 0;
    for (const auto& brick : m_Bricks) {
        if (brick != nullptr && brick->GetType() == targetType) {
            count++;
        }
    }
    return count;
}

bool LevelManager::CheckGameOver(float deathLineY) {
    for (auto& brick : m_Bricks) {
        // 如果這個物件已經被打死 (或不存在)，跳過
        if (!brick || brick->GetHp() <= 0) continue;

        if (brick->IsPlusItem()) continue; 

        // 如果磚塊的 Y 座標小於或等於死亡線，遊戲結束
        if (brick->m_Transform.translation.y <= deathLineY) {
            return true;
        }
    }
    return false; 
}

glm::vec2 LevelManager::GetPositionByColumn(int col, int row) {
    // 1. 計算 X 座標 (7個欄位，每個寬度 70，從 -210 開始)
    float startX = -210.0f;
    float spacingX = 70.0f;
    float x = startX + (col * spacingX);

    // 2. 計算 Y 座標 (頂部從 360 開始，row 越大代表越下面)
    float startY = 360.0f;
    float spacingY = 70.0f;
    float y = startY - (row * spacingY);

    return glm::vec2(x, y);
}

void LevelManager::SpawnProp(int col, PropType type) {
    // 道具也要從 Row 1 生成
    auto pos = GetPositionByColumn(col, 1);

    // 判斷要生成哪種道具
    if (type == PropType::AddBall) {
        
        auto plusItem = std::make_shared<Brick>(RESOURCE_DIR "/Image/PlusItem.png"); 
        plusItem->m_Transform.translation = pos;
        
        plusItem->SetPlusItem(true); 
        
        m_Bricks.push_back(plusItem); 
        
        m_Root.AddChild(plusItem);
        
    } 
    else if (type == PropType::Coin) {
        // auto coin = std::make_shared<Brick>(RESOURCE_DIR "/Image/Coin.png");
        // coin->SetCoinItem(true);  
        // m_Bricks.push_back(coin);
        // m_Root.AddChild(coin);
    }
}
void LevelManager::Draw() {
    // 畫出畫面上所有的磚塊
    for (auto& brick : m_Bricks) {
        if (brick) {
            brick->Draw(); 
        }
    }
}

void LevelManager::ProcessDoubleDrop() {
    // 先建一個「(row, col) → 是否被佔用」的表
    auto occupied = ComputeOccupied();   // 自動處理 GIANT 2×2

    for (auto& b : m_Bricks) {
        if (!b || b->GetType() != Brick::BrickType::DOUBLE_DROP) continue;

        int r = b->GetRow();
        int c = b->GetCol();

        // 正下方一格空就再下移一列
        if (!occupied.count({r + 1, c})) {
            occupied.erase({r, c});
            occupied.insert({r + 1, c});

            b->m_RowIndex += 1;
            auto pos = b->GetPosition();
            b->SetPosition(glm::vec2(pos.x, pos.y - 70.0f));   
            b->RefreshInvisibleVisual();
        }
    }
}

void LevelManager::AddLinked(int hp) {
    m_LinkedSharedHp += hp;
    // 同步所有 LINKED 顯示
    for (auto& b : m_Bricks) {
        if (b && b->GetType() == Brick::BrickType::LINKED) {
            b->SetMaxHp(m_LinkedSharedHp);     
            b->SetHp(m_LinkedSharedHp);
        }
    }
}

bool LevelManager::DamageLinked(int dmg) {
    m_LinkedSharedHp -= dmg;
    if (m_LinkedSharedHp < 0) m_LinkedSharedHp = 0;

    for (auto& b : m_Bricks) {
        if (b && b->GetType() == Brick::BrickType::LINKED) {
            b->SetHp(m_LinkedSharedHp);
        }
    }
    return m_LinkedSharedHp <= 0;
}

void LevelManager::HandleSplit(int newHp) {
    auto occupied = ComputeOccupied();   

    std::vector<std::pair<int, int>> emptySlots;
    for (int r = 1; r <= 5; ++r) {
        for (int c = 0; c < 7; ++c) {
            if (!occupied.count({r, c})) emptySlots.push_back({r, c});
        }
    }
    if (emptySlots.empty()) return;

    // 隨機選最多 2 個位置
    static std::mt19937 g(std::random_device{}());
    std::shuffle(emptySlots.begin(), emptySlots.end(), g);
    int spawnCount = std::min((int)emptySlots.size(), 2);

    for (int i = 0; i < spawnCount; ++i) {
        int newRow = emptySlots[i].first;
        int newCol = emptySlots[i].second;

        // 用 newHp 當 init round → referenceMax = newHp*2 → 子磚塊滿血 ratio=0.5 → 紫
        auto newBrick = std::make_shared<Brick>(Brick::BrickType::SQUARE, newHp, 0);
        newBrick->SetHp(std::max(1, newHp));
        newBrick->SetPosition(GetPositionByColumn(newCol, newRow));
        newBrick->SetRowCol(newRow, newCol);
        newBrick->m_GraceTimer = 0.3f;   // 0.3 秒穿透

        m_Bricks.push_back(newBrick);
        m_Root.AddChild(newBrick);
    }
}

void LevelManager::SpawnGiant(int col, int currentRound) {
    int row = 0;  // GIANT 暫時佔用 row 0,1（下一回合下移後變 row 1,2）

    auto brick = std::make_shared<Brick>(Brick::BrickType::GIANT, currentRound, 0);

    // 中心 = 左上格中心 + (35, -35) 偏移到 2×2 中央
    auto leftTopPos = GetPositionByColumn(col, row);
    brick->SetPosition(glm::vec2(leftTopPos.x + 35.0f, leftTopPos.y - 35.0f));
    brick->SetRowCol(row, col);   // 用左上角為錨點

    // GIANT 視覺要 ~132 px (33mm)，原圖 122 → scale 約 1.08
    brick->m_Transform.scale = glm::vec2(128.0f / 122.0f, 128.0f / 122.0f);

    m_Bricks.push_back(brick);
    m_Root.AddChild(brick);
}

std::set<std::pair<int, int>> LevelManager::ComputeOccupied() const {
    std::set<std::pair<int, int>> result;
    for (auto& b : m_Bricks) {
        if (!b) continue;
        int r = b->GetRow();
        int c = b->GetCol();
        if (b->GetType() == Brick::BrickType::GIANT) {
            // GIANT 佔 (r, c), (r, c+1), (r+1, c), (r+1, c+1)
            result.insert({r, c});
            result.insert({r, c + 1});
            result.insert({r + 1, c});
            result.insert({r + 1, c + 1});
        } else {
            result.insert({r, c});
        }
    }
    return result;
}

void LevelManager::StartLevel(int level) {
    Clear();                      // 清空場上磚塊
    m_CurrentLevel = level;
    m_IsLevelMode = true;
    m_LinkedSharedHp = 0;
    m_BossBrick = nullptr;
    LoadLevelData(level);
}

void LevelManager::LoadLevelData(int level) {
    // 之後 18 關的設計都在這個 switch 裡擴充
    switch (level) {
        case 1: {
                m_MaxRound = 15;       // 16 回合起停止生成
                m_VictoryType = 1;     // 場上清空算贏
                SpawnRowLevel(1);
                break;
            }
        case 2: {
            m_MaxRound = 12;
            m_VictoryType = 1;
            SpawnRowLevel(1);
            break;
        }
        case 3: { m_MaxRound = 15; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 4: { m_MaxRound = 10; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 5: { m_MaxRound = 10; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 6: { m_MaxRound = 15; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 7: { m_MaxRound = 15; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 8: { m_MaxRound = 12; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 9: { m_MaxRound = 10; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 10: {
            m_MaxRound = 20;
            m_VictoryType = 2;
            m_BossBrick = nullptr;   // round 20 才 spawn
            SpawnRowLevel(1);
            break;
        }

        case 11: { m_MaxRound = 30; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 12: { m_MaxRound = 30; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 13: { m_MaxRound = 30; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 14: { m_MaxRound = 30; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 15: {
            m_MaxRound = 9999;     // 沒有「回合限制」，撐到 boss 死為止
            m_VictoryType = 2;
            m_BossBrick = nullptr;
            m_Level15SpikeKilledThisRound = 0;   
            m_Level15NoKillRounds = 0;            
            m_LevelFailed = false;                
            SpawnRowLevel(1);
            break;
        }

        case 16: { m_MaxRound = 40; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 17: { m_MaxRound = 40; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 18: { m_MaxRound = 40; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 19: { m_MaxRound = 40; m_VictoryType = 1; SpawnRowLevel(1); break; }
        case 20: {
            m_MaxRound = 9999;
            m_VictoryType = 2;
            m_BossBrick = nullptr;
            SpawnRowLevel(1);   // round 1 就 spawn Boss
            break;
        }

        default: {
            // 未實作的關卡 fallback 成普通 3 回合清空
            m_MaxRound = 3;
            m_VictoryType = 1;
            SpawnRowLevel(level);
            break;
        }
    }
}

void LevelManager::SpawnRowLevel(int currentRound) {
    // 通用：超過 m_MaxRound 不再生成任何東西
    if (currentRound > m_MaxRound) return;

    switch (m_CurrentLevel) {
        case 1: SpawnLevel1Round(currentRound); break;
        case 2: SpawnLevel2Round(currentRound); break;
        case 3: SpawnLevel3Round(currentRound); break;
        case 4: SpawnLevel4Round(currentRound); break;
        case 5: SpawnLevel5Round(currentRound); break;
        case 6: SpawnLevel6Round(currentRound); break;
        case 7: SpawnLevel7Round(currentRound); break;
        case 8: SpawnLevel8Round(currentRound); break;
        case 9: SpawnLevel9Round(currentRound); break;

        case 10: SpawnLevel10Round(currentRound); break;

        case 11: SpawnLevel11Round(currentRound); break;
        case 12: SpawnLevel12Round(currentRound); break;
        case 13: SpawnLevel13Round(currentRound); break;
        case 14: SpawnLevel14Round(currentRound); break;

        case 15: SpawnLevel15Round(currentRound); break;

        case 16: SpawnLevel16Round(currentRound); break;
        case 17: SpawnLevel17Round(currentRound); break;
        case 18: SpawnLevel18Round(currentRound); break;
        case 19: SpawnLevel19Round(currentRound); break;

        case 20: SpawnLevel20Round(currentRound); break;

        default: {
            // fallback：隨機 3 個方塊
            std::vector<int> cols = {0, 1, 2, 3, 4, 5, 6};
            static std::mt19937 g(std::random_device{}());
            std::shuffle(cols.begin(), cols.end(), g);
            for (int i = 0; i < 3 && i < (int)cols.size(); ++i) {
                SpawnBrick(cols[i], Brick::BrickType::SQUARE, currentRound, 0);
            }
            break;
        }
    }
}

void LevelManager::SpawnLevel1Round(int round) {
    // 三角形方向：0=◣左下、1=◢右下、2=◥右上、3=◤左上
    auto T  = [&](int col, int o) { SpawnBrick(col, Brick::BrickType::TRIANGLE,     round, o); };
    auto SQ = [&](int col)        { SpawnBrick(col, Brick::BrickType::SQUARE,       round, 0); };
    auto HD = [&](int col)        { SpawnBrick(col, Brick::BrickType::HIGH_DENSITY, round, 0); };
    auto AB = [&](int col)        { SpawnBrick(col, Brick::BrickType::ADD_BALL,     0,     0); };
    auto CN = [&](int col)        { SpawnBrick(col, Brick::BrickType::COIN,         0,     0); };
    auto HL = [&](int col)        { SpawnBrick(col, Brick::BrickType::HLASER,       0,     0); };
    auto VL = [&](int col)        { SpawnBrick(col, Brick::BrickType::VLASER,       0,     0); };
    auto SC = [&](int col)        { SpawnBrick(col, Brick::BrickType::SCATTER,      0,     0); };
    auto G  = [&](int col)        { SpawnGiant(col, round); };

    switch (round) {
        case 1:  T(2, 2); AB(3); T(4, 3);                                    break;
        case 2:  CN(0); T(1, 2); SQ(2); AB(3); SQ(4); T(5, 3); CN(6);        break;
        case 3:  T(0, 2); SQ(1); SQ(2); AB(3); SQ(4); SQ(5); T(6, 3);        break;
        case 4:  SQ(0); CN(2); SQ(3); CN(4); SQ(6);                          break;
        case 5:  AB(0); T(1, 1); SQ(2); CN(3); SQ(4); T(5, 0); AB(6);        break;
        case 6:  HL(0); AB(1); T(2, 1); SC(3); T(4, 0); AB(5); HL(6);        break;
        case 7:  SQ(1); SQ(2); SQ(3); SQ(4); SQ(5);                          break;
        case 8:  AB(0); AB(1); AB(2); AB(3); AB(4); AB(5); AB(6);            break;
        case 9:  T(0, 2); G(1); G(4); T(6, 3);                               break;
        case 10: CN(0); HD(3); CN(6);                                        break;
        case 11: G(0); G(5);                                                 break;
        case 12: CN(2); SC(3); CN(4);                                        break;
        case 13: VL(0); T(1, 1); SQ(2); SQ(3); SQ(4); T(5, 0); VL(6);        break;
        case 14: T(2, 1); SQ(3); T(4, 0);                                    break;
        case 15: SQ(0); SQ(3); SQ(6);                                        break;
    }
}

void LevelManager::SpawnLevel2Round(int round) {
    auto SQ = [&](int col) { SpawnBrick(col, Brick::BrickType::SQUARE,   round, 0); };
    auto LK = [&](int col) { SpawnBrick(col, Brick::BrickType::LINKED,   round, 0); };
    auto AB = [&](int col) { SpawnBrick(col, Brick::BrickType::ADD_BALL, 0,     0); };
    auto CN = [&](int col) { SpawnBrick(col, Brick::BrickType::COIN,     0,     0); };

    // 4 種布局 ABCD 循環：1,5,9 → A；2,6,10 → B；3,7,11 → C；4,8,12 → D
    int phase = ((round - 1) % 4) + 1;

    switch (phase) {
        case 1: LK(0); LK(2); AB(3); LK(4); LK(6);                       break; // A
        case 2: LK(1); LK(2); AB(3); LK(4); LK(5);                       break; // B
        case 3: LK(0); SQ(1); SQ(2); AB(3); SQ(4); SQ(5); LK(6);         break; // C
        case 4: SQ(0); CN(2); AB(3); CN(4); SQ(6);                       break; // D
    }
}

void LevelManager::SpawnLevel3Round(int round) {
    auto SP = [&](int col) { SpawnBrick(col, Brick::BrickType::SPLIT,    round, 0); };
    auto AB = [&](int col) { SpawnBrick(col, Brick::BrickType::ADD_BALL, 0,     0); };
    auto CN = [&](int col) { SpawnBrick(col, Brick::BrickType::COIN,     0,     0); };
    auto G  = [&](int col) { SpawnGiant(col, round); };

    switch (round) {
        case 1:  G(0); AB(2); AB(3); AB(4); G(5);                   break; // GIANT 下半 (對 row1)
        case 2:  SP(2); SP(3);                                      break; // GIANT 上半，不重 spawn
        case 3:                                                     break;
        case 4:  SP(0); AB(2); AB(3); AB(4); SP(5); SP(6);          break;
        case 5:  CN(0); SP(3); SP(4); CN(5); SP(6);                 break;
        case 6:  SP(1); CN(2); CN(4); SP(5);                        break;
        case 7:  G(0); AB(2); AB(3); AB(4); G(5);                   break; // GIANT 下半
        case 8:  SP(2); SP(3); SP(4);                               break; // GIANT 上半
        case 9:  SP(1); SP(5);                                      break;
        case 10: SP(0); AB(2); AB(3); AB(4); SP(6);                 break;
        case 11: SP(1); SP(2); SP(4); SP(5);                        break;
        case 12: CN(1); CN(2); CN(4); CN(5);                        break;
        case 13: G(0); AB(2); AB(3); AB(4); G(5);                   break; // GIANT 下半
        case 14: SP(2); SP(4);                                      break; // GIANT 上半
        case 15: SP(1); SP(3); SP(5);                               break;
    }
}

void LevelManager::SpawnLevel4Round(int round) {
    auto SP = [&](int col) { SpawnBrick(col, Brick::BrickType::SPLIT,           round, 0); };
    auto HB = [&](int col) { SpawnBrick(col, Brick::BrickType::HARDENED_BOTTOM, round, 0); };
    auto AB = [&](int col) { SpawnBrick(col, Brick::BrickType::ADD_BALL,        0,     0); };
    auto CN = [&](int col) { SpawnBrick(col, Brick::BrickType::COIN,            0,     0); };
    auto G  = [&](int col) { SpawnGiant(col, round); };

    switch (round) {
        case 1:  G(1); CN(3); HB(4); HB(5); CN(6);                  break; // GIANT 下半
        case 2:  AB(3); AB(4);                                      break; // GIANT 上半
        case 3:  HB(0); G(2); CN(4); HB(5); HB(6);                  break; // GIANT 下半
        case 4:  AB(4); AB(5);                                      break; // GIANT 上半
        case 5:  HB(0); HB(1); G(3); HB(6);                         break; // GIANT 下半
        case 6:  AB(1); AB(2);                                      break; // GIANT 上半
        case 7:  CN(0); HB(1); HB(2); CN(3); G(4);                  break; // GIANT 下半
        case 8:  CN(1); AB(2); AB(3);                               break; // GIANT 上半
        case 9:  CN(0); SP(1); AB(2); SP(3); AB(4); SP(5); CN(6);   break;
        case 10: SP(0); SP(2); SP(4); SP(6);                        break;
    }
}

void LevelManager::SpawnLevel5Round(int round) {
    auto SK = [&](int col) { SpawnBrick(col, Brick::BrickType::SPIKE,    round, 0); };
    auto HL = [&](int col) { SpawnBrick(col, Brick::BrickType::HLASER,   0,     0); };
    auto AB = [&](int col) { SpawnBrick(col, Brick::BrickType::ADD_BALL, 0,     0); };
    auto G  = [&](int col) { SpawnGiant(col, round); };

    switch (round) {
        case 1:  G(0); AB(2);                                  break; // 左GIANT下半
        case 2:  HL(2); SK(3); AB(4); G(5);                    break; // 左上半 / 右GIANT下半
        case 3:  G(0); AB(2); SK(4);                           break; // 左GIANT下半 / 右上半
        case 4:  SK(2); AB(4); G(5);                           break; // 左上半 / 右GIANT下半
        case 5:  G(0); AB(2); SK(3); HL(4);                    break; // 左GIANT下半 / 右上半
        case 6:  SK(2); AB(4); G(5);                           break; // 左上半 / 右GIANT下半
        case 7:  G(0); AB(2); SK(4);                           break; // 左GIANT下半 / 右上半
        case 8:  HL(2); SK(3); AB(4); G(5);                    break; // 左上半 / 右GIANT下半
        case 9:  AB(2); HL(4);                                 break; // 左上半 / 右上半
        case 10: SK(0); SK(1); HL(3); AB(4); SK(5); SK(6);     break;
    }
}

void LevelManager::SpawnLevel6Round(int round) {
    auto SQ = [&](int col)         { SpawnBrick(col, Brick::BrickType::SQUARE,   round, 0); };
    auto SP = [&](int col)         { SpawnBrick(col, Brick::BrickType::SPLIT,    round, 0); };
    auto SH = [&](int col)         { SpawnBrick(col, Brick::BrickType::SHIELDED, round, 0); };
    auto T  = [&](int col, int o)  { SpawnBrick(col, Brick::BrickType::TRIANGLE, round, o); };
    auto AB = [&](int col)         { SpawnBrick(col, Brick::BrickType::ADD_BALL, 0,     0); };
    auto CN = [&](int col)         { SpawnBrick(col, Brick::BrickType::COIN,     0,     0); };
    auto SC = [&](int col)         { SpawnBrick(col, Brick::BrickType::SCATTER,  0,     0); };
    auto G  = [&](int col)         { SpawnGiant(col, round); };

    switch (round) {
        case 1:  G(0); AB(2); SH(3); AB(4); T(5, 2); T(6, 3);                       break; // 左GIANT 下半
        case 2:  AB(2); SP(3); SH(4); T(5, 1); T(6, 0);                             break; // 左GIANT 上半
        case 3:  T(0, 1); T(1, 0); SQ(3); SC(4); SH(5); SH(6);                      break;
        case 4:  T(0, 2); T(1, 3); AB(2); AB(3); AB(4); SP(5); SP(6);               break;
        case 5:  CN(0); SP(3); SP(4); T(5, 2); T(6, 3);                             break;
        case 6:  SP(1); CN(2); SH(3); CN(4); T(5, 1); T(6, 0);                      break;
        case 7:  T(0, 2); T(1, 3); AB(2); AB(3); AB(4); G(5);                       break; // 右GIANT 下半
        case 8:  T(0, 1); T(1, 0); SH(2); SH(3); SP(4);                             break; // 右GIANT 上半
        case 9:  SP(1); SH(5); SH(6);                                               break;
        case 10: SH(0); SH(1); AB(2); AB(3); AB(4); SP(6);                          break;
        case 11: SP(1); SQ(2); SQ(4); SP(5);                                        break;
        case 12: AB(0); CN(1); CN(2); SH(3); CN(4); CN(5); AB(6);                   break;
        case 13: T(0, 2); T(1, 3); AB(2); SH(3); AB(4); T(5, 2); T(6, 3);           break;
        case 14: T(0, 1); T(1, 0); SP(2); SH(4); T(5, 1); T(6, 0);                  break;
        case 15: SH(1); SP(3); SP(5); SH(6);                                        break;
    }
}

void LevelManager::SpawnLevel7Round(int round) {
    auto SQ = [&](int col)         { SpawnBrick(col, Brick::BrickType::SQUARE,    round, 0); };
    auto IV = [&](int col)         { SpawnBrick(col, Brick::BrickType::INVISIBLE, round, 0); };
    auto LK = [&](int col)         { SpawnBrick(col, Brick::BrickType::LINKED,    round, 0); };
    auto SK = [&](int col)         { SpawnBrick(col, Brick::BrickType::SPIKE,     round, 0); };
    auto T  = [&](int col, int o)  { SpawnBrick(col, Brick::BrickType::TRIANGLE,  round, o); };
    auto AB = [&](int col)         { SpawnBrick(col, Brick::BrickType::ADD_BALL,  0,     0); };
    auto CN = [&](int col)         { SpawnBrick(col, Brick::BrickType::COIN,      0,     0); };
    auto HL = [&](int col)         { SpawnBrick(col, Brick::BrickType::HLASER,    0,     0); };
    auto VL = [&](int col)         { SpawnBrick(col, Brick::BrickType::VLASER,    0,     0); };
    auto G  = [&](int col)         { SpawnGiant(col, round); };

    switch (round) {
        case 1:  LK(0); CN(1); SQ(2); LK(3); HL(4); SK(5); AB(6);                   break;
        case 2:  LK(1); SK(2); SK(3); HL(4); AB(5); LK(6);                          break;
        case 3:  IV(0); CN(1); CN(2); CN(3); AB(4); SQ(5); SQ(6);                   break;
        case 4:  LK(1); LK(2); AB(3); IV(4); CN(5); CN(6);                          break;
        case 5:  CN(0); CN(1); AB(2); SQ(3); IV(5);                                 break;
        case 6:  SQ(0); AB(1); T(2, 2); IV(4);                                      break;
        case 7:  AB(0); G(3); SQ(5); IV(6);                                         break; // GIANT 下半
        case 8:  AB(1); IV(2); SK(5); CN(6);                                        break; // GIANT 上半
        case 9:  SK(0); IV(1); AB(2); LK(4); LK(5); CN(6);                          break;
        case 10: SQ(0); SQ(2); AB(3); SQ(4); T(5, 3); SQ(6);                        break;
        case 11: SK(1); IV(2); IV(3); AB(4); SQ(5); IV(6);                          break;
        case 12: SQ(0); LK(1); LK(2); IV(3); AB(5); CN(6);                          break;
        case 13: VL(0); T(1, 3); SQ(2); IV(3); AB(6);                               break;
        case 14: LK(0); LK(1); LK(2); LK(3); AB(5);                                 break;
        case 15: IV(0); IV(1); IV(3); AB(4); IV(5); IV(6);                          break;
    }
}

void LevelManager::SpawnLevel8Round(int round) {
    auto HD = [&](int col) { SpawnBrick(col, Brick::BrickType::HIGH_DENSITY, round, 0); };
    auto LK = [&](int col) { SpawnBrick(col, Brick::BrickType::LINKED,       round, 0); };
    auto SP = [&](int col) { SpawnBrick(col, Brick::BrickType::SPLIT,        round, 0); };
    auto AB = [&](int col) { SpawnBrick(col, Brick::BrickType::ADD_BALL,     0,     0); };
    auto CN = [&](int col) { SpawnBrick(col, Brick::BrickType::COIN,         0,     0); };

    switch (round) {
        case 1:  HD(0); AB(1); HD(2); AB(3); HD(4); AB(5); HD(6);                   break;
        case 2:  CN(0); HD(2); HD(3); HD(4); CN(6);                                 break;
        case 3:  SP(0); CN(1); SP(2); SP(4); CN(5); SP(6);                          break;
        case 4:  HD(0); AB(1); HD(2); AB(3); HD(4); AB(5); HD(6);                   break;
        case 5:  CN(0); HD(2); HD(3); HD(4); CN(6);                                 break;
        case 6:  SP(0); CN(1); SP(2); SP(4); CN(5); SP(6);                          break;
        case 7:  HD(0); LK(1); LK(2); AB(3); LK(4); LK(5); HD(6);                   break;
        case 8:  SP(0); SP(2); AB(3); SP(4); SP(6);                                 break;
        case 9:  LK(1); HD(2); AB(3); HD(4); LK(5);                                 break;
        case 10: HD(0); LK(1); LK(2); AB(3); LK(4); LK(5); HD(6);                   break;
        case 11: SP(0); SP(2); AB(3); SP(4); SP(6);                                 break;
        case 12: LK(1); HD(2); AB(3); HD(4); LK(5);                                 break;
    }
}

void LevelManager::SpawnLevel9Round(int round) {
    auto SP = [&](int col) { SpawnBrick(col, Brick::BrickType::SPLIT,       round, 0); };
    auto DD = [&](int col) { SpawnBrick(col, Brick::BrickType::DOUBLE_DROP, round, 0); };
    auto AB = [&](int col) { SpawnBrick(col, Brick::BrickType::ADD_BALL,    0,     0); };
    auto CN = [&](int col) { SpawnBrick(col, Brick::BrickType::COIN,        0,     0); };

    switch (round) {
        case 1:  CN(0); DD(1); DD(2); AB(3); DD(4); DD(5); CN(6);                   break;
        case 2:  DD(0); SP(2); CN(3); SP(4); DD(6);                                 break;
        case 3:  SP(0); SP(1); AB(2); AB(4); SP(5); SP(6);                          break;
        case 4:  CN(0); DD(1); DD(2); AB(3); DD(4); DD(5); CN(6);                   break;
        case 5:  DD(0); SP(2); CN(3); SP(4); DD(6);                                 break;
        case 6:  SP(0); SP(1); AB(2); AB(4); SP(5); SP(6);                          break;
        case 7:  CN(0); DD(1); DD(2); AB(3); DD(4); DD(5); CN(6);                   break;
        case 8:  DD(0); SP(2); DD(3); SP(4); DD(6);                                 break;
        case 9:  SP(0); SP(1); AB(2); AB(3); AB(4); SP(5); SP(6);                   break;
        case 10: DD(1); DD(2); DD(3); DD(4); DD(5);                                 break;
    }
}

void LevelManager::SpawnLevel10Round(int round) {
    auto SQ = [&](int c)        { SpawnBrick(c, Brick::BrickType::SQUARE,          round, 0); };
    auto SK = [&](int c)        { SpawnBrick(c, Brick::BrickType::SPIKE,           round, 0); };
    auto SH = [&](int c)        { SpawnBrick(c, Brick::BrickType::SHIELDED,        round, 0); };
    auto IV = [&](int c)        { SpawnBrick(c, Brick::BrickType::INVISIBLE,       round, 0); };
    auto SP = [&](int c)        { SpawnBrick(c, Brick::BrickType::SPLIT,           round, 0); };
    auto LK = [&](int c)        { SpawnBrick(c, Brick::BrickType::LINKED,          round, 0); };
    auto HB = [&](int c)        { SpawnBrick(c, Brick::BrickType::HARDENED_BOTTOM, round, 0); };
    auto T  = [&](int c, int o) { SpawnBrick(c, Brick::BrickType::TRIANGLE,        round, o); };
    auto AB = [&](int c)        { SpawnBrick(c, Brick::BrickType::ADD_BALL,        0,     0); };
    auto CN = [&](int c)        { SpawnBrick(c, Brick::BrickType::COIN,            0,     0); };
    auto HL = [&](int c)        { SpawnBrick(c, Brick::BrickType::HLASER,          0,     0); };

    // P1: round 1,4,7,10,13
    auto P1 = [&]() { AB(0); SQ(1); SK(2); SQ(3); HL(4); CN(5); SH(6); };
    // P2: round 2,5,8,11,14
    auto P2 = [&]() { SH(0); CN(1); HL(2); SQ(3); SK(4); SQ(5); AB(6); };
    // P3: round 3,6,9,12
    auto P3 = [&]() { IV(0); SH(2); AB(3); IV(4); };

    switch (round) {
        case 1: case 4: case 7: case 10: case 13: P1(); break;
        case 2: case 5: case 8: case 11: case 14: P2(); break;
        case 3: case 6: case 9: case 12:          P3(); break;
        case 15: T(0, 3); AB(1); CN(3); AB(5); T(6, 2);                            break;
        case 16: CN(1); LK(2); LK(3); LK(4); CN(5);                                break;
        case 17: LK(0); HB(1); CN(2); AB(3); CN(4); HB(5); LK(6);                  break;
        case 18: AB(0); T(2, 2); T(4, 3); AB(6);                                   break;
        case 19: T(1, 2); SP(2); AB(3); SP(4); T(5, 3);                            break;
        case 20: {
            T(0, 2); SP(1); SP(5); T(6, 3);
            // Boss 在 col 3 (= 表格第 4 排)
            auto pos = GetPositionByColumn(3, 1);
            auto boss = std::make_shared<Brick>(Brick::BrickType::LEVEL10_BOSS, 1, 0);
            boss->SetPosition(pos);
            boss->SetRowCol(1, 3);
            m_Bricks.push_back(boss);
            m_Root.AddChild(boss);
            m_BossBrick = boss;
            // Boss 出現後立刻補滿 4 個 SPLIT
            Level10MaintainSplits();
            break;
        }
    }
}

void LevelManager::Level10TeleportBoss() {
    if (!m_BossBrick) return;

    auto occupied = ComputeOccupied();
    occupied.erase({m_BossBrick->GetRow(), m_BossBrick->GetCol()});

    // 第 2-7 列 → row 1-6
    std::vector<std::pair<int, int>> emptyForBoss;
    for (int r = 1; r <= 6; ++r) {
        for (int c = 0; c < 7; ++c) {
            if (!occupied.count({r, c})) emptyForBoss.push_back({r, c});
        }
    }
    if (emptyForBoss.empty()) return;

    static std::mt19937 g(std::random_device{}());
    auto p = emptyForBoss[std::uniform_int_distribution<int>(0, emptyForBoss.size() - 1)(g)];
    
    // 從佔用表移除 Boss 舊位置、加入新位置
    occupied.insert({p.first, p.second});
    
    m_BossBrick->SetPosition(GetPositionByColumn(p.second, p.first));
    m_BossBrick->SetRowCol(p.first, p.second);

    // 順便在第 2-5 列空位 spawn 4 個 SPLIT（HP=20）
    std::vector<std::pair<int, int>> emptyForSplit;
    for (int r = 1; r <= 4; ++r) {
        for (int c = 0; c < 7; ++c) {
            if (!occupied.count({r, c})) emptyForSplit.push_back({r, c});
        }
    }
    std::shuffle(emptyForSplit.begin(), emptyForSplit.end(), g);
    int spawnCount = std::min((int)emptyForSplit.size(), 4);

    for (int i = 0; i < spawnCount; ++i) {
        auto brick = std::make_shared<Brick>(Brick::BrickType::SPLIT, 1, 0);
        brick->SetHp(20);
        brick->SetPosition(GetPositionByColumn(emptyForSplit[i].second, emptyForSplit[i].first));
        brick->SetRowCol(emptyForSplit[i].first, emptyForSplit[i].second);
        m_Bricks.push_back(brick);
        m_Root.AddChild(brick);
    }
}

void LevelManager::Level10MaintainSplits() {
    if (!m_BossBrick) return;

    // 確認 Boss 還活著
    bool bossAlive = false;
    for (auto& b : m_Bricks) {
        if (b == m_BossBrick) { bossAlive = true; break; }
    }
    if (!bossAlive) { m_BossBrick = nullptr; return; }

    // 只數「Boss 守衛」這種 SPLIT，分裂出來的新方塊不算
    int currentGuards = 0;
    for (auto& b : m_Bricks) {
        if (b && b->m_IsBossGuard) currentGuards++;
    }
    int need = 4 - currentGuards;
    if (need <= 0) return;

    auto occupied = ComputeOccupied();
    std::vector<std::pair<int, int>> empty;
    for (int r = 1; r <= 4; ++r) {
        for (int c = 0; c < 7; ++c) {
            if (!occupied.count({r, c})) empty.push_back({r, c});
        }
    }
    if (empty.empty()) return;

    static std::mt19937 g(std::random_device{}());
    std::shuffle(empty.begin(), empty.end(), g);
    int spawnCount = std::min(need, (int)empty.size());

    for (int i = 0; i < spawnCount; ++i) {
        auto brick = std::make_shared<Brick>(Brick::BrickType::SPLIT, 1, 0);
        brick->SetHp(20);
        brick->m_IsBossGuard = true;     
        brick->SetPosition(GetPositionByColumn(empty[i].second, empty[i].first));
        brick->SetRowCol(empty[i].first, empty[i].second);
        m_Bricks.push_back(brick);
        m_Root.AddChild(brick);
    }
}

void LevelManager::OnLevelRoundEnd(int currentRound) {
    if (m_CurrentLevel == 15) {
        // 只有 Boss 在場時才計算失敗條件
        bool bossAlive = false;
        if (m_BossBrick) {
            for (auto& b : m_Bricks) {
                if (b == m_BossBrick) { bossAlive = true; break; }
            }
        }

        if (bossAlive) {
            if (m_Level15SpikeKilledThisRound == 0) {
                m_Level15NoKillRounds++;
                if (m_Level15NoKillRounds >= 3) {
                    m_LevelFailed = true;
                }
            } else {
                m_Level15NoKillRounds = 0;
            }
        }
        m_Level15SpikeKilledThisRound = 0;

        Level15OnRoundStart();
    }

    if (m_CurrentLevel == 20) {
        Level20OnRoundStart(currentRound);
    }
}

void LevelManager::SpawnLevel11Round(int round) {
    auto SQ = [&](int c)        { SpawnBrick(c, Brick::BrickType::SQUARE,           round, 0); };
    auto HD = [&](int c)        { SpawnBrick(c, Brick::BrickType::HIGH_DENSITY,     round, 0); };
    auto IV = [&](int c)        { SpawnBrick(c, Brick::BrickType::INVISIBLE,        round, 0); };
    auto LK = [&](int c)        { SpawnBrick(c, Brick::BrickType::LINKED,           round, 0); };
    auto HB = [&](int c)        { SpawnBrick(c, Brick::BrickType::HARDENED_BOTTOM,  round, 0); };
    auto T  = [&](int c, int o) { SpawnBrick(c, Brick::BrickType::TRIANGLE,         round, o); };
    auto AB = [&](int c)        { SpawnBrick(c, Brick::BrickType::ADD_BALL,         0, 0); };
    auto CN = [&](int c)        { SpawnBrick(c, Brick::BrickType::COIN,             0, 0); };
    auto HL = [&](int c)        { SpawnBrick(c, Brick::BrickType::HLASER,           0, 0); };
    auto VL = [&](int c)        { SpawnBrick(c, Brick::BrickType::VLASER,           0, 0); };
    auto SC = [&](int c)        { SpawnBrick(c, Brick::BrickType::SCATTER,          0, 0); };

    if (round <= 20) {
        switch (round % 5) {
            case 1: AB(1); SQ(3); CN(5);                                            break;
            case 2: SQ(0); HD(1); AB(2); T(3, 3); SC(5); SQ(6);                     break;
            case 3: IV(0); T(2, 1); AB(3); T(4, 3); IV(6);                          break;
            case 4: SQ(0); SC(1); T(3, 1); AB(4); HD(5); SQ(6);                     break;
            case 0: CN(1); SQ(3); AB(5);                                            break;
        }
        return;
    }

    switch (round) {
        case 21: LK(0); CN(1); LK(2); LK(4); AB(5); LK(6);                          break;
        case 22: SQ(0); HL(1); SQ(2); AB(4); HB(5); HB(6);                          break;
        case 23: VL(1); AB(3); VL(5);                                               break;
        case 24: HB(0); HB(1); AB(2); SQ(4); HL(5); SQ(6);                          break;
        case 25: LK(0); AB(1); LK(2); LK(4); CN(5); LK(6);                          break;
        case 26: LK(0); AB(1); LK(2); LK(4); CN(5); LK(6);                          break;
        case 27: HB(0); HB(1); AB(2); SQ(4); HL(5); SQ(6);                          break;
        case 28: VL(1); AB(3); VL(5);                                               break;
        case 29: SQ(0); HL(1); SQ(2); AB(4); HB(5); HB(6);                          break;
        case 30: LK(0); CN(1); LK(2); LK(4); AB(5); LK(6);                          break;
    }
}

void LevelManager::SpawnLevel12Round(int round) {
    auto HD = [&](int c)        { SpawnBrick(c, Brick::BrickType::HIGH_DENSITY,     round, 0); };
    auto IV = [&](int c)        { SpawnBrick(c, Brick::BrickType::INVISIBLE,        round, 0); };
    auto SK = [&](int c)        { SpawnBrick(c, Brick::BrickType::SPIKE,            round, 0); };
    auto LK = [&](int c)        { SpawnBrick(c, Brick::BrickType::LINKED,           round, 0); };
    auto HB = [&](int c)        { SpawnBrick(c, Brick::BrickType::HARDENED_BOTTOM,  round, 0); };
    auto T  = [&](int c, int o) { SpawnBrick(c, Brick::BrickType::TRIANGLE,         round, o); };
    auto AB = [&](int c)        { SpawnBrick(c, Brick::BrickType::ADD_BALL,         0, 0); };
    auto CN = [&](int c)        { SpawnBrick(c, Brick::BrickType::COIN,             0, 0); };
    auto SC = [&](int c)        { SpawnBrick(c, Brick::BrickType::SCATTER,          0, 0); };
    auto G  = [&](int c)        { SpawnGiant(c, round); };

    if (round <= 15) {
        // 右 GIANT (col 5) 在 mod 1 spawn；左 GIANT (col 0) 在 mod 4 spawn
        switch (round % 5) {
            case 1: HB(0); HB(1); CN(2); AB(3); G(5);                               break; // 右 GIANT 下半
            case 2: AB(1); LK(2); LK(3); CN(4);                                     break; // 右 GIANT 上半
            case 3: T(0, 3); CN(1); HD(3); AB(5); T(6, 1);                          break;
            case 4: G(0); AB(2); LK(3); LK(4); CN(5);                               break; // 左 GIANT 下半
            case 0: CN(3); AB(4); HB(5); HB(6);                                     break; // 左 GIANT 上半
        }
        return;
    }

    // round 16-30: 雙 GIANT 在 mod 1, 4 spawn
    switch (round % 5) {
        case 1: G(0); G(5); AB(3);                                                  break; // 雙 GIANT 下半
        case 2: /* 雙 GIANT 上半，不重 spawn */                                       break;
        case 3: IV(0); SK(1); AB(3); SK(5); IV(6);                                  break;
        case 4: G(0); G(5); CN(2); SC(3); AB(4);                                    break; // 雙 GIANT 下半
        case 0: AB(3);                                                              break; // 雙 GIANT 上半
    }
}

void LevelManager::SpawnLevel13Round(int round) {
    auto SQ = [&](int c)        { SpawnBrick(c, Brick::BrickType::SQUARE,           round, 0); };
    auto HD = [&](int c)        { SpawnBrick(c, Brick::BrickType::HIGH_DENSITY,     round, 0); };
    auto IV = [&](int c)        { SpawnBrick(c, Brick::BrickType::INVISIBLE,        round, 0); };
    auto SP = [&](int c)        { SpawnBrick(c, Brick::BrickType::SPLIT,            round, 0); };
    auto LK = [&](int c)        { SpawnBrick(c, Brick::BrickType::LINKED,           round, 0); };
    auto DD = [&](int c)        { SpawnBrick(c, Brick::BrickType::DOUBLE_DROP,      round, 0); };
    auto HB = [&](int c)        { SpawnBrick(c, Brick::BrickType::HARDENED_BOTTOM,  round, 0); };
    auto SK = [&](int c)        { SpawnBrick(c, Brick::BrickType::SPIKE,            round, 0); };
    auto T  = [&](int c, int o) { SpawnBrick(c, Brick::BrickType::TRIANGLE,         round, o); };
    auto AB = [&](int c)        { SpawnBrick(c, Brick::BrickType::ADD_BALL,         0, 0); };
    auto CN = [&](int c)        { SpawnBrick(c, Brick::BrickType::COIN,             0, 0); };
    auto HL = [&](int c)        { SpawnBrick(c, Brick::BrickType::HLASER,           0, 0); };
    auto SC = [&](int c)        { SpawnBrick(c, Brick::BrickType::SCATTER,          0, 0); };

    if (round <= 9) {
        switch (round % 3) {
            case 1: AB(0); SQ(1); T(2, 3); AB(3); T(4, 2); SQ(5); AB(6);            break;
            case 2: LK(0); LK(2); IV(3); LK(4); LK(6);                              break;
            case 0: CN(0); T(2, 1); HD(3); T(4, 0); CN(6);                          break;
        }
        return;
    }

    if (round <= 18) {
        switch (round % 3) {
            case 1: AB(2); SP(3); AB(4);                                            break;
            case 2: DD(0); DD(1); CN(2); HD(3); CN(4); DD(5); DD(6);                break;
            case 0: SP(2); AB(3); SP(4);                                            break;
        }
        return;
    }

    if (round <= 27) {
        switch (round % 3) {
            case 1: HB(0); HB(1); HB(2); SC(3); HB(4); HB(5); HB(6);                break;
            case 2: HL(0); SK(1); SK(2); SK(3); SK(4); SK(5); HL(6);                break;
            case 0: HB(0); AB(2); AB(3); AB(4); HB(6);                              break;
        }
        return;
    }

    switch (round) {
        case 28: DD(0); DD(1); AB(2); DD(5); DD(6);                                 break;
        case 29: HD(2); AB(3); HD(4);                                               break;
        case 30: DD(0); DD(1); AB(4); DD(5); DD(6);                                 break;
    }
}

void LevelManager::SpawnLevel14Round(int round) {
    auto SQ = [&](int c) { SpawnBrick(c, Brick::BrickType::SQUARE,           round, 0); };
    auto IV = [&](int c) { SpawnBrick(c, Brick::BrickType::INVISIBLE,        round, 0); };
    auto SP = [&](int c) { SpawnBrick(c, Brick::BrickType::SPLIT,            round, 0); };
    auto LK = [&](int c) { SpawnBrick(c, Brick::BrickType::LINKED,           round, 0); };
    auto HB = [&](int c) { SpawnBrick(c, Brick::BrickType::HARDENED_BOTTOM,  round, 0); };
    auto AB = [&](int c) { SpawnBrick(c, Brick::BrickType::ADD_BALL,         0, 0); };
    auto SC = [&](int c) { SpawnBrick(c, Brick::BrickType::SCATTER,          0, 0); };

    auto HBRow1 = [&]() { HB(0); HB(1); SC(2); HB(3); SC(4); HB(5); HB(6); };
    auto HBRow2 = [&]() { HB(0); HB(1); HB(2); SC(3); HB(4); HB(5); HB(6); };

    switch (round) {
        case 1:  AB(0); IV(1); SQ(2); AB(3); LK(4); LK(5); AB(6);                   break;
        case 2:  LK(1); IV(3); SQ(4); SQ(6);                                        break;
        case 3:  HBRow1();                                                          break;
        case 4:  AB(0); IV(2); AB(3); SQ(4); IV(5); AB(6);                          break;
        case 5:  SQ(1); SQ(2); IV(3);                                               break;
        case 6:  HBRow2();                                                          break;
        case 7:  AB(0); SP(1); IV(2); AB(3); SP(5); AB(6);                          break;
        case 8:  SQ(1); LK(2); SQ(3); LK(4); IV(5); SP(6);                          break;
        case 9:  HBRow1();                                                          break;
        case 10: AB(0); SQ(1); AB(3); SP(4); IV(5); AB(6);                          break;
        case 11: IV(0); IV(1); SP(2); SP(3); SQ(5);                                 break;
        case 12: HBRow2();                                                          break;
        case 13: AB(0); IV(1); LK(2); AB(3); LK(4); SP(5); AB(6);                   break;
        case 14: SQ(1); SP(3); SQ(4); SP(5);                                        break;
        case 15: HBRow1();                                                          break;
        case 16: AB(0); SP(1); SP(2); AB(3); LK(4); AB(6);                          break;
        case 17: LK(1); LK(2); IV(3); SQ(4); SP(5);                                 break;
        case 18: HBRow2();                                                          break;
        case 19: AB(0); SP(1); LK(2); AB(3); SP(4); AB(6);                          break;
        case 20: IV(1); LK(2); SQ(3); SP(4); LK(5); IV(6);                          break;
        case 21: HBRow1();                                                          break;
        case 22: AB(0); IV(2); AB(3); AB(6);                                        break;
        case 23: SQ(2); SQ(4);                                                      break;
        case 24: HBRow2();                                                          break;
        case 25: AB(0); LK(1); LK(2); AB(3); SP(4); IV(5); AB(6);                   break;
        case 26: SQ(0); SP(2); IV(3); SP(4); SQ(6);                                 break;
        case 27: HBRow1();                                                          break;
        case 28: AB(0); IV(1); SQ(2); AB(3); IV(5); AB(6);                          break;
        case 29: SQ(1); IV(2);                                                      break;
        case 30: HBRow2();                                                          break;
    }
}

void LevelManager::SpawnLevel15Round(int round) {
    auto SQ = [&](int c)        { SpawnBrick(c, Brick::BrickType::SQUARE,          round, 0); };
    auto HD = [&](int c)        { SpawnBrick(c, Brick::BrickType::HIGH_DENSITY,    round, 0); };
    auto IV = [&](int c)        { SpawnBrick(c, Brick::BrickType::INVISIBLE,       round, 0); };
    auto SH = [&](int c)        { SpawnBrick(c, Brick::BrickType::SHIELDED,        round, 0); };
    auto HB = [&](int c)        { SpawnBrick(c, Brick::BrickType::HARDENED_BOTTOM, round, 0); };
    auto SP = [&](int c)        { SpawnBrick(c, Brick::BrickType::SPLIT,           round, 0); };
    auto LK = [&](int c)        { SpawnBrick(c, Brick::BrickType::LINKED,          round, 0); };
    auto T  = [&](int c, int o) { SpawnBrick(c, Brick::BrickType::TRIANGLE,        round, o); };
    auto AB = [&](int c)        { SpawnBrick(c, Brick::BrickType::ADD_BALL,        0,     0); };
    auto CN = [&](int c)        { SpawnBrick(c, Brick::BrickType::COIN,            0,     0); };
    auto VL = [&](int c)        { SpawnBrick(c, Brick::BrickType::VLASER,          0,     0); };
    auto SC = [&](int c)        { SpawnBrick(c, Brick::BrickType::SCATTER,         0,     0); };

    // round 31+: 第一列持續金幣
    if (round >= 31) {
        CN(1); CN(5);
        return;
    }

    // round 25-30: 固定 spawn
    if (round >= 25) {
        switch (round) {
            case 25: AB(0); SQ(2); HD(3); SQ(4); AB(6);                                 break;
            case 26: T(0, 3); AB(1); CN(2); SC(3); CN(4); AB(5); T(6, 2);               break;
            case 27: LK(1); LK(2); LK(4); LK(5);                                        break;
            case 28: SP(0); HD(1); SC(3); HD(5); SP(6);                                 break;
            case 29: CN(0); CN(1); AB(2); AB(4); CN(5); CN(6);                          break;
            case 30: {
                // Boss spawn 在第 2 列第 4 排 (row 1, col 3)
                auto pos = GetPositionByColumn(3, 1);
                auto boss = std::make_shared<Brick>(Brick::BrickType::LEVEL15_BOSS, 1, 0);
                boss->SetPosition(pos);
                boss->SetRowCol(1, 3);
                m_Bricks.push_back(boss);
                m_Root.AddChild(boss);
                m_BossBrick = boss;
                // Boss 出現後立刻觸發第一次 SPIKE 鋪設
                Level15OnRoundStart();
                break;
            }
        }
        return;
    }

    // round 1-24: 4 回合循環
    int phase = round % 4;
    switch (phase) {
        case 1: HB(0); T(2, 3); AB(3); T(4, 2); HB(6);                                  break;
        case 2: AB(0); T(1, 1); CN(2); IV(3); CN(4); T(5, 0); AB(6);                    break;
        case 3: SQ(0); SH(1); HD(3); SH(5); SQ(6);                                      break;
        case 0: VL(1); IV(2); AB(3); IV(4); VL(5);                                      break;
    }
}

void LevelManager::Level15SpawnSpikes(int count) {
    auto occupied = ComputeOccupied();
    std::vector<std::pair<int, int>> empty;
    for (int r = 1; r <= 6; ++r) {   // 第 2-7 列
        for (int c = 0; c < 7; ++c) {
            if (!occupied.count({r, c})) empty.push_back({r, c});
        }
    }
    if (empty.empty()) return;

    static std::mt19937 g(std::random_device{}());
    std::shuffle(empty.begin(), empty.end(), g);
    int spawn = std::min(count, (int)empty.size());

    for (int i = 0; i < spawn; ++i) {
        auto brick = std::make_shared<Brick>(Brick::BrickType::SPIKE, 1, 0);
        brick->SetHp(15);
        brick->SetPosition(GetPositionByColumn(empty[i].second, empty[i].first));
        brick->SetRowCol(empty[i].first, empty[i].second);
        m_Bricks.push_back(brick);
        m_Root.AddChild(brick);
    }
}

void LevelManager::Level15OnRoundStart() {
    if (!m_BossBrick) return;

    // Boss alive check
    bool bossAlive = false;
    for (auto& b : m_Bricks) {
        if (b == m_BossBrick) { bossAlive = true; break; }
    }
    if (!bossAlive) { m_BossBrick = nullptr; return; }

    // 清除所有 SPIKE
    for (auto it = m_Bricks.begin(); it != m_Bricks.end(); ) {
        if (*it && (*it)->GetType() == Brick::BrickType::SPIKE) {
            m_Root.RemoveChild(*it);
            it = m_Bricks.erase(it);
        } else {
            ++it;
        }
    }

    // 重新 spawn 2 個 SPIKE
    Level15SpawnSpikes(2);
}

void LevelManager::Level15OnBossDamaged() {
    if (!m_BossBrick) return;

    // 傳送 Boss
    auto occupied = ComputeOccupied();
    occupied.erase({m_BossBrick->GetRow(), m_BossBrick->GetCol()});

    std::vector<std::pair<int, int>> empty;
    for (int r = 1; r <= 6; ++r) {
        for (int c = 0; c < 7; ++c) {
            if (!occupied.count({r, c})) empty.push_back({r, c});
        }
    }
    if (!empty.empty()) {
        static std::mt19937 g(std::random_device{}());
        auto p = empty[std::uniform_int_distribution<int>(0, empty.size() - 1)(g)];
        m_BossBrick->SetPosition(GetPositionByColumn(p.second, p.first));
        m_BossBrick->SetRowCol(p.first, p.second);
    }

    // 額外 spawn 2 個 SPIKE
    Level15SpawnSpikes(2);
}

void LevelManager::OnSpikeKilled() {
    if (m_CurrentLevel == 15) {
        m_Level15SpikeKilledThisRound++;
    }
}

void LevelManager::SpawnLevel16Round(int round) {
    auto SQ = [&](int c)        { SpawnBrick(c, Brick::BrickType::SQUARE,           round, 0); };
    auto HD = [&](int c)        { SpawnBrick(c, Brick::BrickType::HIGH_DENSITY,     round, 0); };
    auto IV = [&](int c)        { SpawnBrick(c, Brick::BrickType::INVISIBLE,        round, 0); };
    auto LK = [&](int c)        { SpawnBrick(c, Brick::BrickType::LINKED,           round, 0); };
    auto HB = [&](int c)        { SpawnBrick(c, Brick::BrickType::HARDENED_BOTTOM,  round, 0); };
    auto T  = [&](int c, int o) { SpawnBrick(c, Brick::BrickType::TRIANGLE,         round, o); };
    auto AB = [&](int c)        { SpawnBrick(c, Brick::BrickType::ADD_BALL,         0, 0); };
    auto CN = [&](int c)        { SpawnBrick(c, Brick::BrickType::COIN,             0, 0); };
    auto HL = [&](int c)        { SpawnBrick(c, Brick::BrickType::HLASER,           0, 0); };
    auto VL = [&](int c)        { SpawnBrick(c, Brick::BrickType::VLASER,           0, 0); };
    auto SC = [&](int c)        { SpawnBrick(c, Brick::BrickType::SCATTER,          0, 0); };

    if (round <= 25) {
        switch (round % 5) {
            case 1: AB(1); SQ(3); CN(5);                                            break;
            case 2: SQ(0); HD(1); AB(2); T(3, 3); SC(5); SQ(6);                     break;
            case 3: IV(0); T(2, 1); AB(3); T(4, 3); IV(6);                          break;
            case 4: SQ(0); SQ(1); HL(2); AB(4); HD(5); SQ(6);                       break;
            case 0: CN(1); LK(2); LK(4); AB(5);                                     break;
        }
        return;
    }

    switch (round) {
        case 26: LK(0); AB(1); LK(2); LK(4); CN(5); LK(6);                          break;
        case 27: HB(0); HB(1); AB(2); SQ(4); HL(5); SQ(6);                          break;
        case 28: VL(1); AB(3); VL(5);                                               break;
        case 29: SQ(0); HL(1); SQ(2); AB(4); HB(5); HB(6);                          break;
        case 30: LK(0); CN(1); LK(2); LK(4); AB(5); LK(6);                          break;
        case 31: LK(0); CN(1); LK(2); LK(4); AB(5); LK(6);                          break;
        case 32: SQ(0); HL(1); SQ(2); AB(4); HB(5); HB(6);                          break;
        case 33: VL(1); AB(3); VL(5);                                               break;
        case 34: HB(0); HB(1); AB(2); SQ(4); HL(5); SQ(6);                          break;
        case 35: LK(0); AB(1); LK(2); LK(4); CN(5); LK(6);                          break;
        case 36: HD(0); AB(1); LK(2); LK(4); CN(5); HD(6);                          break;
        case 37: HB(0); HB(1); AB(2); SQ(4); SC(5); HB(6);                          break;
        case 38: VL(1); AB(3); VL(5);                                               break;
        case 39: HB(0); HL(1); SQ(2); AB(4); HL(5); HB(6);                          break;
        case 40: LK(0); LK(1); CN(2); LK(3); AB(4); LK(5); LK(6);                   break;
    }
}

void LevelManager::SpawnLevel17Round(int round) {
    auto HD = [&](int c)        { SpawnBrick(c, Brick::BrickType::HIGH_DENSITY,     round, 0); };
    auto IV = [&](int c)        { SpawnBrick(c, Brick::BrickType::INVISIBLE,        round, 0); };
    auto SK = [&](int c)        { SpawnBrick(c, Brick::BrickType::SPIKE,            round, 0); };
    auto DD = [&](int c)        { SpawnBrick(c, Brick::BrickType::DOUBLE_DROP,      round, 0); };
    auto HB = [&](int c)        { SpawnBrick(c, Brick::BrickType::HARDENED_BOTTOM,  round, 0); };
    auto T  = [&](int c, int o) { SpawnBrick(c, Brick::BrickType::TRIANGLE,         round, o); };
    auto AB = [&](int c)        { SpawnBrick(c, Brick::BrickType::ADD_BALL,         0, 0); };
    auto CN = [&](int c)        { SpawnBrick(c, Brick::BrickType::COIN,             0, 0); };
    auto SC = [&](int c)        { SpawnBrick(c, Brick::BrickType::SCATTER,          0, 0); };
    auto G  = [&](int c)        { SpawnGiant(c, round); };

    if (round <= 20) {
        // 右 GIANT (col 5) 在 mod 1 spawn；左 GIANT (col 0) 在 mod 4 spawn
        switch (round % 5) {
            case 1: HB(0); HB(1); CN(2); AB(3); G(5);                               break; // 右 GIANT 下半
            case 2: AB(1); SK(2); SK(3); CN(4);                                     break; // 右 GIANT 上半
            case 3: T(0, 3); CN(1); HD(3); AB(5); T(6, 1);                          break;
            case 4: G(0); AB(2); SK(3); SK(4); CN(5);                               break; // 左 GIANT 下半
            case 0: CN(3); AB(4); HB(5); HB(6);                                     break; // 左 GIANT 上半
        }
        return;
    }

    // round 21-40: 雙 GIANT 在 mod 1, 4 spawn
    switch (round % 5) {
        case 1: G(0); G(5);                                                         break; // 雙 GIANT 下半
        case 2: /* 雙 GIANT 上半，不重 spawn */                                       break;
        case 3: IV(0); DD(1); AB(3); DD(5); IV(6);                                  break;
        case 4: G(0); G(5); CN(2); SC(3); AB(4);                                    break; // 雙 GIANT 下半
        case 0: AB(3);                                                              break; // 雙 GIANT 上半
    }
}

void LevelManager::SpawnLevel18Round(int round) {
    auto SQ = [&](int c)        { SpawnBrick(c, Brick::BrickType::SQUARE,           round, 0); };
    auto HD = [&](int c)        { SpawnBrick(c, Brick::BrickType::HIGH_DENSITY,     round, 0); };
    auto IV = [&](int c)        { SpawnBrick(c, Brick::BrickType::INVISIBLE,        round, 0); };
    auto SP = [&](int c)        { SpawnBrick(c, Brick::BrickType::SPLIT,            round, 0); };
    auto LK = [&](int c)        { SpawnBrick(c, Brick::BrickType::LINKED,           round, 0); };
    auto DD = [&](int c)        { SpawnBrick(c, Brick::BrickType::DOUBLE_DROP,      round, 0); };
    auto HB = [&](int c)        { SpawnBrick(c, Brick::BrickType::HARDENED_BOTTOM,  round, 0); };
    auto SK = [&](int c)        { SpawnBrick(c, Brick::BrickType::SPIKE,            round, 0); };
    auto T  = [&](int c, int o) { SpawnBrick(c, Brick::BrickType::TRIANGLE,         round, o); };
    auto AB = [&](int c)        { SpawnBrick(c, Brick::BrickType::ADD_BALL,         0, 0); };
    auto CN = [&](int c)        { SpawnBrick(c, Brick::BrickType::COIN,             0, 0); };
    auto HL = [&](int c)        { SpawnBrick(c, Brick::BrickType::HLASER,           0, 0); };
    auto SC = [&](int c)        { SpawnBrick(c, Brick::BrickType::SCATTER,          0, 0); };

    if (round <= 12) {
        switch (round % 3) {
            case 1: AB(0); SQ(1); T(2, 3); AB(3); T(4, 2); SQ(5); AB(6);            break;
            case 2: LK(0); LK(2); IV(3); LK(4); LK(6);                              break;
            case 0: CN(0); SP(1); T(2, 1); HD(3); T(4, 0); SP(5); CN(6);            break;
        }
        return;
    }

    if (round <= 24) {
        switch (round % 3) {
            case 1: AB(2); SP(3); AB(4);                                            break;
            case 2: DD(0); DD(1); CN(2); HD(3); CN(4); DD(5); DD(6);                break;
            case 0: SP(2); AB(3); SP(4);                                            break;
        }
        return;
    }

    if (round <= 36) {
        switch (round % 3) {
            case 1: HB(0); HB(1); HB(2); SC(3); HB(4); HB(5); HB(6);                break;
            case 2: HL(0); SK(1); SK(2); SK(3); SK(4); SK(5); HL(6);                break;
            case 0: HB(0); AB(2); AB(3); AB(4); HB(6);                              break;
        }
        return;
    }

    switch (round) {
        case 37: IV(0); IV(1); IV(2); AB(3); IV(4); IV(5); IV(6);                   break;
        case 38: DD(0); DD(1); AB(2); DD(5); DD(6);                                 break;
        case 39: HD(2); AB(3); HD(4);                                               break;
        case 40: DD(0); DD(1); AB(4); DD(5); DD(6);                                 break;
    }
}

void LevelManager::SpawnLevel19Round(int round) {
    auto SQ = [&](int c) { SpawnBrick(c, Brick::BrickType::SQUARE,           round, 0); };
    auto IV = [&](int c) { SpawnBrick(c, Brick::BrickType::INVISIBLE,        round, 0); };
    auto SP = [&](int c) { SpawnBrick(c, Brick::BrickType::SPLIT,            round, 0); };
    auto LK = [&](int c) { SpawnBrick(c, Brick::BrickType::LINKED,           round, 0); };
    auto HB = [&](int c) { SpawnBrick(c, Brick::BrickType::HARDENED_BOTTOM,  round, 0); };
    auto AB = [&](int c) { SpawnBrick(c, Brick::BrickType::ADD_BALL,         0, 0); };
    auto SC = [&](int c) { SpawnBrick(c, Brick::BrickType::SCATTER,          0, 0); };

    // 厚底列：round 1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40
    auto HBRow1 = [&]() { HB(0); HB(1); SC(2); HB(3); SC(4); HB(5); HB(6); };
    auto HBRow2 = [&]() { HB(0); HB(1); HB(2); SC(3); HB(4); HB(5); HB(6); };

    switch (round) {
        case 1:  HBRow1();                                                          break;
        case 2:  AB(0); IV(2); AB(3); AB(6);                                        break;
        case 3:  SQ(2); SQ(4);                                                      break;
        case 4:  HBRow2();                                                          break;
        case 5:  AB(0); LK(1); LK(2); AB(3); SP(4); IV(5); AB(6);                   break;
        case 6:  SQ(0); SP(2); IV(3); SP(4); SQ(6);                                 break;
        case 7:  HBRow1();                                                          break;
        case 8:  AB(0); IV(1); SQ(2); AB(3); IV(5); AB(6);                          break;
        case 9:  SQ(1); IV(2);                                                      break;
        case 10: HBRow2();                                                          break;
        case 11: AB(0); IV(1); SQ(2); AB(3); LK(4); LK(5); AB(6);                   break;
        case 12: LK(1); IV(3); SQ(4); SQ(6);                                        break;
        case 13: HBRow1();                                                          break;
        case 14: AB(0); IV(2); AB(3); SQ(4); IV(5); AB(6);                          break;
        case 15: SQ(1); SQ(2); IV(3);                                               break;
        case 16: HBRow2();                                                          break;
        case 17: AB(0); SP(1); IV(2); AB(3); SP(5); AB(6);                          break;
        case 18: SQ(1); LK(2); SQ(3); LK(4); IV(5); SP(6);                          break;
        case 19: HBRow1();                                                          break;
        case 20: AB(0); SQ(1); AB(3); SP(4); IV(5); AB(6);                          break;
        case 21: IV(0); IV(1); SP(2); SP(3); SQ(5);                                 break;
        case 22: HBRow2();                                                          break;
        case 23: AB(0); IV(1); LK(2); AB(3); LK(4); SP(5); AB(6);                   break;
        case 24: SQ(1); SP(3); SQ(4); SP(5);                                        break;
        case 25: HBRow1();                                                          break;
        case 26: AB(0); SP(1); SP(2); AB(3); LK(4); AB(6);                          break;
        case 27: LK(1); LK(2); IV(3); SQ(4); SP(5);                                 break;
        case 28: HBRow2();                                                          break;
        case 29: AB(0); SP(1); LK(2); AB(3); SP(4); AB(6);                          break;
        case 30: IV(1); LK(2); SQ(3); SP(4); LK(5); IV(6);                          break;
        case 31: HBRow1();                                                          break;
        case 32: AB(0); IV(2); AB(3); AB(6);                                        break;
        case 33: SQ(2); SQ(4);                                                      break;
        case 34: HBRow2();                                                          break;
        case 35: AB(0); LK(1); LK(2); AB(3); SP(4); IV(5); AB(6);                   break;
        case 36: SQ(0); SP(2); IV(3); SP(4); SQ(6);                                 break;
        case 37: HBRow1();                                                          break;
        case 38: AB(0); IV(1); SQ(2); AB(3); IV(5); AB(6);                          break;
        case 39: SQ(1); IV(2);                                                      break;
        case 40: HBRow2();                                                          break;
    }
}

void LevelManager::SpawnBrickAtRow(int targetRow, int col, Brick::BrickType type, int hpRound, int orientation) {
    SpawnBrick(col, type, hpRound, orientation);
    if (!m_Bricks.empty()) {
        auto& brick = m_Bricks.back();
        brick->SetPosition(GetPositionByColumn(col, targetRow));
        brick->SetRowCol(targetRow, col);
    }
}

void LevelManager::SpawnLevel20Round(int round) {
    auto SQ_at = [&](int row, int col) { SpawnBrickAtRow(row, col, Brick::BrickType::SQUARE,          40, 0); };
    auto SP_at = [&](int row, int col) { SpawnBrickAtRow(row, col, Brick::BrickType::SPLIT,           40, 0); };
    auto LK_at = [&](int row, int col) { SpawnBrickAtRow(row, col, Brick::BrickType::LINKED,          40, 0); };
    auto DD_at = [&](int row, int col) { SpawnBrickAtRow(row, col, Brick::BrickType::DOUBLE_DROP,     40, 0); };
    auto HB_at = [&](int row, int col) { SpawnBrickAtRow(row, col, Brick::BrickType::HARDENED_BOTTOM, 40, 0); };
    auto HL_at = [&](int row, int col) { SpawnBrickAtRow(row, col, Brick::BrickType::HLASER,          0,  0); };
    auto CN_at = [&](int row, int col) { SpawnBrickAtRow(row, col, Brick::BrickType::COIN,            0,  0); };
    auto SC_at = [&](int row, int col) { SpawnBrickAtRow(row, col, Brick::BrickType::SCATTER,         0,  0); };

    // round 1: Boss 第一次出現於 row 2, col 3，HP 500
    if (round == 1 && !m_BossBrick) {
        auto pos = GetPositionByColumn(3, 2);
        auto boss = std::make_shared<Brick>(Brick::BrickType::LEVEL20_BOSS, 1, 0);
        boss->SetPosition(pos);
        boss->SetRowCol(2, 3);
        m_Bricks.push_back(boss);
        m_Root.AddChild(boss);
        m_BossBrick = boss;
    }

    bool odd = (round % 2 == 1);

    if (odd) {
        // 奇數回合
        // Row 1 (第二列): 方形 | _ | 連鎖 | 橫向雷射 | 連鎖 | _ | 方形
        if (IsGridSlotEmpty(1, 0)) SQ_at(1, 0);
        if (IsGridSlotEmpty(1, 2)) LK_at(1, 2);
        if (IsGridSlotEmpty(1, 3)) HL_at(1, 3);
        if (IsGridSlotEmpty(1, 4)) LK_at(1, 4);
        if (IsGridSlotEmpty(1, 6)) SQ_at(1, 6);

        // Row 4 (第五列): 金幣 | 下墜 | 分裂 | _ | 分裂 | 下墜 | 金幣
        if (IsGridSlotEmpty(4, 0)) CN_at(4, 0);
        if (IsGridSlotEmpty(4, 1)) DD_at(4, 1);
        if (IsGridSlotEmpty(4, 2)) SP_at(4, 2);
        if (IsGridSlotEmpty(4, 4)) SP_at(4, 4);
        if (IsGridSlotEmpty(4, 5)) DD_at(4, 5);
        if (IsGridSlotEmpty(4, 6)) CN_at(4, 6);
    } else {
        // 偶數回合
        // Row 1 (第二列): _ | _ | 分裂 | 橫向雷射 | 分裂 | _ | _
        if (IsGridSlotEmpty(1, 2)) SP_at(1, 2);
        if (IsGridSlotEmpty(1, 3)) HL_at(1, 3);
        if (IsGridSlotEmpty(1, 4)) SP_at(1, 4);

        // Row 4 (第五列): 金幣 | 厚底 | 厚底 | 散射 | 厚底 | 厚底 | 金幣
        if (IsGridSlotEmpty(4, 0)) CN_at(4, 0);
        if (IsGridSlotEmpty(4, 1)) HB_at(4, 1);
        if (IsGridSlotEmpty(4, 2)) HB_at(4, 2);
        if (IsGridSlotEmpty(4, 3)) SC_at(4, 3);
        if (IsGridSlotEmpty(4, 4)) HB_at(4, 4);
        if (IsGridSlotEmpty(4, 5)) HB_at(4, 5);
        if (IsGridSlotEmpty(4, 6)) CN_at(4, 6);
    }
}

void LevelManager::Level20OnBossDamaged() {
    if (!m_BossBrick) return;

    auto occupied = ComputeOccupied();
    occupied.erase({m_BossBrick->GetRow(), m_BossBrick->GetCol()});

    std::vector<std::pair<int, int>> empty;
    for (int r = 1; r <= 6; ++r) {
        for (int c = 0; c < 7; ++c) {
            if (!occupied.count({r, c})) empty.push_back({r, c});
        }
    }
    if (empty.empty()) return;

    static std::mt19937 g(std::random_device{}());
    auto p = empty[std::uniform_int_distribution<int>(0, empty.size() - 1)(g)];
    m_BossBrick->SetPosition(GetPositionByColumn(p.second, p.first));
    m_BossBrick->SetRowCol(p.first, p.second);
}

void LevelManager::Level20OnRoundStart(int currentRound) {
    if (!m_BossBrick) return;

    // Boss alive check
    bool bossAlive = false;
    for (auto& b : m_Bricks) {
        if (b == m_BossBrick) { bossAlive = true; break; }
    }
    if (!bossAlive) { m_BossBrick = nullptr; return; }

    // 1. 上一回合的 Boss 尖刺若還在 → 清除 + Boss HP +20 (cap 600)
    for (auto it = m_Bricks.begin(); it != m_Bricks.end(); ) {
        if (*it && (*it)->m_IsBossSpike) {
            m_Root.RemoveChild(*it);
            it = m_Bricks.erase(it);
            int newHp = std::min(m_BossBrick->GetHp() + 20, 600);
            m_BossBrick->SetHp(newHp);
        } else {
            ++it;
        }
    }

    // 2. 上一回合 Boss 扣血 ≥ 10 → 在第一列第四排 (row 0, col 3) 生成尖刺
    if (m_BossBrick->m_DmgThisRound >= 10 && IsGridSlotEmpty(0, 3)) {
        auto brick = std::make_shared<Brick>(Brick::BrickType::SPIKE, 1, 0);
        brick->SetHp(15);
        brick->m_IsBossSpike = true;                       
        brick->SetPosition(GetPositionByColumn(3, 0));     // row 0, col 3
        brick->SetRowCol(0, 3);
        m_Bricks.push_back(brick);
        m_Root.AddChild(brick);
    }
    m_BossBrick->m_DmgThisRound = 0;

    // 3. 奇偶回合顯隱切換（偶數隱形）+ 換圖
    m_BossBrick->SetBossHidden(currentRound % 2 == 0);
}

bool LevelManager::IsLevelCleared(int currentRound) {
    if (!m_IsLevelMode) return false;

    if (m_VictoryType == 1) {
        if (currentRound <= m_MaxRound) return false;   // 還沒過 15 回合，先不算清空

        int aliveBricks = 0;
        for (auto& b : m_Bricks) {
            if (b && !b->IsProp()) aliveBricks++;
        }
        return aliveBricks == 0;
    }
    if (m_VictoryType == 2) {
        if (!m_BossBrick) return false;
        for (auto& b : m_Bricks) if (b == m_BossBrick) return false;
        m_BossBrick = nullptr;
        return true;
    }
    return false;
}

bool LevelManager::IsLevelFailed() {
    return false;
}