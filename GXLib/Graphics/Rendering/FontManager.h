#pragma once
/// @file FontManager.h
/// @brief 繝輔か繝ｳ繝医・繝阪・繧ｸ繝｣繝ｼ 窶・DirectWrite縺ｫ繧医ｋ繝輔か繝ｳ繝医Λ繧ｹ繧ｿ繝ｩ繧､繧ｺ縺ｨ繝・け繧ｹ繝√Ε繧｢繝医Λ繧ｹ
///
/// 縲仙・蟄ｦ閠・髄縺題ｧ｣隱ｬ縲・
/// 繝・く繧ｹ繝医ｒ逕ｻ髱｢縺ｫ謠冗判縺吶ｋ縺ｫ縺ｯ縲∵枚蟄励・蠖｢・医げ繝ｪ繝包ｼ峨ｒ逕ｻ蜒上↓螟画鋤縺吶ｋ蠢・ｦ√′縺ゅｊ縺ｾ縺吶・
/// DirectWrite縺ｯWindows讓呎ｺ悶・繝・く繧ｹ繝医Ξ繝ｳ繝繝ｪ繝ｳ繧ｰAPI縺ｧ縲・ｫ伜刀雉ｪ縺ｪ譁・ｭ玲緒逕ｻ縺悟庄閭ｽ縺ｧ縺吶・
///
/// FontManager縺ｯ莉･荳九・謇矩・〒繝・く繧ｹ繝域緒逕ｻ繧呈ｺ門ｙ縺励∪縺呻ｼ・
/// 1. DirectWrite縺ｧ繝輔か繝ｳ繝医ｒ菴懈・
/// 2. 蜷・枚蟄暦ｼ医げ繝ｪ繝包ｼ峨ｒCPU蛛ｴ縺ｮ繝薙ャ繝医・繝・・縺ｫ繝ｩ繧ｹ繧ｿ繝ｩ繧､繧ｺ
/// 3. 蜈ｨ繧ｰ繝ｪ繝輔ｒ1譫壹・螟ｧ縺阪↑繝・け繧ｹ繝√Ε・医い繝医Λ繧ｹ・峨↓縺ｾ縺ｨ繧√ｋ
/// 4. 繝・け繧ｹ繝√Ε繧｢繝医Λ繧ｹ繧竪PU縺ｫ繧｢繝・・繝ｭ繝ｼ繝・
/// 5. 蜷・げ繝ｪ繝輔・UV蠎ｧ讓吶ｒ險倬鹸
///
/// 繝・け繧ｹ繝√Ε繧｢繝医Λ繧ｹ縺ｨ縺ｯ縲∬､・焚縺ｮ蟆上＆縺ｪ逕ｻ蜒上ｒ1譫壹・繝・け繧ｹ繝√Ε縺ｫ驟咲ｽｮ縺吶ｋ謚陦薙〒縺吶・
/// 繝・け繧ｹ繝√Ε縺ｮ蛻・ｊ譖ｿ縺医・GPU縺ｫ縺ｨ縺｣縺ｦ繧ｳ繧ｹ繝医′鬮倥＞縺溘ａ縲√い繝医Λ繧ｹ縺ｫ縺ｾ縺ｨ繧√ｋ縺薙→縺ｧ
/// 蜉ｹ邇・噪縺ｪ謠冗判縺悟庄閭ｽ縺ｫ縺ｪ繧翫∪縺吶・

#include "pch.h"
#include "Graphics/Resource/TextureManager.h"
#include <wincodec.h>
#include <dwrite_1.h>

