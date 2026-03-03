/// @file SamplerFeedbackAnalyze.hlsl
/// @brief サンプラーフィードバック分析コンピュートシェーダ
///
/// サンプラーフィードバックのMinMipマップを分析し、
/// 各テクスチャに必要なミップレベル情報を集約する。
/// テクスチャストリーミングの優先度決定に使用する。

// 入力: リゾルブ済みフィードバックマップ (R8_UINT)
// 各テクセルは対応するタイル領域で使用された最小ミップレベルを格納
Texture2D<uint> g_FeedbackMap : register(t0);

// 出力: テクスチャごとの集約結果
RWStructuredBuffer<uint> g_MipResults : register(u0);

// 定数バッファ
cbuffer AnalyzeConstants : register(b0)
{
    uint2 g_FeedbackSize;   // フィードバックマップの解像度（タイル数）
    uint  g_TextureIndex;   // 処理対象テクスチャのインデックス
    uint  g_MaxMipLevels;   // テクスチャの最大ミップレベル数
};

// 共有メモリで最小ミップレベルをリダクション
groupshared uint gs_MinMip;

/// @brief フィードバックマップを走査して最小使用ミップレベルを求める
///
/// 結果は g_MipResults[g_TextureIndex] に書き込まれる。
/// ストリーミングマネージャがこの値を読み取って優先度を決定する。
[numthreads(8, 8, 1)]
void CSMain(
    uint3 dispatchThreadId : SV_DispatchThreadID,
    uint  groupIndex       : SV_GroupIndex)
{
    // グループ先頭スレッドが共有メモリを初期化
    if (groupIndex == 0)
    {
        gs_MinMip = 0xFF;
    }

    GroupMemoryBarrierWithGroupSync();

    // 範囲内のテクセルを読み取り
    if (dispatchThreadId.x < g_FeedbackSize.x &&
        dispatchThreadId.y < g_FeedbackSize.y)
    {
        uint mipLevel = g_FeedbackMap[dispatchThreadId.xy];

        // 0xFFは未使用タイルを示す（シェーダがアクセスしなかった領域）
        if (mipLevel != 0xFF && mipLevel < g_MaxMipLevels)
        {
            // アトミック最小値で最も高解像度なミップリクエストを記録
            InterlockedMin(gs_MinMip, mipLevel);
        }
    }

    GroupMemoryBarrierWithGroupSync();

    // グループ先頭スレッドが結果をグローバルバッファに書き込み
    if (groupIndex == 0)
    {
        // 複数スレッドグループ間でもアトミック最小値を使用
        InterlockedMin(g_MipResults[g_TextureIndex], gs_MinMip);
    }
}
