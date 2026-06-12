#include "App.hpp"
#include "Util/Image.hpp"
#include "Util/Text.hpp"
#include "Util/Input.hpp"
#include "Util/Logger.hpp"

void App::Start() {
    LOG_DEBUG("=== 進入 App::Start ===");
    
    // 1. 載入主畫面背景
    m_MainMenuBG = std::make_shared<Util::GameObject>();
    m_MainMenuBG->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/MainMenu_BG.png"));
    m_MainMenuBG->SetZIndex(-10); 

    // 載入遊戲模式背景
    m_InGameBG = std::make_shared<Util::GameObject>();
    m_InGameBG->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/InGame_BG.png"));
    m_InGameBG->SetZIndex(-10);
    
    // ===== 選關畫面：20 個關卡按鈕（5 排 × 4 列）=====
    for (int i = 0; i < 20; ++i) {
        int row = i / 4;
        int col = i % 4;
        float x = (col - 1.5f) * 90.0f;        // 4 列：x = -135, -45, 45, 135
        float y = 200.0f - row * 120.0f;       // 5 排：y = 200, 80, -40, -160, -280
        
        auto btn = std::make_shared<Util::GameObject>();
        btn->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/NormalBall.png"));
        btn->m_Transform.translation = glm::vec2(x, y);
        btn->m_Transform.scale = glm::vec2(60.0f / 16.0f, 60.0f / 16.0f);
        btn->SetZIndex(5);
        m_LevelButtons.push_back(btn);
        m_Root.AddChild(btn);
        
        // 上面疊一個關卡編號
        auto txt = std::make_shared<Util::GameObject>();
        auto drawable = std::make_shared<Util::Text>(
            RESOURCE_DIR "/Font/Montserrat-Bold.ttf", 28,
            std::to_string(i + 1),
            Util::Color(0, 0, 0, 255)
        );
        txt->SetDrawable(drawable);
        txt->m_Transform.translation = glm::vec2(x, y);
        txt->SetZIndex(6);
        m_LevelButtonTexts.push_back(txt);
        m_Root.AddChild(txt);
    }

    // Level Accomplished UI
    m_LevelAccomplishedText = std::make_shared<Util::Text>(
        RESOURCE_DIR "/Font/Montserrat-Bold.ttf", 48,
        "Level Accomplished",
        Util::Color(255, 255, 0, 255)
    );
    m_LevelAccomplishedObj = std::make_shared<Util::GameObject>();
    m_LevelAccomplishedObj->SetDrawable(m_LevelAccomplishedText);
    m_LevelAccomplishedObj->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);
    m_LevelAccomplishedObj->SetZIndex(100);
    m_Root.AddChild(m_LevelAccomplishedObj);
    // 2. 載入角色
    m_Character = std::make_shared<Util::GameObject>();
    m_Character->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/Character.png"));
    
    // 將角色位置設定為 glm::vec2(52.0f, -129.0f)
    m_Character->m_Transform.translation = glm::vec2(52.0f, -129.0f); 
    
    m_Character->SetZIndex(0);

    m_Character->m_Transform.scale = glm::vec2(0.6f, 0.6f);

    // 確保只建立「一顆」初始球
    m_Balls.clear(); 
    auto firstBall = std::make_shared<Ball>(RESOURCE_DIR "/Image/NormalBall.png");
    firstBall->SetPosition(glm::vec2(0.0f, -250.0f)); 
    firstBall->SetVelocity(glm::vec2(0.0f, 0.0f));
    firstBall->SetZIndex(10);
    
    m_Root.AddChild(firstBall);
    m_Balls.push_back(firstBall);

    m_LevelManager = std::make_unique<LevelManager>(m_Root);

    // 3. 準備無限模式按鈕
    m_BtnEndless = std::make_shared<Util::GameObject>();
    m_BtnEndless->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/MainGameMode_Button.png"));
    m_BtnEndless->m_Transform.translation = glm::vec2(0, -200);
    m_BtnEndless->SetZIndex(5);

    // 4. 準備關卡模式按鈕
    m_BtnLevel = std::make_shared<Util::GameObject>();
    m_BtnLevel->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/Blue_Button.png"));
    m_BtnLevel->m_Transform.translation = glm::vec2(0, -350);
    m_BtnLevel->SetZIndex(5);
    
    m_LevelIntroObj = std::make_shared<Util::GameObject>();
    m_LevelIntroObj->SetZIndex(50);   // 蓋過其他 UI
    m_LevelIntroObj->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);
    m_Root.AddChild(m_LevelIntroObj);

    m_PropsIntroObj = std::make_shared<Util::GameObject>();
    m_PropsIntroObj->SetZIndex(51);   // 比 LevelIntro 高一層
    m_PropsIntroObj->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);
    m_Root.AddChild(m_PropsIntroObj);

    // 將所有物件加入 Root
    m_Root.AddChild(m_MainMenuBG);
    m_Root.AddChild(m_BtnEndless);
    m_Root.AddChild(m_BtnLevel);

    // 製作瞄準線的小點
    m_AimDot = std::make_shared<Util::GameObject>();
    m_AimDot->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/NormalBall.png")); 
    m_AimDot->SetZIndex(9); // 放在球的下面一點點
    m_AimDot->m_Transform.scale = glm::vec2(0.15f, 0.15f); // 縮放成原本的 15% 大小

    // 道具購買按鈕（4 顆，下方一排，y = -350）
    // 圖片素材皆為 80x80
    m_BtnDrillProp = std::make_shared<Util::GameObject>();
    m_BtnDrillProp->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/Drill_Prop.png"));
    m_BtnDrillProp->m_Transform.translation = glm::vec2(-180.0f, -350.0f);
    m_BtnDrillProp->SetZIndex(5);

    m_BtnHorizontalLaserProp = std::make_shared<Util::GameObject>();
    m_BtnHorizontalLaserProp->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/HorizontalDismantle_Prop.png"));
    m_BtnHorizontalLaserProp->m_Transform.translation = glm::vec2(-60.0f, -350.0f);
    m_BtnHorizontalLaserProp->SetZIndex(5);

    m_BtnRainbowProp = std::make_shared<Util::GameObject>();
    m_BtnRainbowProp->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/Rainbow_Prop.png"));
    m_BtnRainbowProp->m_Transform.translation = glm::vec2(60.0f, -350.0f);
    m_BtnRainbowProp->SetZIndex(5);

    m_BtnTntProp = std::make_shared<Util::GameObject>();
    m_BtnTntProp->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/TNT_Prop.png"));
    m_BtnTntProp->m_Transform.translation = glm::vec2(180.0f, -350.0f);
    m_BtnTntProp->SetZIndex(5);

    m_Root.AddChild(m_BtnDrillProp);
    m_Root.AddChild(m_BtnHorizontalLaserProp);
    m_Root.AddChild(m_BtnRainbowProp);
    m_Root.AddChild(m_BtnTntProp);

    // 警告文字（畫面正中間，2 秒後自動消失）
    m_WarningText = std::make_shared<Util::Text>(
        RESOURCE_DIR "/Font/Montserrat-Bold.ttf", 32, " ",
        Util::Color(255, 80, 80, 255)   // 紅色比較顯眼
    );

    auto warningObj = std::make_shared<Util::GameObject>();
    warningObj->SetDrawable(m_WarningText);
    warningObj->SetZIndex(85);
    warningObj->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f); // 一開始就藏起來
    m_WarningObj = warningObj;   // 需要在 hpp 補一行
    m_Root.AddChild(m_WarningObj);

    // TNT 拖曳時的跟隨圖示
    m_DragTntIcon = std::make_shared<Util::GameObject>();
    m_DragTntIcon->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/TNT_Prop.png"));
    m_DragTntIcon->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f); // 預設藏在畫面外
    m_DragTntIcon->SetZIndex(60);
    m_Root.AddChild(m_DragTntIcon);

    // 回收按鈕（720x200 → scale 0.3 約 216x60）
    m_BtnRecycle = std::make_shared<Util::GameObject>();
    m_BtnRecycle->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/Recycle.png"));
    m_BtnRecycle->m_Transform.translation = glm::vec2(-50.0f, -430.0f);
    m_BtnRecycle->m_Transform.scale = glm::vec2(0.3f, 0.3f);
    m_BtnRecycle->SetZIndex(5);
    m_Root.AddChild(m_BtnRecycle);

    // 加速按鈕：內外兩層
    m_BtnSpeedUpIn = std::make_shared<Util::GameObject>();
    m_BtnSpeedUpIn->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/SpeedUp_In.png"));
    m_BtnSpeedUpIn->m_Transform.translation = glm::vec2(200.0f, -430.0f);
    m_BtnSpeedUpIn->SetZIndex(5);                   // 內層
    m_Root.AddChild(m_BtnSpeedUpIn);

    m_BtnSpeedUpOut = std::make_shared<Util::GameObject>();
    m_BtnSpeedUpOut->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/SpeedUp_Out.png"));
    m_BtnSpeedUpOut->m_Transform.translation = glm::vec2(200.0f, -430.0f);
    m_BtnSpeedUpOut->SetZIndex(6);                  // 外層在上 (才轉得到)
    m_Root.AddChild(m_BtnSpeedUpOut);

    // 初始化 Game Over 畫面用的道具

    // 半透明黑色遮罩
    m_GameOverBG = std::make_shared<Util::GameObject>();
    
    m_GameOverBG->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/GameOver.png"));
    
    m_GameOverBG->SetZIndex(90); 
    
    // 一樣把它放大覆蓋全螢幕 (因為本來就是黑的，放大一萬倍還是黑的)
    m_GameOverBG->m_Transform.scale = glm::vec2(100.0f, 100.0f);

    // 2. GAMEOVER 白色文字
    m_GameOverText = std::make_shared<Util::GameObject>();
    m_GameOverText->SetDrawable(std::make_shared<Util::Text>(
        RESOURCE_DIR "/Font/Montserrat-Bold.ttf", 
        48,                 
        "GAMEOVER",         
        Util::Color(255, 255, 255, 255) 
    ));
    m_GameOverText->SetZIndex(95);

    m_State = GameState::GAMEPLAY_ENDLESS;
}

