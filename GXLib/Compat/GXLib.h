#pragma once
/// @file GXLib.h
/// @brief GXLib簡易API — このヘッダー1つで全機能を使用可能
///
/// 使い方：
/// #include "GXLib.h" を記述するだけで、
/// GXLibの簡易APIが使用可能になります。
///
/// DXLibからの移行時は、#include <DxLib.h> を
/// #include "GXLib.h" に置き換え、関数名を対応するものに変更してください。

#include "Compat/CompatTypes.h"

// ============================================================================
// システム
// ============================================================================

/// @brief GXLibを初期化する
/// @return 成功時 0、失敗時 -1
int GX_Init();

/// @brief GXLibを終了し、全リソースを解放する
/// @return 成功時 0
int GX_End();

/// @brief ウィンドウメッセージ処理と入力状態の更新を行う
/// @return 正常時 0、ウィンドウが閉じられた場合 -1
int ProcessMessage();

/// @brief メインウィンドウのタイトルバーテキストを設定する
/// @param title 表示するタイトル文字列
/// @return 成功時 0
int SetMainWindowText(const TCHAR* title);

/// @brief ウィンドウモードを切り替える
/// @param flag TRUE でウィンドウモード、FALSE でフルスクリーン
/// @return 成功時 0
int ChangeWindowMode(int flag);

/// @brief 画面解像度と色深度を設定する（GX_Init の前に呼ぶ）
/// @param width 画面幅（ピクセル）
/// @param height 画面高さ（ピクセル）
/// @param colorBitNum 色ビット数（通常 32）
/// @return 成功時 0
int SetGraphMode(int width, int height, int colorBitNum);

/// @brief RGB値からカラー値を生成する
/// @param r 赤成分（0〜255）
/// @param g 緑成分（0〜255）
/// @param b 青成分（0〜255）
/// @return 0xFFRRGGBB 形式のカラー値
unsigned int GetColor(int r, int g, int b);

/// @brief アプリケーション起動からの経過時間を取得する
/// @return 経過時間（ミリ秒）
int GetNowCount();

/// @brief 描画先スクリーンを設定する
/// @param screen 描画先（`GX_SCREEN_BACK` または `GX_SCREEN_FRONT`）
/// @return 成功時 0
int SetDrawScreen(int screen);

/// @brief 描画先スクリーンをクリアし、フレームを開始する
/// @return 成功時 0
int ClearDrawScreen();

/// @brief バックバッファの内容を画面に表示する（フレーム終了）
/// @return 成功時 0
int ScreenFlip();

/// @brief 画面クリア時の背景色を設定する
/// @param r 赤成分（0〜255）
/// @param g 緑成分（0〜255）
/// @param b 青成分（0〜255）
/// @return 成功時 0
int SetBackgroundColor(int r, int g, int b);

// ============================================================================
// 2D描画 — スプライト
// ============================================================================

/// @brief 画像ファイルを読み込み、グラフィックハンドルを返す
/// @param filePath 画像ファイルのパス
/// @return グラフィックハンドル（失敗時 -1）
int LoadGraph(const TCHAR* filePath);

/// @brief グラフィックハンドルを削除し、リソースを解放する
/// @param handle 削除するグラフィックハンドル
/// @return 成功時 0
int DeleteGraph(int handle);

/// @brief 画像を分割して複数のグラフィックハンドルに読み込む
/// @param filePath 画像ファイルのパス
/// @param allNum 分割総数
/// @param xNum 横方向の分割数
/// @param yNum 縦方向の分割数
/// @param xSize 分割1つあたりの幅（ピクセル）
/// @param ySize 分割1つあたりの高さ（ピクセル）
/// @param handleBuf ハンドルを格納する配列（allNum 個以上の要素が必要）
/// @return 成功時 0、失敗時 -1
int LoadDivGraph(const TCHAR* filePath, int allNum, int xNum, int yNum,
                 int xSize, int ySize, int* handleBuf);

/// @brief グラフィックハンドルの画像サイズを取得する
/// @param handle グラフィックハンドル
/// @param width 幅の格納先
/// @param height 高さの格納先
/// @return 成功時 0
int GetGraphSize(int handle, int* width, int* height);

