#include "Brick.hpp"
#include <algorithm> // 給 std::clamp 使用
#include "Util/Image.hpp"
#include <memory>

glm::vec3 Brick::CalculateGradientColor() const {
    // 全局基準血量：以「該回合 HD 滿血 = round × 2」為標尺
    // → 普通磚塊滿血 = 50% ratio → 紫色
    // → HD 滿血 = 100% ratio → 綠色
    int referenceMax = std::max(1, m_InitRound * 2);
    float ratio = std::clamp(static_cast<float>(m_Hp) / referenceMax, 0.0f, 1.0f);

    // 反轉：滿血(高) → totalValue=0 (起點: 綠)
    //       空血(低) → totalValue=765 (終點: 黃)
    int totalValue = static_cast<int>((1.0f - ratio) * 765.0f);
    int q = totalValue / 255;
    int r = totalValue % 255;

    int R = 0, G = 0, B = 0;
    switch (q) {
        case 0: // 綠 {0,255,0} → 藍 {0,0,255}：G↓ B↑
            R = 0;
            G = 255 - r;
            B = r;
            break;
        case 1: // 藍 {0,0,255} → 紅 {255,0,0}：R↑ B↓ (中段 = 紫)
            R = r;
            G = 0;
            B = 255 - r;
            break;
        case 2: // 紅 {255,0,0} → 黃 {255,255,0}：G↑
            R = 255;
            G = r;
            B = 0;
            break;
        default: // 空血溢出 (理論上不該到)
            R = 255; G = 255; B = 0;
            break;
    }
    return glm::vec3(R, G, B);
}

bool Brick::ShouldTint() const {
    if (IsProp()) return false; 
    switch (m_Type) {
        case BrickType::TNT:
        case BrickType::LOCKED:
        case BrickType::KEY:
        case BrickType::SHIELDED:
        case BrickType::INVISIBLE:
        case BrickType::GIANT:
        case BrickType::LEVEL10_BOSS:
        case BrickType::LEVEL15_BOSS:
        case BrickType::LEVEL20_BOSS:
            return false;       
        default:
            return true;         // SQUARE / HD / LINKED / SPLIT / HB / DD / SPIKE / TRIANGLE
    }
}

void Brick::Draw() {
    Util::GameObject::Draw();
    if (!m_TextObj) return;

    glm::vec2 offset(0.0f, 0.0f);

    if (m_Type == BrickType::TRIANGLE) {
        float t = 16.0f;
        glm::vec2 base(-t, -t);

        // 依 m_Orientation 旋轉這個 offset（圖片轉幾度，文字 offset 也轉幾度）
        float rad = glm::radians(90.0f * m_Orientation);
        float cs = std::cos(rad), sn = std::sin(rad);
        offset = glm::vec2(
            base.x * cs - base.y * sn,
            base.x * sn + base.y * cs
        );
    } else {
        offset = glm::vec2(0.0f, -6.0f);
    }

    m_TextObj->m_Transform.translation = this->m_Transform.translation + offset;
    m_TextObj->Draw();
}

void Brick::UpdateColor() {
    const char* base = GetBaseImageName();
    if (!base) return;

    // INVISIBLE 隱形時不染色
    if (m_Type == BrickType::INVISIBLE && (m_RowIndex % 2 == 0) && m_InitialColorDone) {
        return;
    }
    // Level20 Boss 隱形時不染色
    if (m_Type == BrickType::LEVEL20_BOSS && m_BossHidden) {
        return;
    }

    float ratio;
    if (m_Type == BrickType::LINKED && m_MaxHp > 0) {
        // LINKED 用池子當下/池子初始 (m_MaxHp 由 LevelManager 同步成池子滿值)
        ratio = static_cast<float>(m_Hp) / m_MaxHp;
    } else {
        // 一般磚塊：以「該回合 HD 滿血 = round × 2」為基準
        int referenceMax = std::max(1, m_InitRound * 2);
        ratio = static_cast<float>(m_Hp) / referenceMax;
    }
    ratio = std::clamp(ratio, 0.0f, 1.0f);

    const char* color;
    if      (ratio >= 0.85f) color = "Green";
    else if (ratio >= 0.65f) color = "Blue";
    else if (ratio >= 0.45f) color = "Purple";
    else if (ratio >= 0.25f) color = "Red";
    else if (ratio >= 0.10f) color = "Orange";
    else                     color = "Yellow";

    std::string newPath = std::string(RESOURCE_DIR) + "/Image/"
                        + base + "_" + color + ".png";
    if (newPath != m_CurrentColorPath) {
        m_CurrentColorPath = newPath;
        SetDrawable(std::make_shared<Util::Image>(newPath));
    }

    m_InitialColorDone = true;  
}

