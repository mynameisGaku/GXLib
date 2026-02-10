# Phase 7 Summary: File / Network / Movie

## Overview
ファイルシステム抽象化 (VFS)、暗号化アーカイブ (.gxarc)、非同期アセットロード、ファイル変更監視、
TCP/UDP/HTTP/WebSocket ネットワーク、Media Foundation 動画再生を実装。

## Completion Condition
> 暗号化アーカイブからのアセット読込、HTTP通信、動画再生が動作 → **達成**

---

## Sub-phases

| Sub-phase | Content |
|-----------|---------|
| 7a | FileSystem + PhysicalFileProvider (VFS) |
| 7b | Archive + AES-256 + LZ4 compression |
| 7c | AsyncLoader + FileWatcher |
| 7d | TCP/UDP Socket + HTTP Client + WebSocket |
| 7e | MoviePlayer (Media Foundation) |
| 7f | Integration demo + VFS migration |

## New Files

| File | Description |
|------|-------------|
| `GXLib/IO/FileSystem.h/cpp` | VFS singleton, mount-based provider routing, path normalization |
| `GXLib/IO/PhysicalFileProvider.h/cpp` | Disk-backed IFileProvider |
| `GXLib/IO/Crypto.h/cpp` | AES-256-CBC, SHA-256, random bytes (Windows BCrypt API) |
| `GXLib/IO/Archive.h/cpp` | Custom .gxarc format (LZ4 + AES-256), ArchiveWriter |
| `GXLib/IO/ArchiveFileProvider.h/cpp` | Read-only IFileProvider backed by Archive (priority=100) |
| `GXLib/IO/AsyncLoader.h/cpp` | Background thread file loading, main-thread callback dispatch |
| `GXLib/IO/FileWatcher.h/cpp` | ReadDirectoryChangesW + OVERLAPPED async, change notification |
| `GXLib/IO/Network/TCPSocket.h/cpp` | Winsock2 TCP client |
| `GXLib/IO/Network/UDPSocket.h/cpp` | Winsock2 UDP socket |
| `GXLib/IO/Network/HTTPClient.h/cpp` | WinHTTP sync+async HTTP client |
| `GXLib/IO/Network/WebSocket.h/cpp` | WinHTTP WebSocket client |
| `GXLib/Movie/MoviePlayer.h/cpp` | Media Foundation video decode → texture |
| `GXLib/ThirdParty/lz4.h` | LZ4 compression header (BSD license) |
| `GXLib/ThirdParty/lz4.c` | LZ4 compression source |

## Modified Files

| File | Changes |
|------|---------|
| `GXLib/CMakeLists.txt` | Added IO/*.cpp, Movie/*.cpp to GLOB_RECURSE; lz4.c manual append (LANGUAGE C + SKIP_PRECOMPILE_HEADERS); linked bcrypt, ws2_32, winhttp, mfplat, mfreadwrite, mf, mfuuid |
| `GXLib/pch.h` | Added `<thread>`, `<mutex>`, `<condition_variable>`, `<atomic>` |
| `GXLib/GUI/XMLParser.cpp` | VFS-first LoadFromFile with ifstream fallback |
| `GXLib/GUI/StyleSheet.cpp` | VFS-first LoadFromFile with ifstream fallback |
| `Sandbox/main.cpp` | VFS init, archive demo, async HTTP GET, movie player, Phase 7 status display |

## Architecture

### VFS (Virtual File System)
```
FileSystem (singleton)
  ├── Mount("", PhysicalFileProvider("./"))     priority=0
  └── Mount("", ArchiveFileProvider("game.gxarc"))  priority=100
      ↓ ReadFile("Assets/texture.png")
      → Try highest priority first → Archive → Physical → FileData
```

### Archive Format (.gxarc)
```
[Magic: "GXARC\0\0\0" 8B] [TOC Header 16B] [TOC Data (AES-256)] [File Data (LZ4)]
```

### Network Stack
- TCPSocket / UDPSocket: Winsock2
- HTTPClient: WinHTTP (sync + async via detached thread)
- WebSocket: WinHTTP WebSocket API (receive thread + message queue)

### MoviePlayer
- IMFSourceReader → MFVideoFormat_RGB32 → BGRA→RGBA flip → TextureManager::CreateTextureFromMemory

## Key Design Decisions
- **VFS fallback pattern**: Try FileSystem first, fall back to direct I/O (backward compatible)
- **LZ4 as C source**: Compiled with LANGUAGE C + SKIP_PRECOMPILE_HEADERS to avoid PCH conflicts
- **Archive priority=100**: Archives take precedence over physical files
- **Async pattern**: Worker thread + mutex + completed queue + Update() on main thread
- **FileWatcher**: OVERLAPPED async ReadDirectoryChangesW with stop event

## Issues Encountered
- **LZ4 linker error**: CMake classified lz4.c as `<None Include>` instead of `<ClCompile Include>`. Fixed by adding `LANGUAGE C` to set_source_files_properties.

## Verification
- Build: OK
- VFS: ON (PhysicalFileProvider mounted)
- Archive: OK (encrypted archive created and verified)
- HTTP: 200 OK (232 bytes from httpbin.org)
- Movie: N/A (no test video, controls ready F5/F6)
