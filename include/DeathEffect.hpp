#ifndef DEATHEFFECT_HPP
#define DEATHEFFECT_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "Util/Time.hpp"

class DeathEffect : public Util::GameObject {
public:
    DeathEffect(const std::string& imagePath, glm::vec2 pos, glm::vec3 color) {
        SetDrawable(std::make_shared<Util::Image>(imagePath));
        
        // 使用 m_Transform 設定座標
        m_Transform.translation = pos;
        SetZIndex(3); 
        
        m_Color = color;
    }

    void Update() {
        // 使用 GetDeltaTimeMs() 並換算成秒
        m_Timer += (Util::Time::GetDeltaTimeMs() / 1000.0f);
        
        float progress = m_Timer / m_Duration;
        if (progress > 1.0f) progress = 1.0f;
    }

    bool IsDead() const {
        return m_Timer >= m_Duration;
    }

private:
    float m_Timer = 0.0f;
    const float m_Duration = 0.3f; 
    glm::vec3 m_Color;
};

#endif