const char* Brick::GetBaseImageName() const {
    switch (m_Type) {
        case BrickType::SQUARE:          return "Normal_Brick";
        case BrickType::HIGH_DENSITY:    return "Normal_Brick";
        case BrickType::INVISIBLE:       return "Normal_Brick";
        case BrickType::TRIANGLE:        return "Triangle_Brick";
        case BrickType::LINKED:          return "Linked_Brick";
        case BrickType::SPLIT:           return "Spilt_Brick";
        case BrickType::HARDENED_BOTTOM: return "HardenedButtom_Brick";
        case BrickType::DOUBLE_DROP:     return "DoubleDrop_Brick";
        case BrickType::SPIKE:           return "Spike_Brick";
        case BrickType::GIANT:           return "Giant_Brick";
        case BrickType::SHIELDED:        return "Shielded_Brick";  
        default:                          return nullptr;
    }
}

void Brick::SetHp(int hp) {
    m_Hp = hp;

    if (m_TextObj) {
        Util::Color textColor = Util::Color(0, 0, 0, 255);   // 預設黑
        if (m_Type == BrickType::INVISIBLE && (m_RowIndex % 2 == 0)) {
            textColor = Util::Color(200, 200, 200, 255);
        }
        // 第 20 關 Boss 隱形時也用灰色
        if (m_Type == BrickType::LEVEL20_BOSS && m_BossHidden) {
            textColor = Util::Color(200, 200, 200, 255);
        }

        m_TextDrawable = std::make_shared<Util::Text>(
            RESOURCE_DIR "/Font/Montserrat-Bold.ttf",
            18,
            std::to_string(m_Hp),
            textColor
        );
        m_TextObj->SetDrawable(m_TextDrawable);
    }
    UpdateColor();   // 自動更新顏色
}