namespace GX
{

/// @brief 繧ｰ繝ｪ繝包ｼ・譁・ｭ怜・・峨・謠冗判諠・ｱ
struct GlyphInfo
{
    float u0, v0;       ///< 繧｢繝医Λ繧ｹ荳翫・蟾ｦ荳涯V
    float u1, v1;       ///< 繧｢繝医Λ繧ｹ荳翫・蜿ｳ荳偽V
    int width;          ///< 繧ｰ繝ｪ繝輔・蟷・ｼ医ヴ繧ｯ繧ｻ繝ｫ・・
    int height;         ///< 繧ｰ繝ｪ繝輔・鬮倥＆・医ヴ繧ｯ繧ｻ繝ｫ・・
    int offsetX;        ///< 繝吶・繧ｹ繝ｩ繧､繝ｳ縺九ｉ縺ｮX譁ｹ蜷代が繝輔そ繝・ヨ
    int offsetY;        ///< 繝吶・繧ｹ繝ｩ繧､繝ｳ縺九ｉ縺ｮY譁ｹ蜷代が繝輔そ繝・ヨ
    float advance;      ///< 谺｡縺ｮ譁・ｭ励∈縺ｮ豌ｴ蟷ｳ遘ｻ蜍暮㍼
};

/// @brief 繝輔か繝ｳ繝医・繝阪・繧ｸ繝｣繝ｼ
class FontManager
{
public:
    /// @brief 邂｡逅・〒縺阪ｋ繝輔か繝ｳ繝医・譛螟ｧ謨ｰ
    static constexpr uint32_t k_MaxFonts = 64;
    /// @brief 繧｢繝医Λ繧ｹ繝・け繧ｹ繝√Ε縺ｮ繧ｵ繧､繧ｺ・医ヴ繧ｯ繧ｻ繝ｫ・・
    static constexpr uint32_t k_AtlasSize = 2048;

    FontManager() = default;
    ~FontManager() = default;

    /// @brief 繝輔か繝ｳ繝医・繝阪・繧ｸ繝｣繝ｼ繧貞・譛溷喧縺吶ｋ
    /// @param device D3D12繝・ヰ繧､繧ｹ縺ｸ縺ｮ繝昴う繝ｳ繧ｿ
    /// @param textureManager 繝・け繧ｹ繝√Ε繝槭ロ繝ｼ繧ｸ繝｣繝ｼ縺ｸ縺ｮ繝昴う繝ｳ繧ｿ
    /// @return 蛻晄悄蛹悶↓謌仙粥縺励◆蝣ｴ蜷・rue
    bool Initialize(ID3D12Device* device, TextureManager* textureManager);

    /// @brief 繝輔か繝ｳ繝医ｒ菴懈・縺励※繝上Φ繝峨Ν繧定ｿ斐☆
    /// @param fontName 繝輔か繝ｳ繝亥錐・井ｾ・ L"MS Gothic"・・
    /// @param fontSize 繝輔か繝ｳ繝医し繧､繧ｺ・医ヴ繧ｯ繧ｻ繝ｫ・・
    /// @param bold 螟ｪ蟄励↓縺吶ｋ縺九←縺・°
    /// @param italic 譁應ｽ薙↓縺吶ｋ縺九←縺・°
    /// @return 繝輔か繝ｳ繝医ワ繝ｳ繝峨Ν・亥､ｱ謨玲凾縺ｯ-1・・
    int CreateFont(const std::wstring& fontName, int fontSize, bool bold = false, bool italic = false);

    /// @brief 繧ｰ繝ｪ繝包ｼ・譁・ｭ怜・縺ｮ謠冗判・画ュ蝣ｱ繧貞叙蠕励☆繧具ｼ域悴遏･譁・ｭ励・閾ｪ蜍慕噪縺ｫ繝ｩ繧ｹ繧ｿ繝ｩ繧､繧ｺ・・
    /// @param fontHandle 繝輔か繝ｳ繝医ワ繝ｳ繝峨Ν
    /// @param ch 蜿門ｾ励☆繧区枚蟄・
    /// @return 繧ｰ繝ｪ繝墓ュ蝣ｱ縺ｸ縺ｮ繝昴う繝ｳ繧ｿ・亥､ｱ謨玲凾縺ｯnullptr・・
    const GlyphInfo* GetGlyphInfo(int fontHandle, wchar_t ch);

