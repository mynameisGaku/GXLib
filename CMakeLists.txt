# =============================================================================
# GXLib — DirectX 12 ゲームフレームワーク
#
# 【初学者向け解説】
# CMakeはC++プロジェクトのビルドシステムです。
# このファイルがプロジェクト全体の設定を定義し、
# サブディレクトリ（GXLib、Sandbox）をまとめています。
# =============================================================================
cmake_minimum_required(VERSION 3.24)
project(GXLib VERSION 0.1.0 LANGUAGES CXX)

# C++20を使用
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Visual Studio: UTF-8ソースファイル対応（日本語コメント必須）
add_compile_options(/utf-8)

# サブプロジェクトを追加
add_subdirectory(GXLib)
add_subdirectory(Sandbox)

# Sandboxをスタートアッププロジェクトに設定
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Sandbox)