void App::Init() {
    LOG_DEBUG("=== 進入 App::Init ===");

    // 1. 初始化 20 關管理器
    m_LevelManager = std::make_unique<LevelManager>(m_Root);
    LOG_DEBUG("LevelManager 初始化完成");
    // 如果你想一開始就載入第一關
    // m_LevelManager->StartLevel(1);

    // 2. 初始化選球畫面的 4 種球
    std::vector<std::string> ballNames = {"NormalBall.png", "LargeBall.png", "SmallBall.png", "TinyBall.png"};
    for (int i = 0; i < 4; ++i) {
        auto ballOption = std::make_shared<Util::GameObject>();
        // 使用真實存在的圖片檔名
        ballOption->SetDrawable(std::make_shared<Util::Image>(RESOURCE_DIR "/Image/" + ballNames[i]));
        
        // 把球橫向排開在畫面上
        ballOption->m_Transform.translation = glm::vec2(0, 0);
        ballOption->SetZIndex(10); // 確保顯示在最上層
        
        m_BallOptions.push_back(ballOption);
    }

    // 3. 初始化主畫面 UI
    m_TitleText = std::make_shared<Util::Text>(
        RESOURCE_DIR "/Font/Montserrat-Bold.ttf", 48, "BBTAN CLONE", Util::Color(255, 255, 255, 255));

    
    // 4. 設定背景或其他裝飾...
    LOG_INFO("App Initialized Successfully!");

    m_State = GameState::MAIN_MENU;

    LOG_DEBUG("=== 離開 App::Init ===");

}

