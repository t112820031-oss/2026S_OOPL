#include "Prop.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp"

Prop::Prop(PropType type) : m_Type(type) {
    // 基底路徑
    std::string imagePath = RESOURCE_DIR "/Image/";
    
    // 根據資料夾圖片名稱進行配對
    switch (m_Type) {
        case PropType::AddBall:        
            imagePath += "PlusOne_Prop.png"; 
            break;
        case PropType::Coin:           
            imagePath += "Coin_Prop.png"; 
            break;
        case PropType::Scatter:        
            imagePath += "Split_Prop.png"; 
            break;
        case PropType::HorizontalLaser:
            imagePath += "HorizontalLLaser_Prop.png"; 
            break;
        case PropType::VerticalLaser:  
            imagePath += "VerticalLaser_Prop.png"; 
            break;
        default:
            imagePath += "PlusOne_Prop.png"; 
            break;
    }
    
    SetDrawable(std::make_shared<Util::Image>(imagePath));
    SetZIndex(-4); // 設定圖層，顯示在背景之上

    m_Transform.scale = glm::vec2(0.46f, 0.46f);
}

bool Prop::OnTrigger() {
    // 如果已經被吃掉了就不要再重複觸發
    if (m_IsConsumed) return true;

    // 規則 1：加一球與金幣，只會被觸發一次，觸發後立刻消失 
    if (m_Type == PropType::AddBall || m_Type == PropType::Coin) {
        m_IsConsumed = true;
        LOG_DEBUG("Prop Consumed: {}", static_cast<int>(m_Type));
        return true; 
    }
    
    // 規則 2：雷射與發散，可以在同一回合期間重複觸發，不會立刻消失
    LOG_DEBUG("Prop Triggered (Multi-use): {}", static_cast<int>(m_Type));
    return false; 
}

float Prop::GetRadius() const {
    return 3.0f * Config::PIXELS_PER_MM; // 結果為 24 pixels
}

bool Prop::IsRoundEndClearable() const {
    // 雷射與發散類型的道具，會在回合結束時消失 
    return (m_Type == PropType::Scatter || 
            m_Type == PropType::HorizontalLaser || 
            m_Type == PropType::VerticalLaser);
}