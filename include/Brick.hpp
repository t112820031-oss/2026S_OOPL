#ifndef BRICK_HPP
#define BRICK_HPP

#include "Util/Image.hpp"
#include "Util/GameObject.hpp"
#include "Util/Text.hpp"
#include "GameConfig.hpp"
#include <glm/glm.hpp> 
#include <memory>
#include <cmath>

class Brick : public Util::GameObject {
public:
    enum class BrickType {
        SQUARE,
        TRIANGLE,
        GIANT,
        TNT,
        
        // 解鎖配對（階段二做配對邏輯）
        LOCKED,
        KEY,                    

        // 狀態類（階段二做切換邏輯）
        INVISIBLE,
        SHIELDED,

        // 新增特殊磚塊
        HIGH_DENSITY,           
        LINKED,                 
        SPIKE,                  
        SPLIT,                  
        HARDENED_BOTTOM,        
        DOUBLE_DROP,            

        LEVEL10_BOSS,
        LEVEL15_BOSS,
        LEVEL20_BOSS,

        // 道具
        ADD_BALL, COIN, HLASER, VLASER, SCATTER
    };

    void UpdateColor();

    void SetRowCol(int r, int c) { m_RowIndex = r; m_ColIndex = c; }

    // 座標與圖形相關 (PTSD 標準寫法)
    glm::vec2 GetPosition() const { return m_Transform.translation; }
    void SetPosition(const glm::vec2& pos) { m_Transform.translation = pos; }
    std::string GetImagePath() const { return m_ImagePath; } // 假設你有存
    glm::vec3 GetCurrentColor() const { return m_CurrentColor; } // 假設你有存

    // 網格相關
    int GetRow() const { return m_RowIndex; }
    int GetCol() const { return m_ColIndex; }

    // 道具與種類判斷
    bool IsProp() const {
        return m_Type == BrickType::ADD_BALL || m_Type == BrickType::COIN || 
            m_Type == BrickType::HLASER || m_Type == BrickType::VLASER || 
            m_Type == BrickType::SCATTER;
    }
    BrickType GetType() const { return m_Type; }

    // 建構子中需要傳入當下回合數，當作最大血量
    Brick(BrickType type, int maxHp);

    // 建構子：傳入磚塊種類、當前回合數，以及選填的方向(給三角形用)
    Brick(BrickType type, int currentRound, int orientation = 0);

    Brick(const std::string& imagePath) {
        m_Drawable = std::make_shared<Util::Image>(imagePath);
        m_Hp = 0; // 道具不需要血量
    }


    // 修改 OnHit，加入一個參數判斷是否為「金幣道具」的攻擊
    bool OnHit(int damage = 1, bool isFromCoinProp = false); 

    // 動態判斷磚塊大小 (直接回傳精確的像素尺寸，不受設定檔干擾)
    glm::vec2 GetSize() const {
        if (m_Type == BrickType::GIANT) {
            return glm::vec2(138.0f, 138.0f); // 巨大磚塊 (完美覆蓋 2x2 的邊緣)
        } 
        else if (IsProp()) {
            return glm::vec2(32.0f, 32.0f);   // 道具 (縮小碰撞箱，避免空氣牆)
        }
        
        return glm::vec2(64.0f, 64.0f);       // 預設所有普通實體磚塊 (1x1)
    }
    
    int GetHp() const { return m_Hp; }

    // 設定血量的函式，會自動更新文字顯示
    void SetHp(int hp);
    
    // 取得三角形的方向 (0=左上, 1=右上, 2=左下, 3=右下)
    int GetOrientation() const { return m_Orientation; }

    // 新增一個回合結束的通知函式
    void OnRoundEnd();

    void SetAnimationStartPos(glm::vec2 pos) { m_AnimStartPos = pos; }
    glm::vec2 GetAnimationStartPos() const { return m_AnimStartPos; }
    