void App::ShowWarning(const std::string& msg) {
    if (m_WarningText) m_WarningText->SetText(msg);
    if (m_WarningObj)  m_WarningObj->m_Transform.translation = glm::vec2(0.0f, 0.0f); // 顯示在中央
    m_WarningTimer = 2.0f;
}

void App::RefreshTopUI() {
    // 金幣
    if (m_CoinTextDrawable) {
        m_CoinTextDrawable->SetText(std::to_string(m_Coins));
    }
    // 回合
    if (m_RoundTextDrawable) {
        m_RoundTextDrawable->SetText("Round " + std::to_string(m_CurrentRound));
    }
    // 破紀錄就更新最高紀錄
    if (m_CurrentRound > m_HighScore) {
        m_HighScore = m_CurrentRound;
    }
    if (m_TopTextDrawable) {
        m_TopTextDrawable->SetText("TOP " + std::to_string(m_HighScore));
    }
}

void App::RecycleBalls() {
    if (!m_IsRoundActive || m_IsRecycling) return;

    m_IsRecycling = true;
    // 如果決定下一回合起點的球還沒落下，用該回合的發射位置
    m_RecycleTarget = m_IsFirstBallReturned ? m_NextLaunchPos : m_RoundStartPos;

    // 停止連發（還沒射出的球直接傳到目標）
    m_IsFiringSequence = false;
    for (int i = m_BallsFiredCount; i < (int)m_Balls.size(); ++i) {
        if (m_Balls[i]) {
            m_Balls[i]->SetPosition(m_RecycleTarget);
            m_Balls[i]->SetVelocity(glm::vec2(0.0f, 0.0f));
        }
    }
}