/// @brief 画像を指定座標に描画する
/// @param x 描画先X座標（左上）
/// @param y 描画先Y座標（左上）
/// @param handle グラフィックハンドル
/// @param transFlag TRUE で透過描画を有効にする
/// @return 成功時 0
int DrawGraph(int x, int y, int handle, int transFlag);

/// @brief 画像を回転・拡縮して描画する
/// @param cx 描画中心X座標
/// @param cy 描画中心Y座標
/// @param extRate 拡大率（1.0 で等倍）
/// @param angle 回転角度（ラジアン）
/// @param handle グラフィックハンドル
/// @param transFlag TRUE で透過描画を有効にする
/// @return 成功時 0
int DrawRotaGraph(int cx, int cy, double extRate, double angle,
                  int handle, int transFlag);

/// @brief 画像を指定矩形に拡大縮小して描画する
/// @param x1 描画矩形の左上X座標
/// @param y1 描画矩形の左上Y座標
/// @param x2 描画矩形の右下X座標
/// @param y2 描画矩形の右下Y座標
/// @param handle グラフィックハンドル
/// @param transFlag TRUE で透過描画を有効にする
/// @return 成功時 0
int DrawExtendGraph(int x1, int y1, int x2, int y2, int handle, int transFlag);

/// @brief 画像の一部を切り出して描画する
/// @param x 描画先X座標
/// @param y 描画先Y座標
/// @param srcX 転送元矩形の左上X座標
/// @param srcY 転送元矩形の左上Y座標
/// @param w 転送元矩形の幅
/// @param h 転送元矩形の高さ
/// @param handle グラフィックハンドル
/// @param transFlag TRUE で透過描画を有効にする
/// @param turnFlag TRUE で左右反転する
/// @return 成功時 0
int DrawRectGraph(int x, int y, int srcX, int srcY, int w, int h,
                  int handle, int transFlag, int turnFlag = FALSE);

/// @brief 画像を自由変形して描画する（四隅の座標を個別に指定）
/// @param x1 左上X座標
/// @param y1 左上Y座標
/// @param x2 右上X座標
/// @param y2 右上Y座標
/// @param x3 右下X座標
/// @param y3 右下Y座標
/// @param x4 左下X座標
/// @param y4 左下Y座標
/// @param handle グラフィックハンドル
/// @param transFlag TRUE で透過描画を有効にする
/// @return 成功時 0
int DrawModiGraph(int x1, int y1, int x2, int y2, int x3, int y3,
                  int x4, int y4, int handle, int transFlag);

// ============================================================================
// 2D描画 — プリミティブ
// ============================================================================

/// @brief 直線を描画する
/// @param x1 始点X座標
/// @param y1 始点Y座標
/// @param x2 終点X座標
/// @param y2 終点Y座標
/// @param color 描画色（GetColor で取得）
/// @param thickness 線の太さ（ピクセル、デフォルト 1）
/// @return 成功時 0
int DrawLine(int x1, int y1, int x2, int y2, unsigned int color, int thickness = 1);

/// @brief 矩形を描画する
/// @param x1 左上X座標
/// @param y1 左上Y座標
/// @param x2 右下X座標
/// @param y2 右下Y座標
/// @param color 描画色
/// @param fillFlag TRUE で塗りつぶし、FALSE で枠線のみ
/// @return 成功時 0
int DrawBox(int x1, int y1, int x2, int y2, unsigned int color, int fillFlag);

/// @brief 円を描画する
/// @param cx 中心X座標
/// @param cy 中心Y座標
/// @param r 半径
/// @param color 描画色
/// @param fillFlag TRUE で塗りつぶし、FALSE で枠線のみ
/// @return 成功時 0
int DrawCircle(int cx, int cy, int r, unsigned int color, int fillFlag = TRUE);

/// @brief 三角形を描画する
/// @param x1 頂点1のX座標
/// @param y1 頂点1のY座標
/// @param x2 頂点2のX座標
/// @param y2 頂点2のY座標
/// @param x3 頂点3のX座標
/// @param y3 頂点3のY座標
/// @param color 描画色
/// @param fillFlag TRUE で塗りつぶし、FALSE で枠線のみ
/// @return 成功時 0
int DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3,
                 unsigned int color, int fillFlag);

