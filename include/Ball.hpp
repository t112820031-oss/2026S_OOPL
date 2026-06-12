#ifndef BALL_HPP
#define BALL_HPP

#include "Util/GameObject.hpp"
#include <string>
#include <unordered_set>
#include <memory>
#include <glm/vec2.hpp>

class Prop; 
class Brick;

class Ball : public Util::GameObject {
public:
    Ball(const std::string& imagePath);

    void Update();

    void SetActive(bool active) { m_IsActive = active; }
    bool IsActive() const { return m_IsActive; }

    void SetRainbow(bool rainbow) { m_IsRainbow = rainbow; }
    bool IsRainbow() const { return m_IsRainbow; }

    float GetRadius() const { return 16.0f; } // 直接回傳 16.0f
    
    glm::vec2 GetPosition() const { return m_Transform.translation; }
    void SetPosition(const glm::vec2& pos) { m_Transform.translation = pos; }

    void SetVelocity(const glm::vec2& velocity) { m_Velocity = velocity; }
    glm::vec2 GetVelocity() const { return m_Velocity; }

    std::unordered_set<std::shared_ptr<Prop>> m_OverlappingProps;

    std::unordered_set<Brick*> m_HitBricks;   // 彩虹球本回合已扣過血的磚塊

private:
    bool m_IsActive = true;
    bool m_IsRainbow = false;

    glm::vec2 m_Velocity = glm::vec2(0.0f, 0.0f);
};

#endif // 結束 ifndef