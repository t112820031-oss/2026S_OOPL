#include "Ball.hpp"
#include "Util/Image.hpp" 
#include "Util/Time.hpp"

Ball::Ball(const std::string& imagePath) {
    // 載入圖片 (假設預設已經是 32x32)
    m_Drawable = std::make_shared<Util::Image>(imagePath);
    
    // 強制設定縮放比例為 1.0f (不放大也不縮小)
    GetTransform().scale = glm::vec2(1.0f, 1.0f); 
    // 確保在最上層
    SetZIndex(10);
}

void Ball::Update() {
    if (!m_IsActive) return;

    // 使用 GetDeltaTimeMs() 除以 1000 換算成秒
    float dt = Util::Time::GetDeltaTimeMs() / 1000.0f; 

    // 使用我們剛剛在 HPP 寫好的 Get/Set Position
    glm::vec2 currentPos = GetPosition();
    glm::vec2 newPos = currentPos + (m_Velocity * dt);
    SetPosition(newPos);
}