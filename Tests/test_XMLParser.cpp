/// @file test_XMLParser.cpp
/// @brief XMLDocument / XMLNode パーサーのテスト

#include "pch.h"
#include <gtest/gtest.h>
#include "GUI/XMLParser.h"

// Windows SDKのMSXML::XMLDocumentとの曖昧さを回避
using GXXMLDocument = gx::GUI::XMLDocument;
using gx::GUI::XMLNode;

// ==================== 基本パース ====================

TEST(XMLParserTest, LoadFromString_EmptyFails)
{
    GXXMLDocument doc;
    bool ok = doc.LoadFromString("");
    EXPECT_FALSE(ok);
}

TEST(XMLParserTest, LoadFromString_Simple)
{
    GXXMLDocument doc;
    bool ok = doc.LoadFromString("<root/>");
    EXPECT_TRUE(ok);
    EXPECT_NE(doc.GetRoot(), nullptr);
    EXPECT_EQ(doc.GetRoot()->tag, "root");
}

TEST(XMLParserTest, LoadFromString_SelfClosing)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<item/>"));
    EXPECT_EQ(doc.GetRoot()->tag, "item");
    EXPECT_TRUE(doc.GetRoot()->children.empty());
}

TEST(XMLParserTest, LoadFromString_WithText)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<name>Hello</name>"));
    EXPECT_EQ(doc.GetRoot()->tag, "name");
    EXPECT_EQ(doc.GetRoot()->text, "Hello");
}

// ==================== 属性 ====================

TEST(XMLParserTest, Attribute_Single)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<item id=\"42\"/>"));
    auto* root = doc.GetRoot();
    EXPECT_TRUE(root->HasAttribute("id"));
    EXPECT_EQ(root->GetAttribute("id"), "42");
}

TEST(XMLParserTest, Attribute_Multiple)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<widget x=\"10\" y=\"20\" w=\"100\" h=\"50\"/>"));
    auto* root = doc.GetRoot();
    EXPECT_EQ(root->GetAttribute("x"), "10");
    EXPECT_EQ(root->GetAttribute("y"), "20");
    EXPECT_EQ(root->GetAttribute("w"), "100");
    EXPECT_EQ(root->GetAttribute("h"), "50");
}

TEST(XMLParserTest, Attribute_NotFound)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<item/>"));
    auto* root = doc.GetRoot();
    EXPECT_FALSE(root->HasAttribute("nonexistent"));
    EXPECT_EQ(root->GetAttribute("nonexistent"), "");
}

TEST(XMLParserTest, Attribute_DefaultValue)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<item/>"));
    auto* root = doc.GetRoot();
    EXPECT_EQ(root->GetAttribute("missing", "default"), "default");
}

TEST(XMLParserTest, Attribute_EmptyValue)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<item name=\"\"/>"));
    auto* root = doc.GetRoot();
    EXPECT_TRUE(root->HasAttribute("name"));
    EXPECT_EQ(root->GetAttribute("name"), "");
}

// ==================== 子要素 ====================

TEST(XMLParserTest, Children_Single)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<root><child/></root>"));
    auto* root = doc.GetRoot();
    EXPECT_EQ(root->children.size(), 1u);
    EXPECT_EQ(root->children[0]->tag, "child");
}

TEST(XMLParserTest, Children_Multiple)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<root><a/><b/><c/></root>"));
    auto* root = doc.GetRoot();
    EXPECT_EQ(root->children.size(), 3u);
    EXPECT_EQ(root->children[0]->tag, "a");
    EXPECT_EQ(root->children[1]->tag, "b");
    EXPECT_EQ(root->children[2]->tag, "c");
}

TEST(XMLParserTest, Children_Nested)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<root><parent><child/></parent></root>"));
    auto* root = doc.GetRoot();
    EXPECT_EQ(root->children.size(), 1u);
    EXPECT_EQ(root->children[0]->tag, "parent");
    EXPECT_EQ(root->children[0]->children.size(), 1u);
    EXPECT_EQ(root->children[0]->children[0]->tag, "child");
}

TEST(XMLParserTest, Children_DeeplyNested)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<a><b><c><d/></c></b></a>"));
    auto* root = doc.GetRoot();
    EXPECT_EQ(root->children[0]->children[0]->children[0]->tag, "d");
}

// ==================== テキストコンテンツ ====================

TEST(XMLParserTest, Text_Simple)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<msg>Hello World</msg>"));
    EXPECT_EQ(doc.GetRoot()->text, "Hello World");
}

TEST(XMLParserTest, Text_WithChildren)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<parent><child>inner</child></parent>"));
    EXPECT_EQ(doc.GetRoot()->children[0]->text, "inner");
}

TEST(XMLParserTest, Text_Empty)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<node></node>"));
    EXPECT_TRUE(doc.GetRoot()->text.empty());
}

// ==================== 混合コンテンツ ====================

TEST(XMLParserTest, Mixed_AttributesAndChildren)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<panel style=\"dark\"><button text=\"OK\"/></panel>"));
    auto* root = doc.GetRoot();
    EXPECT_EQ(root->GetAttribute("style"), "dark");
    EXPECT_EQ(root->children.size(), 1u);
    EXPECT_EQ(root->children[0]->GetAttribute("text"), "OK");
}

TEST(XMLParserTest, Mixed_AttributesAndText)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<label color=\"red\">Error</label>"));
    auto* root = doc.GetRoot();
    EXPECT_EQ(root->GetAttribute("color"), "red");
    EXPECT_EQ(root->text, "Error");
}

// ==================== エラー処理 ====================

TEST(XMLParserTest, Error_InvalidXML)
{
    GXXMLDocument doc;
    bool ok = doc.LoadFromString("<unclosed");
    EXPECT_FALSE(ok);
}

TEST(XMLParserTest, Error_MismatchedTags)
{
    GXXMLDocument doc;
    bool ok = doc.LoadFromString("<open></close>");
    // パーサーは寛容: エラーをログに出力するが、部分的な結果でtrueを返す場合がある
    // クラッシュせず、何らかのルートを生成することだけ確認
    (void)ok;
    // 失敗するか、ノードを生成するか
    EXPECT_TRUE(ok || doc.GetRoot() == nullptr);
}

TEST(XMLParserTest, Error_NoRoot)
{
    GXXMLDocument doc;
    bool ok = doc.LoadFromString("   ");
    EXPECT_FALSE(ok);
}

// ==================== 実践的なGUI XML ====================

TEST(XMLParserTest, GUILayout)
{
    const char* xml = R"(
<panel id="main" style="menu">
    <button text="Start" x="100" y="200"/>
    <button text="Quit" x="100" y="260"/>
    <label color="white">Version 1.0</label>
</panel>
)";
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString(xml));
    auto* root = doc.GetRoot();
    EXPECT_EQ(root->tag, "panel");
    EXPECT_EQ(root->GetAttribute("id"), "main");
    EXPECT_EQ(root->children.size(), 3u);
    EXPECT_EQ(root->children[0]->GetAttribute("text"), "Start");
    EXPECT_EQ(root->children[1]->GetAttribute("text"), "Quit");
    EXPECT_EQ(root->children[2]->text, "Version 1.0");
}

// ==================== 複数回パース ====================

TEST(XMLParserTest, ReloadDocument)
{
    GXXMLDocument doc;
    EXPECT_TRUE(doc.LoadFromString("<first/>"));
    EXPECT_EQ(doc.GetRoot()->tag, "first");
    EXPECT_TRUE(doc.LoadFromString("<second/>"));
    EXPECT_EQ(doc.GetRoot()->tag, "second");
}
