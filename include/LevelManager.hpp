#ifndef LEVEL_MANAGER_HPP
#define LEVEL_MANAGER_HPP

#include <vector>
#include <memory>
#include <set>
#include <random>
#include <algorithm>
#include "Brick.hpp"
#include "Util/GameObject.hpp"
#include "Prop.hpp"
#include "GameConfig.hpp" 
#include "glm/glm.hpp"    

/**
 * @class LevelManager
 * @brief 負責管理 BBTAN 的 20 個關卡邏輯、磚塊生成與回合遞進。
 */
class LevelManager {
public:
    LevelManager(Util::GameObject& root);

    ~LevelManager() = default;

    /**
     * @brief BBTAN 核心邏輯：所有球收回後，磚塊下移一格。
     * 此函數會處理位移並在頂部生成新的一列。
     */
    void NextTurn();

    /**
     * @brief 更新目前所有磚塊的狀態（如：血量文字更新、碰撞後的特效）。
     */
    void Update();

    /**
     * @brief 將目前所有磚塊渲染至畫面上。
     */
    void Draw();

    /**
     * @brief 判斷是否輸掉遊戲。
     * @return true 代表有磚塊接觸到底線。
     */
    bool CheckGameOver() const;

    std::vector<std::shared_ptr<Brick>>& GetBricks() { return m_Bricks; }
    
    void GenerateRow(int round);
    bool TryPlaceBomb(glm::vec2 mousePos, int& outRow, int& outCol); // Update signature to match usage
    void TriggerTNTExplosion(int centerRow, int centerCol, Util::GameObject& root);
    bool UseDrill(Util::GameObject& root);
    bool UseHorizontalLaser(Util::GameObject& root);

    // 取得與設定當前關卡編號
    int GetCurrentLevel() const { return m_CurrentLevel; }
    void SetCurrentLevel(int level) { m_CurrentLevel = level; }

    // 檢查是否有磚塊觸碰到底線
    bool CheckGameOver(float deathLineY);

    void Clear(); 

    int GetActiveBrickCount(Brick::BrickType targetType) const;

    void SpawnRowEndless(int currentRound);

    // 處理所有下墜磚塊的額外下落
    void ProcessDoubleDrop();

    void AddLinked(int hp);
    bool DamageLinked(int dmg);    
    int  GetLinkedSharedHp() const { return m_LinkedSharedHp; }

    void HandleSplit(int newHp);

    void SpawnGiant(int col, int currentRound);
    std::set<std::pair<int, int>> ComputeOccupied() const;

    void StartLevel(int level);
    bool IsLevelCleared(int currentRound);
    bool IsLevelFailed();      // 撐不到指定回合或 BOSS 在死亡線前撞底
    
    bool IsLevelMode() const { return m_IsLevelMode; }
    int  GetMaxRound() const { return m_MaxRound; }
    int  GetLevelNum() const { return m_CurrentLevel; }

    void SpawnRowLevel(int currentRound);   // 關卡模式專屬

    void SpawnLevel10Round(int round);
    void Level10TeleportBoss();
    void Level10MaintainSplits();
    void OnLevelRoundEnd(int currentRound);

    void SpawnLevel15Round(int round);
    void Level15OnRoundStart();      // 每回合 Boss 在場時：清 SPIKE + spawn 3 個
    void Level15OnBossDamaged();     // 每 15 滴扣血：傳送 + spawn 3 個 SPIKE
    void Level15SpawnSpikes(int count);
    void OnSpikeKilled();           // SPIKE 被摧毀時通知
    bool m_LevelFailed = false;     // 關卡失敗 flag

    void SpawnLevel20Round(int round);
    void Level20OnBossDamaged();    // 每 10 滴扣血：傳送
    void Level20OnRoundStart(int currentRound);   // 每回合：處理 SPIKE 跟顯隱

    // helper: 在指定 row 生成磚塊
    void SpawnBrickAtRow(int targetRow, int col, Brick::BrickType type, int hpRound, int orientation);

    int GetLastDrillTargetCol() const { return m_LastDrillTargetCol; }
    int GetLastLaserTargetRow() const { return m_LastLaserTargetRow; }
    
private:
    // 將滑鼠螢幕座標轉換為網格的 [row, col] 索引
    // 如果滑鼠在網格範圍外，回傳 false
    bool ScreenPosToGridIndex(glm::vec2 screenPos, int& outRow, int& outCol) const;

    // 檢查指定的 [row, col] 是否已經有磚塊或道具
    bool IsGridSlotEmpty(int row, int col) const;

    // --- 輔助抽卡函式宣告 ---
    Brick::BrickType RollGuaranteedBrick(int currentRound);
    Brick::BrickType RollRandomBrick(int currentRound);
    Brick::BrickType RollRandomProp();
    Brick::BrickType RollSpecialBrick(int currentRound);

    // --- 座標轉換宣告 ---
    glm::vec2 GetPositionByColumn(int col, int row = 0);

    // --- 生成實體物件的宣告 (上一個範例有呼叫到，需要補上) ---
    // 增加第四個參數
    void SpawnBrick(int col, Brick::BrickType type, int round, int orientation);
    void SpawnProp(int col, PropType type);

    /**
     * @brief 根據關卡編號初始化 20 關的排布數據。
     * 建議在此實作不同關卡的幾何形狀（如三角形排列、十字排布等）。
     */
    void LoadLevelData(int level);

    /**
     * @brief 在畫面上方隨機或根據關卡邏輯生成一行新磚塊。
     */
    void SpawnRow();

    int m_CurrentLevel = 1;
    const int m_MaxLevels = 20;

    // 使用容器管理畫面上所有的磚塊物件
    std::vector<std::shared_ptr<Brick>> m_Bricks;

    // 讓 LevelManager 記住 App 傳進來的 root
    Util::GameObject& m_Root;

    // 關卡網格參數設定（例如 7x9 的網格）
    const float m_GridCellSize = 64.0f;
    const float m_BottomBoundary = 600.0f; // 觸底判定高度

    int m_LinkedSharedHp = 0;

    bool m_IsLevelMode = false;
    int  m_MaxRound = 0;          // Type 1：總回合數
    int  m_VictoryType = 0;       // 1=回合用完清空、2=BOSS 死
    std::shared_ptr<Brick> m_BossBrick;   // Type 2 用

    void SpawnLevel1Round(int round);
    void SpawnLevel2Round(int round);
    void SpawnLevel3Round(int round);
    void SpawnLevel4Round(int round);
    void SpawnLevel5Round(int round);
    void SpawnLevel6Round(int round);
    void SpawnLevel7Round(int round);
    void SpawnLevel8Round(int round);
    void SpawnLevel9Round(int round);
    
    void SpawnLevel11Round(int round);
    void SpawnLevel12Round(int round);
    void SpawnLevel13Round(int round);
    void SpawnLevel14Round(int round);

    void SpawnLevel16Round(int round);
    void SpawnLevel17Round(int round);
    void SpawnLevel18Round(int round);
    void SpawnLevel19Round(int round);
    int m_Level15SpikeKilledThisRound = 0;
    int m_Level15NoKillRounds = 0;

    int m_LastDrillTargetCol = -1;
    int m_LastLaserTargetRow = -1;
};

#endif