Brick::Brick(BrickType type, int currentRound, int orientation) 
    : Util::GameObject(), m_Type(type), m_Orientation(orientation) 
{
    InitHpByRound(currentRound);
    
    std::string imagePath;
    switch (m_Type) {
        case BrickType::SQUARE:          imagePath = RESOURCE_DIR "/Image/Normal_Brick.png"; break;
        case BrickType::TRIANGLE:        imagePath = RESOURCE_DIR "/Image/Triangle_Brick.png"; break;
        case BrickType::GIANT:           imagePath = RESOURCE_DIR "/Image/Giant_Brick.png"; break;
        case BrickType::TNT:             imagePath = RESOURCE_DIR "/Image/TNT_Brick.png"; break;
        case BrickType::LOCKED:          imagePath = RESOURCE_DIR "/Image/Locked_Brick.png"; break;
        case BrickType::KEY:             imagePath = RESOURCE_DIR "/Image/Key_Brick.png"; break;
        case BrickType::INVISIBLE:       imagePath = RESOURCE_DIR "/Image/Normal_Brick.png"; break; // 一開始顯形
        case BrickType::SHIELDED:        imagePath = RESOURCE_DIR "/Image/Shielded_Brick.png"; break;
        case BrickType::HIGH_DENSITY:    imagePath = RESOURCE_DIR "/Image/Normal_Brick.png"; break;
        case BrickType::LINKED:          imagePath = RESOURCE_DIR "/Image/Linked_Brick.png"; break;
        case BrickType::SPIKE:           imagePath = RESOURCE_DIR "/Image/Spike_Brick.png"; break;
        case BrickType::SPLIT:           imagePath = RESOURCE_DIR "/Image/Spilt_Brick.png"; break;
        case BrickType::HARDENED_BOTTOM: imagePath = RESOURCE_DIR "/Image/HardenedButtom_Brick.png"; break;
        case BrickType::DOUBLE_DROP:     imagePath = RESOURCE_DIR "/Image/DoubleDrop_Brick.png"; break;
        case BrickType::ADD_BALL:        imagePath = RESOURCE_DIR "/Image/PlusOne_Prop.png"; break;
        case BrickType::HLASER:          imagePath = RESOURCE_DIR "/Image/HorizontalLaser_Prop.png"; break;
        case BrickType::VLASER:          imagePath = RESOURCE_DIR "/Image/VerticalLaser_Prop.png"; break;
        case BrickType::COIN:            imagePath = RESOURCE_DIR "/Image/Coin_Prop.png"; break;
        case BrickType::SCATTER:         imagePath = RESOURCE_DIR "/Image/Scatter_Prop.png"; break;
        case BrickType::LEVEL10_BOSS:    imagePath = RESOURCE_DIR "/Image/Boss_Brick.png"; break;
        case BrickType::LEVEL15_BOSS:    imagePath = RESOURCE_DIR "/Image/Boss_Brick.png"; break;
        case BrickType::LEVEL20_BOSS:    imagePath = RESOURCE_DIR "/Image/Boss_Brick.png"; break;
        default:                         imagePath = RESOURCE_DIR "/Image/Normal_Brick.png"; break;
    }
        
    SetDrawable(std::make_shared<Util::Image>(imagePath));
    SetZIndex(-3);
    m_Transform.scale = glm::vec2(0.5f, 0.5f);

    // 立刻套用初始顏色 (依生成時 HP / referenceMax)
    UpdateColor();
    // 決定文字的最終偏移量
    
    if (m_Type == BrickType::TRIANGLE) {
        m_Transform.rotation = glm::radians(90.0f * m_Orientation);
    }

    // 建立文字物件，套用疊加後的偏移量
    if (m_Hp > 0) {
        m_TextObj = std::make_shared<Util::GameObject>();
        m_TextDrawable = std::make_shared<Util::Text>(
            RESOURCE_DIR "/Font/Montserrat-Bold.ttf", 
            18, 
            std::to_string(m_Hp), 
            Util::Color(0, 0, 0, 255) 
        );
        m_TextObj->SetDrawable(m_TextDrawable);
        m_TextObj->SetZIndex(20); 
    }

    // 防護罩磚塊一誕生就帶罩
    if (m_Type == BrickType::SHIELDED) {
        m_HasShield = true;
    }

    // 隱形磚塊一誕生就依列數決定顯/隱
    if (m_Type == BrickType::INVISIBLE) {
        UpdateColor();
    }
}

void Brick::InitHpByRound(int round) {
    m_InitRound = round;   // 記錄生成回合
    switch (m_Type) {
        case BrickType::SQUARE:
        case BrickType::TRIANGLE:
        case BrickType::LINKED:           // 階段二會接管：和場上其他 LINKED 共用 HP
        case BrickType::SPLIT:
        case BrickType::KEY:
        case BrickType::LOCKED:
        case BrickType::SHIELDED:
            m_Hp = round;
            break;

        case BrickType::HIGH_DENSITY:
            m_Hp = round * 2;             // 兩倍血
            break;

        case BrickType::SPIKE:
        case BrickType::HARDENED_BOTTOM:
            m_Hp = std::max(1, round / 2); // 一半（保底 1）
            break;

        case BrickType::INVISIBLE:
        case BrickType::DOUBLE_DROP:
            m_Hp = std::max(1, static_cast<int>(round * 0.8f)); // 0.8 倍
            break;

        case BrickType::TNT:
            m_Hp = 1;
            break;

        case BrickType::GIANT:
            m_Hp = round * 4;             // 沿用舊行為，4 倍
            break;

        case BrickType::LEVEL10_BOSS:
            m_Hp = 150;
            break;

        case BrickType::LEVEL15_BOSS:
            m_Hp = 300;
            break;

        case BrickType::LEVEL20_BOSS:
            m_Hp = 500;
            break;

        default:
            m_Hp = round;
            break;
    }
    m_MaxHp = m_Hp;
}