    /// @brief 繝輔か繝ｳ繝医・繧｢繝医Λ繧ｹ繝・け繧ｹ繝√Ε繝上Φ繝峨Ν繧貞叙蠕励☆繧・
    /// @param fontHandle 繝輔か繝ｳ繝医ワ繝ｳ繝峨Ν
    /// @return 繝・け繧ｹ繝√Ε繝上Φ繝峨Ν・亥､ｱ謨玲凾縺ｯ-1・・
    int GetAtlasTextureHandle(int fontHandle) const;

    /// @brief 繝輔か繝ｳ繝医・陦後・鬮倥＆繧貞叙蠕励☆繧・
    /// @param fontHandle 繝輔か繝ｳ繝医ワ繝ｳ繝峨Ν
    /// @return 陦後・鬮倥＆・医ヴ繧ｯ繧ｻ繝ｫ・・
    int GetLineHeight(int fontHandle) const;

    /// @brief 上側の余白を詰めるための縦オフセットを取得
    /// 初学者向け: 文字が少し下に寄るのを防ぐための調整値です。
    float GetCapOffset(int fontHandle) const;

    /// @brief dirty縺ｪ繧｢繝医Λ繧ｹ繧竪PU縺ｫ繧｢繝・・繝ｭ繝ｼ繝峨☆繧具ｼ医ヵ繝ｬ繝ｼ繝蠅・阜縺ｧ蜻ｼ縺ｶ・・
    void FlushAtlasUpdates();

    /// @brief 邨ゆｺ・・逅・ｒ陦後＞蜈ｨ繝ｪ繧ｽ繝ｼ繧ｹ繧定ｧ｣謾ｾ縺吶ｋ
    void Shutdown();

private:
    /// 繝輔か繝ｳ繝医お繝ｳ繝医Μ
    struct FontEntry
    {
        ComPtr<IDWriteTextFormat>  textFormat;
        std::unordered_map<wchar_t, GlyphInfo> glyphs;
        std::vector<uint32_t> atlasPixels;
        int atlasTextureHandle = -1;
        int fontSize = 0;
        int lineHeight = 0;
        float baseline = 0.0f;
        float capOffset = 0.0f;
        int cursorX = 0;       ///< 繧｢繝医Λ繧ｹ縺ｮ迴ｾ蝨ｨ縺ｮ譖ｸ縺崎ｾｼ縺ｿX菴咲ｽｮ
        int cursorY = 0;       ///< 繧｢繝医Λ繧ｹ縺ｮ迴ｾ蝨ｨ縺ｮ譖ｸ縺崎ｾｼ縺ｿY菴咲ｽｮ
        int rowHeight = 0;     ///< 迴ｾ蝨ｨ縺ｮ陦後・譛螟ｧ鬮倥＆
        bool valid = false;
        bool atlasDirty = false; ///< CPU蛛ｴ繝斐け繧ｻ繝ｫ縺梧峩譁ｰ縺輔ｌ縺溘′GPU譛ｪ蜿肴丐
    };

    /// 1譁・ｭ励ｒ繝ｩ繧ｹ繧ｿ繝ｩ繧､繧ｺ縺励※繧｢繝医Λ繧ｹ縺ｫ霑ｽ蜉
    bool RasterizeGlyph(FontEntry& entry, wchar_t ch);

    /// 繧｢繝医Λ繧ｹ繝・け繧ｹ繝√Ε繧竪PU縺ｫ・亥・・峨い繝・・繝ｭ繝ｼ繝・
    void UploadAtlas(FontEntry& entry);

    /// ASCII蝓ｺ譛ｬ譁・ｭ励ｒ荳諡ｬ繝ｩ繧ｹ繧ｿ繝ｩ繧､繧ｺ
    void RasterizeBasicChars(FontEntry& entry);

    int AllocateHandle();

    ID3D12Device*   m_device = nullptr;
    TextureManager* m_textureManager = nullptr;

    ComPtr<IDWriteFactory>      m_dwriteFactory;
    ComPtr<ID2D1Factory>        m_d2dFactory;
    ComPtr<IWICImagingFactory>  m_wicFactory;

    bool m_comInitialized = false;

    std::vector<FontEntry> m_entries;
    std::vector<int>       m_freeHandles;
    int                    m_nextHandle = 0;
};

} // namespace GX