/// @brief 楕円を描画する
/// @param cx 中心X座標
/// @param cy 中心Y座標
/// @param rx X方向の半径
/// @param ry Y方向の半径
/// @param color 描画色
/// @param fillFlag TRUE で塗りつぶし、FALSE で枠線のみ
/// @return 成功時 0
int DrawOval(int cx, int cy, int rx, int ry, unsigned int color, int fillFlag);

/// @brief 1ピクセルを描画する
/// @param x X座標
/// @param y Y座標
/// @param color 描画色
/// @return 成功時 0
int DrawPixel(int x, int y, unsigned int color);

// ============================================================================
// ブレンドモード・描画色
// ============================================================================

/// @brief 描画ブレンドモードとパラメータを設定する
/// @param blendMode ブレンドモード（`GX_BLENDMODE_ALPHA`, `GX_BLENDMODE_ADD` など）
/// @param blendParam ブレンドパラメータ（0〜255、アルファブレンド時は不透明度）
/// @return 成功時 0
int SetDrawBlendMode(int blendMode, int blendParam);

/// @brief 描画輝度を設定する（描画色に乗算される）
/// @param r 赤成分の輝度（0〜255）
/// @param g 緑成分の輝度（0〜255）
/// @param b 青成分の輝度（0〜255）
/// @return 成功時 0
int SetDrawBright(int r, int g, int b);

// ============================================================================
// テキスト描画（デフォルトフォント）
// ============================================================================

/// @brief デフォルトフォントで文字列を描画する
/// @param x 描画先X座標
/// @param y 描画先Y座標
/// @param str 描画する文字列
/// @param color 文字色
/// @return 成功時 0
int DrawString(int x, int y, const TCHAR* str, unsigned int color);

/// @brief デフォルトフォントで書式付き文字列を描画する
/// @param x 描画先X座標
/// @param y 描画先Y座標
/// @param color 文字色
/// @param format printf 形式のフォーマット文字列
/// @return 成功時 0
int DrawFormatString(int x, int y, unsigned int color, const TCHAR* format, ...);

/// @brief デフォルトフォントでの文字列の描画幅を取得する
/// @param str 計測する文字列
/// @param strLen 文字列の長さ（文字数）
/// @return 描画幅（ピクセル）
int GetDrawStringWidth(const TCHAR* str, int strLen);

// ============================================================================
// フォントハンドル
// ============================================================================

/// @brief フォントハンドルを作成する
/// @param fontName フォント名（例: "メイリオ"）
/// @param size フォントサイズ
/// @param thick フォントの太さ（0〜9）
/// @param fontType フォントタイプ（GX_FONTTYPE_NORMAL 等、-1 でデフォルト）
/// @return フォントハンドル（失敗時 -1）
int CreateFontToHandle(const TCHAR* fontName, int size, int thick, int fontType = -1);

/// @brief フォントハンドルを削除する
/// @param handle 削除するフォントハンドル
/// @return 成功時 0
int DeleteFontToHandle(int handle);

/// @brief 指定フォントで文字列を描画する
/// @param x 描画先X座標
/// @param y 描画先Y座標
/// @param str 描画する文字列
/// @param color 文字色
/// @param fontHandle フォントハンドル
/// @return 成功時 0
int DrawStringToHandle(int x, int y, const TCHAR* str, unsigned int color, int fontHandle);

/// @brief 指定フォントで書式付き文字列を描画する
/// @param x 描画先X座標
/// @param y 描画先Y座標
/// @param color 文字色
/// @param fontHandle フォントハンドル
/// @param format printf 形式のフォーマット文字列
/// @return 成功時 0
int DrawFormatStringToHandle(int x, int y, unsigned int color, int fontHandle,
                              const TCHAR* format, ...);

/// @brief 指定フォントでの文字列の描画幅を取得する
/// @param str 計測する文字列
/// @param strLen 文字列の長さ（文字数）
/// @param fontHandle フォントハンドル
/// @return 描画幅（ピクセル）
int GetDrawStringWidthToHandle(const TCHAR* str, int strLen, int fontHandle);

// ============================================================================
// 入力
// ============================================================================

/// @brief 指定キーが押されているか確認する
/// @param keyCode キーコード（KEY_INPUT_* 定数、DirectInput DIK_* 互換）
/// @return 押されている場合 1、押されていない場合 0
int CheckHitKey(int keyCode);