bool Brick::OnHit(int damage, bool isFromCoinProp) {
    // 在未解鎖前不受一般傷害，除非是金幣道具
    if (m_Type == BrickType::LOCKED) { 
        if (!isFromCoinProp) {
            return false; 
        }
    }

    // 防護罩磚塊 (Shielded) 的無敵判定
    if (m_Type == BrickType::SHIELDED && m_HasShield) {
        if (!isFromCoinProp) {
            // 一般球打到：不扣血，但記錄這回合被打過了
            m_HitThisRound = true;
            
            return false; 
        }
    }


    // Boss 共通：金幣道具單次傷害上限 30
    if (isFromCoinProp && 
        (m_Type == BrickType::LEVEL10_BOSS || 
        m_Type == BrickType::LEVEL15_BOSS ||
        m_Type == BrickType::LEVEL20_BOSS)) {
        damage = std::min(damage, 30);
    }
    // 正常受擊與扣血邏輯 (如果前面沒有被擋下來，就會走到這裡)
    SetHp(m_Hp - damage);
    if (m_Type == BrickType::LEVEL10_BOSS ||
        m_Type == BrickType::LEVEL15_BOSS ||
        m_Type == BrickType::LEVEL20_BOSS) {
        m_DmgSinceTeleport += damage;
    }

    // 第 20 關 Boss 累積本回合傷害（用於下回合決定 spawn SPIKE）
    if (m_Type == BrickType::LEVEL20_BOSS) {
        m_DmgThisRound += damage;
    }
    // 扣血後更新顏色
    UpdateColor();

    // 回傳是否死亡
    return m_Hp <= 0;
}

void Brick::UpdateHpText() {
    // 炸藥磚塊不需要血量顯示
    if (m_Type == BrickType::TNT) {
        m_HpText->SetText(""); 
        return;
    }
    if (m_Type == BrickType::LOCKED) {
        m_HpText->SetText("");
        return;
    }

    // 隱形磚塊 (偶數列顯形，奇數列隱形)
    if (m_Type == BrickType::INVISIBLE) {
        bool isInvisible = (m_RowIndex % 2 == 0); 
        
        if (isInvisible) {
            // 隱形時，數字以淺灰色寫出
            m_HpText->SetColor(Util::Color(200, 200, 200, 255));
        } else {
            // 顯形時，恢復黑色
            m_HpText->SetColor(Util::Color(0, 0, 0, 255));
        }
    }

    // 正常更新血量數字
    m_HpText->SetText(std::to_string(m_Hp));
}

// 每個回合結束時由 LevelManager 呼叫
void Brick::OnRoundEnd() {
    if (m_Type == BrickType::SHIELDED && m_HasShield && m_HitThisRound) {
        m_HasShield = false;
        m_HitThisRound = false;

        m_Type = BrickType::SQUARE;     // 先改型別
        m_CurrentColorPath = "";        // 重設快取
        UpdateColor();                  // 立刻染色
    }
}

void Brick::RefreshInvisibleVisual() {
    if (m_Type != BrickType::INVISIBLE) return;

    bool hidden = (m_RowIndex % 2 == 0);
    if (hidden) {
        SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/Invisible_Brick.png"));
        m_CurrentColorPath = "";  
        if (m_TextDrawable) {
            m_TextDrawable = std::make_shared<Util::Text>(
                RESOURCE_DIR "/Font/Montserrat-Bold.ttf", 18,
                std::to_string(m_Hp),
                Util::Color(200, 200, 200, 255));
            if (m_TextObj) m_TextObj->SetDrawable(m_TextDrawable);
        }
    } else {
        UpdateColor();             
        if (m_TextDrawable) {
            m_TextDrawable = std::make_shared<Util::Text>(
                RESOURCE_DIR "/Font/Montserrat-Bold.ttf", 18,
                std::to_string(m_Hp),
                Util::Color(0, 0, 0, 255));
            if (m_TextObj) m_TextObj->SetDrawable(m_TextDrawable);
        }
    }
}

void Brick::SetBossHidden(bool hidden) {
    m_BossHidden = hidden;
    if (m_Type == BrickType::LEVEL20_BOSS) {
        std::string img = hidden
            ? RESOURCE_DIR "/Image/Invisible_Brick.png"
            : RESOURCE_DIR "/Image/Boss_Brick.png";
        SetDrawable(std::make_shared<Util::Image>(img));

        // 切換顯隱時也更新文字顏色
        SetHp(m_Hp);
    }
}