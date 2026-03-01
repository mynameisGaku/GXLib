#include "GXLib.h"

// ============================================================================
// GXLib ゲームテンプレート
//
// このファイルを自由に書き換えて、ゲームを作ってみよう！
//
// よく使う関数:
//   DrawGraph(x, y, handle, TRUE)     — 画像を描画
//   DrawCircle(x, y, r, color, TRUE)  — 円を描画
//   DrawBox(x1, y1, x2, y2, color, TRUE)  — 四角を描画
//   DrawString(x, y, "文字", color)   — 文字を描画
//   CheckHitKey(KEY_INPUT_SPACE)      — キーが押されているか
//   GetMousePoint(&mx, &my)           — マウス座標を取得
//   LoadGraph("画像.png")             — 画像を読み込み
//   PlaySoundMem(handle, GX_PLAYTYPE_BACK) — 音を鳴らす
// ============================================================================

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // --- 初期設定 ---
    ChangeWindowMode(TRUE);
    SetGraphMode(1280, 720, 32);
    SetMainWindowText("テスト");

    if (GX_Init() == -1) return -1;
    SetDrawScreen(GX_SCREEN_BACK);

    // --- ここにゲームの初期化を書く ---
    float x = 640.0f;
    float y = 360.0f;
    float speed = 5.0f;
    unsigned int white = GetColor(255, 255, 255);
    unsigned int red   = GetColor(255, 100, 100);

    // --- メインループ ---
    while (ProcessMessage() == 0)
    {
        // ESCキーで終了
        if (CheckHitKey(KEY_INPUT_ESCAPE)) break;
        gx::Vector2 vel{};
        // --- 更新処理 ---
        if (CheckHitKey(KEY_INPUT_A)) 
        {
            vel.x -= speed * GetDeltaTime();
        }
        if (CheckHitKey(KEY_INPUT_D)) 
        {
            vel.x += speed * GetDeltaTime();
        }
        if (CheckHitKey(KEY_INPUT_W)) 
        {
            vel.y -= speed * GetDeltaTime();
        }
        if (CheckHitKey(KEY_INPUT_S)) 
        {
            vel.y += speed * GetDeltaTime();
        }
        x += vel.x;
		y += vel.y;

        // --- 描画処理 ---
        ClearDrawScreen();

        if (CheckHitKey(KEY_INPUT_W))
        {
            DrawFormatString(450, 10, white, "W");
        }
        if (CheckHitKey(KEY_INPUT_A))
        {
            DrawFormatString(400, 50, white, "A");
        }
        if (CheckHitKey(KEY_INPUT_S))
        {
            DrawFormatString(450, 100, white, "S");
        }
        if (CheckHitKey(KEY_INPUT_D))
        {
            DrawFormatString(500, 50, white, "D");
        }

        DrawCircleF(x, y, 30, red, TRUE);
        DrawFormatString(10, 10, white, "FPS : %f", GetFPS());
        DrawFormatString(10, 40, white, "DeltaT : %f", GetDeltaTime());
        DrawString(10, 70, "矢印キー: Move    ESC: Exit", white);
        DrawFormatString(10, 100, white, "座標 : %f, %f", x, y);

        ScreenFlip();
    }

    GX_End();
    return 0;
}