/// @brief 全キーの押下状態を配列に取得する
/// @param keyStateBuf 256要素の配列（各要素: 押下時 1、非押下時 0）
/// @return 成功時 0
int GetHitKeyStateAll(char* keyStateBuf);

/// @brief マウスボタンの押下状態をビットフラグで取得する
/// @return `MOUSE_INPUT_LEFT` / `MOUSE_INPUT_RIGHT` / `MOUSE_INPUT_MIDDLE` のビット和
int GetMouseInput();

/// @brief マウスカーソルの座標を取得する
/// @param x X座標の格納先
/// @param y Y座標の格納先
/// @return 成功時 0
int GetMousePoint(int* x, int* y);

/// @brief マウスホイールの回転量を取得する
/// @return 回転量（上方向が正）
int GetMouseWheelRotVol();

/// @brief ゲームパッドの入力状態をビットフラグで取得する
/// @param inputType パッドID（`GX_INPUT_PAD1`〜`GX_INPUT_PAD4`）
/// @return PAD_INPUT_* 定数の論理和
int GetJoypadInputState(int inputType);

// ============================================================================
// サウンド
// ============================================================================

/// @brief サウンドファイルをメモリに読み込む
/// @param filePath サウンドファイルのパス（.wav 等）
/// @return サウンドハンドル（失敗時 -1）
int LoadSoundMem(const TCHAR* filePath);

/// @brief サウンドを再生する
/// @param handle サウンドハンドル
/// @param playType 再生タイプ（`GX_PLAYTYPE_NORMAL`, `GX_PLAYTYPE_BACK`, `GX_PLAYTYPE_LOOP`）
/// @param resumeFlag TRUE で最初から再生（デフォルト）
/// @return 成功時 0
int PlaySoundMem(int handle, int playType, int resumeFlag = TRUE);

/// @brief サウンドの再生を停止する
/// @param handle サウンドハンドル
/// @return 成功時 0
int StopSoundMem(int handle);

/// @brief サウンドハンドルを削除し、リソースを解放する
/// @param handle 削除するサウンドハンドル
/// @return 成功時 0
int DeleteSoundMem(int handle);

/// @brief サウンドの音量を変更する
/// @param volume 音量（0〜255）
/// @param handle サウンドハンドル
/// @return 成功時 0
int ChangeVolumeSoundMem(int volume, int handle);

/// @brief サウンドが再生中か確認する
/// @param handle サウンドハンドル
/// @return 再生中の場合 1、停止中の場合 0
int CheckSoundMem(int handle);

/// @brief BGMファイルを再生する
/// @param filePath 音楽ファイルのパス
/// @param playType 再生タイプ（GX_PLAYTYPE_LOOP でループ再生）
/// @return 成功時 0
int PlayMusic(const TCHAR* filePath, int playType);

/// @brief BGMの再生を停止する
/// @return 成功時 0
int StopMusic();

/// @brief BGMが再生中か確認する
/// @return 再生中の場合 1、停止中の場合 0
int CheckMusic();

// ============================================================================
// 3D — カメラ
// ============================================================================

/// @brief 3Dカメラの位置と注視点を設定する
/// @param position カメラの位置
/// @param target カメラの注視点
/// @return 成功時 0
int SetCameraPositionAndTarget(VECTOR position, VECTOR target);

/// @brief 3Dカメラのニアクリップとファークリップを設定する
/// @param nearZ ニアクリップ距離
/// @param farZ ファークリップ距離
/// @return 成功時 0
int SetCameraNearFar(float nearZ, float farZ);

// ============================================================================
// 3D — モデル
// ============================================================================

/// @brief 3Dモデルファイルを読み込む（glTF/GLB対応）
/// @param filePath モデルファイルのパス
/// @return モデルハンドル（失敗時 -1）
int LoadModel(const TCHAR* filePath);

/// @brief 3Dモデルハンドルを削除し、リソースを解放する
/// @param handle 削除するモデルハンドル
/// @return 成功時 0
int DeleteModel(int handle);

/// @brief 3Dモデルを描画する
/// @param handle モデルハンドル
/// @return 成功時 0
int DrawModel(int handle);

/// @brief 3Dモデルの位置を設定する
/// @param handle モデルハンドル
/// @param position ワールド座標での位置
/// @return 成功時 0
int SetModelPosition(int handle, VECTOR position);

