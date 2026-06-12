#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" 
#include "EffectLine.hpp"

#include <vector>
#include <memory>
#include <string>

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "Util/Text.hpp"

#include "Ball.hpp"
#include "LevelManager.hpp"
#include "DeathEffect.hpp" 

class App {
    public:
        App();
        ~App();

        enum class GameState {
            MAIN_MENU,
            GAMEPLAY_ENDLESS,
            GAMEPLAY_LEVEL,
            BALL_SELECTION,
            GAME_OVER,           
            LEVEL_INTRO,
            PROPS_INTRO
        };

        enum class GameplayState {
            AIMING,
            SHOOTING,
            PLAYING,
            RECYCLING, 
            ROUND_END
        };

        GameState GetCurrentState() const { return m_State; }

        void Init();   
        void Start();
        void Update();
        void End(); 

    private:
        
        void UpdateGameplay();
        GameState m_State = GameState::MAIN_MENU; 

        void UpdateBallsAndCollisions(float dt);
        
        std::vector<std::shared_ptr<Ball>> m_Balls;
        
        std::shared_ptr<Util::GameObject> m_MainMenuBG;
        std::shared_ptr<Util::GameObject> m_Character;
        std::shared_ptr<Util::GameObject> m_BtnEndless;
        std::shared_ptr<Util::GameObject> m_BtnLevel;
        std::shared_ptr<Util::GameObject> m_BtnSelectBall;

        std::shared_ptr<Util::GameObject> m_MenuTopTextObj;
        std::shared_ptr<Util::Text> m_MenuTopTextDrawable;

        // 宣告遊戲模式的背景
        std::shared_ptr<Util::GameObject> m_InGameBG;

        // 宣告第一顆球
        std::shared_ptr<Util::GameObject> m_Ball;
    
        GameplayState m_GameplayState = GameplayState::AIMING;
        
        // 用來傳遞給 LevelManager
        Util::GameObject m_Root;

        // 用來存放瞄準線點點的陣列
        std::vector<std::shared_ptr<Util::GameObject>> m_AimDots;

        // 瞄準線的小點點
        std::shared_ptr<Util::GameObject> m_AimDot;
        
        // 追蹤滑鼠拖曳狀態的變數
        bool m_IsAiming = false;     // 是否正在瞄準
        glm::vec2 m_DragStart;       // 紀錄滑鼠剛按下去的座標
        std::shared_ptr<Util::Image> m_AimDotImage;
        glm::vec2 m_CurrentAimDir;

        // 物理狀態變數
        bool m_IsFiring = false;         // 判斷球是否正在飛
        glm::vec2 m_BallVelocity;        // 記錄球的飛行速度與方向

        // 機關槍連發系統專用變數
        bool m_IsFiringSequence = false;   // 是否正在執行連發程序
        int m_BallsFiredCount = 0;         // 記錄目前已經射出了幾顆球
        float m_FireTimer = 0.0f;          // 用來計算兩顆球之間的時間差
        glm::vec2 m_FireDirection = glm::vec2(0.0f, 0.0f); // 記錄瞄準的方向

        // 記錄目前是否正在戰鬥回合中
        bool m_IsRoundActive = false;

        int m_PendingExtraBalls = 0; // 本回合吃到的加球道具數量

        // Game Over 專用的物件與計時器
        std::shared_ptr<Util::GameObject> m_GameOverBG;
        std::shared_ptr<Util::GameObject> m_GameOverText;
        float m_GameOverTimer = 0.0f;

        void ResetGame();

        // 用來追蹤第一顆回來的球
        bool m_IsFirstBallReturned = false;
        glm::vec2 m_NextLaunchPos = glm::vec2(0.0f, -250.0f); // 預設的底線位置
    
        // 用來記住這回合的固定發射點
        glm::vec2 m_RoundStartPos = glm::vec2(0.0f, -250.0f);

        // --- 數據紀錄 ---
        int m_Coins = 8888888;
        int m_HighScore = 0; 

        // --- UI 文字物件 ---
        std::shared_ptr<Util::GameObject> m_CoinTextObj;
        std::shared_ptr<Util::Text> m_CoinTextDrawable;

        std::shared_ptr<Util::GameObject> m_RoundTextObj;
        std::shared_ptr<Util::Text> m_RoundTextDrawable;