bool App::HasLevelIntro(int level) const {
    // 有介紹的關卡：2-10, 15, 20
    if (level >= 2 && level <= 10) return true;
    if (level == 15) return true;
    if (level == 20) return true;
    return false;
}

void App::ShowLevelIntro(int level) {
    std::string path = std::string(RESOURCE_DIR) + "/Image/level"
                     + (level < 10 ? "0" : "") + std::to_string(level)
                     + "_intro.png";
    m_LevelIntroObj->SetDrawable(std::make_shared<Util::Image>(path));
    m_LevelIntroObj->m_Transform.translation = glm::vec2(0.0f, 0.0f);
}

void App::EnterLevel(int level) {
    m_CurrentRound = 1;
    m_PendingExtraBalls = 0;

    // 重置球
    for (auto& ball : m_Balls) m_Root.RemoveChild(ball);
    m_Balls.clear();

    int initialBalls = (level == 20) ? 40 : 1;
    for (int b = 0; b < initialBalls; ++b) {
        auto ball = std::make_shared<Ball>(RESOURCE_DIR "/Image/NormalBall.png");
        ball->SetPosition(glm::vec2(0.0f, -250.0f));
        ball->SetZIndex(10);
        m_Root.AddChild(ball);
        m_Balls.push_back(ball);
    }
    m_NextLaunchPos = glm::vec2(0.0f, -250.0f);
    m_RoundStartPos = glm::vec2(0.0f, -250.0f);

    if (m_LevelIntroObj) {
        m_LevelIntroObj->m_Transform.translation = glm::vec2(-9999.0f, -9999.0f);
    }

    m_LevelManager->StartLevel(level);

    // Level 1 首次進入 → 顯示道具介紹
    if (level == 1 && !m_PropsIntroShownLevel) {
        m_PropsIntroShownLevel = true;
        ShowPropsIntro(true);
        m_State = GameState::PROPS_INTRO;
    } else {
        m_State = GameState::GAMEPLAY_LEVEL;
    }
}

void App::ShowPropsIntro(bool isLevelMode) {
    std::string path = std::string(RESOURCE_DIR) + "/Image/"
                     + (isLevelMode ? "props_level_mode.png" : "props_infinite_mode.png");
    m_PropsIntroObj->SetDrawable(std::make_shared<Util::Image>(path));
    m_PropsIntroObj->m_Transform.translation = glm::vec2(0.0f, 0.0f);
    m_PropsIntroObj->SetZIndex(51);
}

void App::SpawnLaserEffectVertical(int col, float thickness, const char* imageName) {
    const float GRID_START_X = -210.0f;
    const float SLOT_SIZE = 70.0f;
    float colX = GRID_START_X + col * SLOT_SIZE;
    
    char path[256];
    snprintf(path, sizeof(path), "%s/Image/%s", RESOURCE_DIR, imageName);
    LOG_INFO("Spawning effect, path={}", path);

    auto fx = std::make_shared<EffectLine>();
    fx->Setup(
        glm::vec2(colX, 65.0f),
        590.0f,
        true,
        thickness,
        std::string(path)
    );
    fx->SetLifetime(0.15f);
    m_Root.AddChild(fx);
    m_LaserEffects.push_back(fx);
}

void App::SpawnLaserEffectHorizontal(int row, float thickness, const char* imageName) {
    const float GRID_START_Y = 360.0f;
    const float SLOT_SIZE = 70.0f;
    float rowY = GRID_START_Y - row * SLOT_SIZE;
    
    char path[256];
    snprintf(path, sizeof(path), "%s/Image/%s", RESOURCE_DIR, imageName);
    LOG_INFO("Spawning effect, path={}", path);

    auto fx = std::make_shared<EffectLine>();
    fx->Setup(
        glm::vec2(0.0f, rowY),
        490.0f,
        false,
        thickness,
        std::string(path)
    );
    fx->SetLifetime(0.15f);
    m_Root.AddChild(fx);
    m_LaserEffects.push_back(fx);
}