/// @brief 3Dモデルのスケールを設定する
/// @param handle モデルハンドル
/// @param scale 各軸のスケール値
/// @return 成功時 0
int SetModelScale(int handle, VECTOR scale);

/// @brief 3Dモデルの回転を設定する（オイラー角）
/// @param handle モデルハンドル
/// @param rotation 各軸の回転角度（ラジアン）
/// @return 成功時 0
int SetModelRotation(int handle, VECTOR rotation);

// --- 3Dモデル: マテリアル/シェーダー ---
int GetModelSubMeshCount(int handle);
int GetModelSubMeshMaterial(int handle, int subMeshIndex);
int SetModelSubMeshMaterial(int handle, int subMeshIndex, int materialHandle);
int SetModelSubMeshShader(int handle, int subMeshIndex, int shaderHandle);

int GetModelMaterialCount(int handle);
int GetModelMaterialHandle(int handle, int materialIndex);

int CreateMaterial();
int DeleteMaterial(int materialHandle);
int SetMaterialParam(int materialHandle, const GX_MATERIAL_PARAM* param);
int SetMaterialTexture(int materialHandle, int slot, int textureHandle);
int SetMaterialShader(int materialHandle, int shaderHandle);

int CreateMaterialShader(const TCHAR* vsPath, const TCHAR* psPath);

// --- 3Dモデル: アニメーション ---
int GetModelAnimationCount(int handle);
int PlayModelAnimation(int handle, int animIndex, int loop);
int CrossFadeModelAnimation(int handle, int animIndex, float duration, int loop);
int StopModelAnimation(int handle);
int UpdateModelAnimation(int handle, float deltaTime);

// ============================================================================
// 数学ユーティリティ
// ============================================================================

/// @brief VECTOR を生成する
/// @param x X成分
/// @param y Y成分
/// @param z Z成分
/// @return 生成された VECTOR
VECTOR VGet(float x, float y, float z);

/// @brief ベクトルの加算
/// @param a 左辺ベクトル
/// @param b 右辺ベクトル
/// @return a + b の結果
VECTOR VAdd(VECTOR a, VECTOR b);

/// @brief ベクトルの減算
/// @param a 左辺ベクトル
/// @param b 右辺ベクトル
/// @return a - b の結果
VECTOR VSub(VECTOR a, VECTOR b);

/// @brief ベクトルのスカラー倍
/// @param v ベクトル
/// @param scale スカラー値
/// @return v * scale の結果
VECTOR VScale(VECTOR v, float scale);

/// @brief ベクトルの内積
/// @param a 左辺ベクトル
/// @param b 右辺ベクトル
/// @return 内積の値
float  VDot(VECTOR a, VECTOR b);

/// @brief ベクトルの外積
/// @param a 左辺ベクトル
/// @param b 右辺ベクトル
/// @return a x b の結果
VECTOR VCross(VECTOR a, VECTOR b);

/// @brief ベクトルの正規化（単位ベクトル化）
/// @param v 正規化するベクトル
/// @return 正規化された単位ベクトル
VECTOR VNorm(VECTOR v);

/// @brief ベクトルの大きさ（長さ）を取得する
/// @param v ベクトル
/// @return ベクトルの長さ
float  VSize(VECTOR v);

/// @brief 単位行列を取得する
/// @return 4x4単位行列
MATRIX MGetIdent();

/// @brief 行列の乗算
/// @param a 左辺行列
/// @param b 右辺行列
/// @return a * b の結果
MATRIX MMult(MATRIX a, MATRIX b);

/// @brief X軸回りの回転行列を取得する
/// @param angle 回転角度（ラジアン）
/// @return 回転行列
MATRIX MGetRotX(float angle);

/// @brief Y軸回りの回転行列を取得する
/// @param angle 回転角度（ラジアン）
/// @return 回転行列
MATRIX MGetRotY(float angle);

/// @brief Z軸回りの回転行列を取得する
/// @param angle 回転角度（ラジアン）
/// @return 回転行列
MATRIX MGetRotZ(float angle);

/// @brief 平行移動行列を取得する
/// @param v 移動量ベクトル
/// @return 平行移動行列
MATRIX MGetTranslate(VECTOR v);
