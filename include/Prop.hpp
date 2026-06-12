#pragma once
#include "Util/GameObject.hpp"
#include "GameConfig.hpp" 
#include "glm/glm.hpp"

class Prop : public Util::GameObject {
public:
    Prop(PropType type);

    PropType GetType() const { return m_Type; }
    int GetRow() const { return m_RowIndex; }
    int GetCol() const { return m_ColIndex; }
    glm::vec2 GetPosition() const { return m_Transform.translation; }

    bool OnTrigger();
    float GetRadius() const;
    bool IsRoundEndClearable() const;

    // 👇 補上這個讓外部可以查詢
    bool IsConsumed() const { return m_IsConsumed; } 

private:
    PropType m_Type;
    int m_RowIndex = 0;
    int m_ColIndex = 0;
    
    // 👇 補上這個變數，預設為 false (還沒被吃掉)
    bool m_IsConsumed = false; 
};