        std::shared_ptr<Util::GameObject> m_TopTextObj;
        std::shared_ptr<Util::Text> m_TopTextDrawable;
        // --- 道具按鈕 ---
        std::shared_ptr<Util::GameObject> m_BtnRecycle; 
        std::shared_ptr<Util::GameObject> m_BtnSpeedUpIn;
        std::shared_ptr<Util::GameObject> m_BtnSpeedUpOut;
        // 收回功能
        bool m_IsRecycling = false;
        glm::vec2 m_RecycleTarget;
        void RecycleBalls();

        void RefreshTopUI();   // 每幀同步上排 UI 文字
        
        float m_SpeedUpRotation = 0.0f; 
        bool m_IsSpeedUp = false;       

        int m_TotalBalls = 1;         
        int m_BallsShot = 0;          
        int m_ReturnedBalls = 0;      

        float m_ShootTimer = 0.0f;    
        glm::vec2 m_ShootDir;         
        glm::vec2 m_LauncherPos;      
        glm::vec2 m_NextRoundPos;     

        float m_RoundEndTimer = 0.0f;
        const float ANIMATION_DURATION = 0.5f; 
        const float ROW_HEIGHT = 136.0f;       
        
        int m_CurrentRound = 1;                     

        bool m_ShouldSpawnBall = false;
        
        std::shared_ptr<Util::GameObject> m_BtnRainbowProp;
        // 道具「已購買、待使用」旗標
        bool m_HasBoughtDrill = false;
        bool m_HasBoughtHorizontalLaser = false;
        bool m_HasBoughtTnt = false;
        bool m_HasBoughtRainbow = false; 
        bool m_IsShootingRainbow = false; 
        
        std::shared_ptr<Util::GameObject> m_BtnTntProp; 
        std::shared_ptr<Util::GameObject> m_DragTntIcon; 
        bool m_IsDraggingTnt = false; 

        std::shared_ptr<Util::GameObject> m_BtnDrillProp;
        std::shared_ptr<Util::GameObject> m_BtnHorizontalLaserProp;
        
        std::shared_ptr<Util::GameObject> m_WarningObj; 
        std::shared_ptr<Util::Text> m_WarningText;
        float m_WarningTimer = 0.0f;
        void ShowWarning(const std::string& msg); 
        void UseDrill();
        void UseHorizontalLaser();

        std::unique_ptr<LevelManager> m_LevelManager;
        std::vector<std::shared_ptr<Util::GameObject>> m_BallOptions;
        
        std::shared_ptr<Util::GameObject> m_StartMenuBG;
        std::shared_ptr<Util::Text> m_TitleText;
        std::shared_ptr<Util::GameObject> m_StartBackground;
        std::shared_ptr<Util::GameObject> m_StartButton;
        
        int m_SelectedBallIndex = 0; 

        std::vector<std::shared_ptr<DeathEffect>> m_Effects;
            
        std::string GetSelectedBallPath() {
            return "Resources/Image/ball_" + std::to_string(m_SelectedBallIndex) + ".png";
        } 

        // LEVEL_SELECT 用：20 個關卡按鈕（圓背景 + 數字）
        std::vector<std::shared_ptr<Util::GameObject>> m_LevelButtons;
        std::vector<std::shared_ptr<Util::GameObject>> m_LevelButtonTexts;

        // 通關 UI
        std::shared_ptr<Util::GameObject> m_LevelAccomplishedObj;
        std::shared_ptr<Util::Text>       m_LevelAccomplishedText;
        float m_LevelAccomplishedTimer = 0.0f;

        std::shared_ptr<Util::GameObject> m_LevelIntroObj;
        int m_SelectedLevel = 0;   

        bool HasLevelIntro(int level) const;
        void ShowLevelIntro(int level);
        void EnterLevel(int level);     // 把原本選關後進關卡的程式碼包成這個

        std::shared_ptr<Util::GameObject> m_PropsIntroObj;
        bool m_PropsIntroShownEndless = false;   // 無限模式只在第一次進入時顯示
        bool m_PropsIntroShownLevel = false;     // Level 1 只在第一次進入時顯示

        void ShowPropsIntro(bool isLevelMode);
        
        std::vector<std::shared_ptr<EffectLine>> m_LaserEffects;

        void SpawnLaserEffectVertical(int col, float thickness, const char* imageName);
        void SpawnLaserEffectHorizontal(int row, float thickness, const char* imageName);
};

#endif