    void SetAnimationTargetPos(glm::vec2 pos) { m_AnimTargetPos = pos; }
    glm::vec2 GetAnimationTargetPos() const { return m_AnimTargetPos; }

    // 用於記錄該磚塊目前位於第幾列 (0~8)，方便判斷遊戲結束
    int m_RowIndex = 0;

    // 更新血量文字顯示
    void UpdateHpText();

    // 確保這行存在，告訴引擎「畫圖這件事由我親自來處理」
    void Draw();

    // 讓外部可以設定與讀取它的身分
    void SetPlusItem(bool isItem) { m_IsPlusItem = isItem; }
    bool IsPlusItem() const { return m_IsPlusItem; }

    // 提供外部讀取與修改
    bool IsActivated() const { return m_IsActivated; }
    void SetActivated(bool activated) { m_IsActivated = activated; }

    // 存取最後撞擊的球
    void* GetLastHitBall() const { return m_LastHitBall; }
    void SetLastHitBall(void* ball) { m_LastHitBall = ball; }

    // 依 row 奇偶切換顯形/隱形外觀
    void RefreshInvisibleVisual();
    bool IsInvisibleHidden() const {
        if (m_Type == BrickType::INVISIBLE)    return (m_RowIndex % 2 == 0);
        if (m_Type == BrickType::LEVEL20_BOSS) return m_BossHidden;
        return false;
    }

    float m_GraceTimer = 0.0f;       // 0.3 秒穿透計時
    int   GetMaxHp() const { return m_MaxHp; }

    int m_DmgSinceTeleport = 0;
    bool m_IsBossGuard = false;     // 標記為 Boss 守衛磚塊
    
    int m_DmgThisRound = 0;       // 本回合累積傷害（LEVEL20_BOSS 用）
    bool m_BossHidden = false;     // 第 20 關 Boss 回合控制顯隱
    bool m_IsBossSpike = false;       // 第 20 關 Boss 固定在第一列第四排的尖刺（不下移）
    void SetBossHidden(bool hidden);  // 第 20 關 Boss 顯隱切換（含換圖）

    int m_InitRound = 1;   // 生成時的回合，用於計算染色基準
    bool ShouldTint() const;

    int GetInitRound() const { return m_InitRound; }
    void SetMaxHp(int hp) { m_MaxHp = hp; }

private:
    BrickType m_Type;
    int m_Orientation; // 記錄三角形的朝向
    
    bool m_HasShield = false;       // 是否有護盾
    bool m_HitThisRound = false;    // 這回合是否被一般球打過

    int m_Hp = 0;
    int m_MaxHp = 0; // 記錄最大血量，用來算比例

    std::string m_ImagePath;
    glm::vec3 m_CurrentColor;
    int m_ColIndex = 0;

    // 根據企劃書邏輯初始化血量
    void InitHpByRound(int round);

    // 文字物件
    std::shared_ptr<Util::Text> m_HpText;

    // 加上這個變數，預設為 false (普通磚塊)
    bool m_IsPlusItem = false;
    
    // 負責帶著文字移動的實體
    std::shared_ptr<Util::GameObject> m_TextObj = nullptr;
    // 專門控制文字內容的遙控器！
    std::shared_ptr<Util::Text> m_TextDrawable = nullptr;

    glm::vec2 m_AnimStartPos;
    glm::vec2 m_AnimTargetPos;

    // 宣告這個專門用來顯示數字的小孩
    glm::vec3 CalculateGradientColor() const;

    // 用來記錄這個道具這回合是否已經發動過了
    bool m_IsActivated = false;
    // 記錄上一顆撞到這個磚塊的球（用來處理同一回合多次撞擊的情況，避免重複觸發道具效果）
    void* m_LastHitBall = nullptr;

    std::string m_CurrentColorPath;   // 避免重複 SetDrawable 同張圖
    const char* GetBaseImageName() const;   // 回傳該磚塊基底圖檔名 (不含 _Color.png)

    bool m_InitialColorDone = false;
};

#endif