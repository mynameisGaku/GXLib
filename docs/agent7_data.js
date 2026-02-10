// Agent 7: IO + Network + Movie (12 classes)
// FileSystem, PhysicalFileProvider, Archive, ArchiveFileProvider, Crypto,
// AsyncLoader, FileWatcher, TCPSocket, UDPSocket, HTTPClient, WebSocket, MoviePlayer

{

// ============================================================
// FileSystem  (IO/FileSystem.h)  —  page-FileSystem
// ============================================================

// --- FileData struct ---
'FileSystem-data': [
  'std::vector<uint8_t> data',
  'FileDataが保持するファイルの生バイトデータ。IFileProvider::Read()やFileSystem::ReadFile()が返すFileData構造体のメンバーで、ファイル内容全体をバイト列として格納する。空の場合はファイルが見つからなかったか読み込みに失敗したことを意味する。',
  '// FileDataの生バイトにアクセス\nGX::FileData fd = GX::FileSystem::Instance().ReadFile("assets/config.bin");\nif (fd.IsValid()) {\n    const uint8_t* raw = fd.data.data();\n    size_t len = fd.data.size();\n}',
  '• FileData はムーブ可能な軽量構造体であり、読み込み結果のコンテナとして使用される\n• data が空の場合 IsValid() は false を返す\n• std::vector<uint8_t> なので RAII で自動的にメモリ解放される'
],

'FileSystem-IsValid': [
  'bool IsValid() const',
  'FileDataにデータが含まれているかどうかを判定する。内部的にはdata.empty()の否定を返す。ファイル読み込み結果の成否チェックに最初に使うべきメソッド。読み込みに失敗した場合やファイルが存在しない場合にfalseとなる。',
  '// ファイル読み込み結果の検証\nGX::FileData fd = GX::FileSystem::Instance().ReadFile("assets/level.dat");\nif (!fd.IsValid()) {\n    GX::Logger::Error("ファイル読み込み失敗");\n    return;\n}\n// 有効なデータを処理',
  '• 0バイトのファイルも IsValid() == false となる点に注意\n• Read 失敗時とファイル未存在時の区別はできない\n• 読み込み後は必ず IsValid() でチェックすること'
],

'FileSystem-Data': [
  'const uint8_t* Data() const',
  'FileDataの生バイトデータへのポインタを返す。バイナリファイルの解析やメモリコピーに使用する。data.data()のラッパーで、空の場合はnullptrを返す可能性がある。',
  '// バイナリデータの直接アクセス\nGX::FileData fd = GX::FileSystem::Instance().ReadFile("assets/texture.bin");\nif (fd.IsValid()) {\n    const uint8_t* ptr = fd.Data();\n    size_t size = fd.Size();\n    // ptr から size バイトを処理\n}',
  '• IsValid() が false の場合はアクセスしないこと\n• ポインタの寿命は FileData オブジェクトと同じ\n• const ポインタなので書き換え不可'
],

'FileSystem-Size': [
  'size_t Size() const',
  'FileDataのバイトサイズを返す。data.size()のラッパー。ファイル全体のサイズをバイト単位で取得でき、バッファ確保やデータ解析時のサイズ指定に利用する。',
  '// ファイルサイズの確認\nGX::FileData fd = GX::FileSystem::Instance().ReadFile("assets/model.glb");\nif (fd.IsValid()) {\n    GX::Logger::Info("File size: %zu bytes", fd.Size());\n}',
  '• IsValid() == false の場合は 0 を返す\n• 圧縮アーカイブからの読み込みでは展開後のサイズが返される\n• size_t 型なので 32bit環境では最大約4GBまで'
],

'FileSystem-AsString': [
  'std::string AsString() const',
  'FileDataの内容をUTF-8文字列として取得する。テキストファイル (JSON, XML, CSS, シェーダーソースなど) の読み込みに便利。バイトデータをそのままstd::stringに変換するため、バイナリファイルに使用すると文字化けする。',
  '// テキストファイルの読み込み\nGX::FileData fd = GX::FileSystem::Instance().ReadFile("assets/config.json");\nif (fd.IsValid()) {\n    std::string json = fd.AsString();\n    // JSON パースなどの処理\n}',
  '• BOM は自動除去されないため、必要に応じて手動で除去すること\n• バイナリファイルに使用すると意図しない結果になる\n• NUL文字を含むデータでは途中で文字列が切れる場合がある'
],

// --- IFileProvider interface ---
'FileSystem-IFileProviderExists': [
  'virtual bool Exists(const std::string& path) const = 0',
  'IFileProviderの純粋仮想メソッド。指定パスにファイルが存在するかを判定する。マウントポイント相対パスが渡される。PhysicalFileProviderではディスク上の実ファイルを、ArchiveFileProviderではアーカイブ内のエントリを検索する。',
  '// カスタムプロバイダーの実装例\nclass MyProvider : public GX::IFileProvider {\npublic:\n    bool Exists(const std::string& path) const override {\n        return m_files.count(path) > 0;\n    }\n    // ...\n};',
  '• IFileProvider を継承して独自プロバイダーを実装可能\n• パスはマウントポイントを除いた相対パス\n• スレッドセーフ性は実装クラスに依存する'
],

'FileSystem-IFileProviderRead': [
  'virtual FileData Read(const std::string& path) const = 0',
  'IFileProviderの純粋仮想メソッド。指定パスのファイルを読み込みFileDataとして返す。読み込み失敗時はIsValid()==falseのFileDataを返すべき。ArchiveFileProviderでは復号・LZ4解凍が自動的に行われる。',
  '// プロバイダー経由のファイル読み込み\nclass MyProvider : public GX::IFileProvider {\npublic:\n    GX::FileData Read(const std::string& path) const override {\n        GX::FileData fd;\n        // 独自ソースからデータ読み込み\n        return fd;\n    }\n};',
  '• 失敗時は空の FileData を返す (data が空)\n• 大きなファイルの場合メモリに全体を読み込むため注意\n• const メソッドなので内部状態を変更しないこと'
],

'FileSystem-IFileProviderWrite': [
  'virtual bool Write(const std::string& path, const void* data, size_t size) = 0',
  'IFileProviderの純粋仮想メソッド。指定パスにデータを書き込む。PhysicalFileProviderではディスクに書き込み、ArchiveFileProviderでは常にfalseを返す (読み取り専用)。書き込みをサポートしないプロバイダーはfalseを返せばよい。',
  '// プロバイダー経由の書き込み\nstd::string text = "Hello";\nGX::FileSystem::Instance().WriteFile(\n    "save/data.txt", text.data(), text.size());',
  '• ArchiveFileProvider は書き込み非対応 (常に false)\n• ディレクトリが存在しない場合の動作は実装依存\n• 書き込み成功で true、失敗で false を返す'
],

'FileSystem-Priority': [
  'virtual int Priority() const',
  'プロバイダーの優先度を返す。同一マウントポイントに複数プロバイダーが登録された場合、Priority()の値が高いものが先に検索される。デフォルト実装は0を返す。ArchiveFileProviderは100を返し、PhysicalFileProviderより優先される。',
  '// 高優先度のカスタムプロバイダー\nclass HighPriorityProvider : public GX::IFileProvider {\npublic:\n    int Priority() const override { return 200; }\n    // ...\n};',
  '• デフォルト値は 0\n• ArchiveFileProvider は 100 を返す\n• 同一優先度の場合はマウント順に検索される'
],

// --- FileSystem class ---
'FileSystem-Instance': [
  'static FileSystem& Instance()',
  'FileSystemのシングルトンインスタンスを取得する。アプリケーション全体で1つのVFSインスタンスを共有するため、どこからでもこのメソッドでアクセスできる。初回呼び出し時にインスタンスが生成される。',
  '// シングルトンへのアクセス\nauto& fs = GX::FileSystem::Instance();\nfs.Mount("assets", provider);\nauto data = fs.ReadFile("assets/config.json");',
  '• スレッドセーフな static local 初期化 (C++11保証)\n• アプリケーション終了時に自動破棄される\n• Mount/Unmount は起動時に一度行うのが一般的'
],

'FileSystem-Mount': [
  'void Mount(const std::string& mountPoint, std::shared_ptr<IFileProvider> provider)',
  'マウントポイントにファイルプロバイダーを登録する。登録後は "mountPoint/path" の形式でファイルアクセスが可能になる。同じマウントポイントに複数のプロバイダーを登録でき、Priority()順にファイルが検索される。内部的にPriority降順でソートされる。',
  '// 物理ディレクトリとアーカイブを同じマウントポイントに登録\nauto disk = std::make_shared<GX::PhysicalFileProvider>("Assets");\nauto arc = std::make_shared<GX::ArchiveFileProvider>();\narc->Open("Assets.gxarc", "password");\n\nauto& fs = GX::FileSystem::Instance();\nfs.Mount("assets", disk);\nfs.Mount("assets", arc); // Priority=100 で優先',
  '• マウントポイント名にスラッシュは不要 (例: "assets")\n• shared_ptr で所有権を共有するため、Unmountまでプロバイダーが生存する\n• 複数プロバイダーで同じパスにファイルがある場合は高優先度が返される'
],

'FileSystem-Unmount': [
  'void Unmount(const std::string& mountPoint)',
  '指定マウントポイントに登録された全プロバイダーを解除する。解除後はそのマウントポイント経由のファイルアクセスが不可能になる。アプリケーション終了時やシーン切り替え時に使用する。',
  '// マウントポイントの解除\nGX::FileSystem::Instance().Unmount("assets");\n// 以降 "assets/..." へのアクセスは失敗する',
  '• 登録されていないマウントポイントを指定しても安全 (何も起きない)\n• Unmount後もプロバイダーのshared_ptrを保持していれば再マウント可能\n• Clear()で全マウントポイントを一括解除できる'
],

'FileSystem-Exists': [
  'bool Exists(const std::string& path) const',
  '指定パスにファイルが存在するかを判定する。パスからマウントポイントを特定し、該当するプロバイダー群のExistsを優先度順に呼び出す。いずれかのプロバイダーでtrueが返ればtrueとなる。',
  '// ファイル存在チェック\nif (GX::FileSystem::Instance().Exists("assets/config.json")) {\n    auto data = GX::FileSystem::Instance().ReadFile("assets/config.json");\n    // ...\n}',
  '• パスは自動的に正規化される (バックスラッシュ→スラッシュ、先頭スラッシュ除去)\n• マウントされていないパスを指定すると常にfalse\n• ReadFile()の前にExistsで確認する使い方が一般的'
],

'FileSystem-ReadFile': [
  'FileData ReadFile(const std::string& path) const',
  'VFS経由でファイルを読み込む。パスからマウントポイントを特定し、優先度順にプロバイダーのRead()を呼び出す。最初にIsValid()==trueのデータを返したプロバイダーの結果を返す。全プロバイダーで失敗した場合はIsValid()==falseのFileDataを返す。',
  '// テキストファイルの読み込み\nauto data = GX::FileSystem::Instance().ReadFile("assets/ui/menu.css");\nif (data.IsValid()) {\n    std::string css = data.AsString();\n}\n// バイナリファイルの読み込み\nauto bin = GX::FileSystem::Instance().ReadFile("assets/model.glb");\nconst uint8_t* ptr = bin.Data();',
  '• ファイル全体をメモリに読み込むため、大きなファイルには AsyncLoader を検討\n• パスの正規化が自動的に行われる\n• アーカイブ内ファイルは自動的にLZ4解凍+AES復号される'
],

'FileSystem-WriteFile': [
  'bool WriteFile(const std::string& path, const void* data, size_t size)',
  'VFS経由でファイルを書き込む。パスからマウントポイントを特定し、対応するプロバイダーのWrite()を呼び出す。PhysicalFileProviderがマウントされていれば実際のディスクに書き込まれる。ArchiveFileProviderは書き込み非対応。',
  '// セーブデータの書き込み\nstd::string json = "{\\\"score\\\": 1000}";\nGX::FileSystem::Instance().WriteFile(\n    "save/progress.json",\n    json.data(), json.size());',
  '• ArchiveFileProvider は Write を常に false で返す\n• ディレクトリが存在しない場合は PhysicalFileProvider の実装に依存\n• 成功で true、失敗で false を返す'
],

'FileSystem-Clear': [
  'void Clear()',
  '全マウントポイントの登録を解除する。内部のマウントエントリリストを空にする。アプリケーション終了時やテスト環境でのリセットに使用する。プロバイダーのshared_ptrの参照カウントが減少し、他に参照がなければ破棄される。',
  '// 全マウントポイントの解除\nGX::FileSystem::Instance().Clear();\n// 新しいプロバイダーで再マウント\nauto newProvider = std::make_shared<GX::PhysicalFileProvider>("NewAssets");\nGX::FileSystem::Instance().Mount("assets", newProvider);',
  '• 実行後は全てのファイルアクセスが失敗する\n• shared_ptr なので他の箇所がプロバイダーを保持していれば破棄されない\n• シーン切り替え時のリソースリセットに便利'
],

// ============================================================
// PhysicalFileProvider  (IO/PhysicalFileProvider.h)  —  page-PhysicalFileProvider
// ============================================================

'PhysicalFileProvider-PhysicalFileProvider': [
  'explicit PhysicalFileProvider(const std::string& rootDir)',
  'ルートディレクトリを指定してPhysicalFileProviderを作成する。以後のファイルアクセスはこのルートディレクトリからの相対パスとして解決される。FileSystemにマウントして使用するのが一般的。',
  '// 物理ファイルプロバイダーの作成とマウント\nauto provider = std::make_shared<GX::PhysicalFileProvider>("Assets");\nGX::FileSystem::Instance().Mount("assets", provider);\n// "assets/textures/hero.png" → "Assets/textures/hero.png"',
  '• rootDir は実行ファイルからの相対パスまたは絶対パスを指定\n• ルートディレクトリが存在しなくてもコンストラクタは成功する (アクセス時にエラー)\n• Priority() はデフォルトの 0 を返す'
],

'PhysicalFileProvider-Exists': [
  'bool Exists(const std::string& path) const override',
  'ルートディレクトリ以下に指定パスのファイルが存在するかを判定する。内部的にrootDir + "/" + pathの結合パスで実ファイルの存在をチェックする。ディレクトリの場合はfalseを返す。',
  '// ファイル存在確認\nauto provider = std::make_shared<GX::PhysicalFileProvider>("Assets");\nbool found = provider->Exists("textures/hero.png");\n// → "Assets/textures/hero.png" の存在をチェック',
  '• パスの結合時にスラッシュが正規化される\n• ディレクトリに対しては false を返す\n• ファイルシステムの大文字小文字はOS依存 (Windowsでは区別しない)'
],

'PhysicalFileProvider-Read': [
  'FileData Read(const std::string& path) const override',
  'ルートディレクトリからの相対パスでファイルを読み込む。ファイル全体をバイト配列としてFileDataに格納して返す。ファイルが存在しないか読み込みに失敗した場合はIsValid()==falseのFileDataを返す。',
  '// ファイル読み込み\nauto provider = std::make_shared<GX::PhysicalFileProvider>("Assets");\nGX::FileData fd = provider->Read("config.json");\nif (fd.IsValid()) {\n    std::string text = fd.AsString();\n}',
  '• バイナリモードで読み込むため改行コードの変換は行わない\n• 大きなファイルはメモリ使用量に注意\n• 失敗原因 (権限、ロック等) はログに出力される'
],

'PhysicalFileProvider-Write': [
  'bool Write(const std::string& path, const void* data, size_t size) override',
  'ルートディレクトリからの相対パスにデータを書き込む。ファイルが存在する場合は上書きされる。セーブデータや設定ファイルの書き込みに使用する。書き込み成功でtrue、失敗でfalseを返す。',
  '// ファイル書き込み\nauto provider = std::make_shared<GX::PhysicalFileProvider>("SaveData");\nstd::string json = "{\\\"level\\\": 5}";\nbool ok = provider->Write("save.json", json.data(), json.size());',
  '• バイナリモードで書き込むため改行コードの変換は行わない\n• 中間ディレクトリが存在しない場合は失敗する可能性がある\n• アトミック書き込み (一時ファイル経由) ではないため、書き込み中にクラッシュするとデータ破損の可能性'
],

// ============================================================
// Archive / ArchiveWriter  (IO/Archive.h)  —  page-Archive
// ============================================================

// --- ArchiveEntry struct ---
'Archive-path': [
  'std::string path',
  'ArchiveEntry構造体のメンバー。アーカイブ内でのファイルパスを保持する。アーカイブ作成時にAddFile()で指定したarchivePathがそのまま格納される。GetEntries()で取得したエントリ一覧からパスを確認できる。',
  '// エントリ一覧の表示\nGX::Archive arc;\narc.Open("Assets.gxarc");\nfor (const auto& entry : arc.GetEntries()) {\n    printf("Path: %s, Size: %u\\n",\n           entry.path.c_str(), entry.originalSize);\n}',
  '• パスの区切りはスラッシュ (/) で正規化される\n• 絶対パスではなくアーカイブ内相対パス\n• 大文字小文字は保持されるが、検索時は実装に依存'
],

'Archive-offset': [
  'uint64_t offset',
  'ArchiveEntry構造体のメンバー。アーカイブファイルのデータ領域先頭からのバイトオフセットを格納する。内部的にRead()がこのオフセットを使ってシークし、データを読み込む。通常はユーザーが直接使用する必要はない。',
  '// エントリのオフセット情報 (デバッグ用)\nfor (const auto& entry : arc.GetEntries()) {\n    printf("  %s: offset=%llu, compressed=%u, original=%u\\n",\n           entry.path.c_str(), entry.offset,\n           entry.compressedSize, entry.originalSize);\n}',
  '• データ領域の先頭 (ヘッダー/TOC以降) からのオフセット\n• 直接ファイルシークに使用するものではない\n• アーカイブ形式の内部実装に依存'
],

'Archive-compressedSize': [
  'uint32_t compressedSize',
  'ArchiveEntry構造体のメンバー。LZ4圧縮後のデータサイズをバイト単位で格納する。非圧縮の場合はoriginalSizeと同じ値になる。圧縮率の計算やデバッグ情報の表示に使用できる。',
  '// 圧縮率の計算\nfor (const auto& e : arc.GetEntries()) {\n    float ratio = (float)e.compressedSize / e.originalSize * 100.0f;\n    printf("%s: %.1f%% compressed\\n", e.path.c_str(), ratio);\n}',
  '• 暗号化されている場合はパディングを含むサイズになる場合がある\n• LZ4非圧縮時は originalSize と同値\n• uint32_t のため最大約4GBまでの個別ファイルに対応'
],

'Archive-originalSize': [
  'uint32_t originalSize',
  'ArchiveEntry構造体のメンバー。圧縮・暗号化前の元のファイルサイズをバイト単位で格納する。Read()で取得するFileDataのSizeはこの値と一致する。アーカイブ展開後のバッファサイズ確保にも使われる。',
  '// 元のファイルサイズ確認\nconst auto& entries = arc.GetEntries();\nfor (const auto& e : entries) {\n    printf("%s: %u bytes (original)\\n",\n           e.path.c_str(), e.originalSize);\n}',
  '• Read() の結果の Size() と一致する\n• uint32_t のため最大約4GBまでの個別ファイルに対応\n• TOC (Table of Contents) に記録されるメタデータ'
],

'Archive-flags': [
  'uint8_t flags',
  'ArchiveEntry構造体のメンバー。エントリのフラグビットを格納する。bit0が1の場合はLZ4圧縮が適用されていることを示す。将来の拡張用にbit1-7は予約されている。',
  '// 圧縮フラグの確認\nfor (const auto& e : arc.GetEntries()) {\n    bool compressed = (e.flags & 0x01) != 0;\n    printf("%s: %s\\n", e.path.c_str(),\n           compressed ? "compressed" : "raw");\n}',
  '• bit0: LZ4圧縮フラグ\n• bit1-7: 予約 (将来の拡張用)\n• 暗号化フラグはTOCヘッダーレベルで管理される'
],

// --- Archive class ---
'Archive-Open': [
  'bool Open(const std::string& filePath, const std::string& password = "")',
  '.gxarcアーカイブファイルを開く。マジックナンバーの検証、TOC (目次) の読み込み、暗号化されている場合はパスワードからSHA-256で鍵を導出して復号する。成功するとContains()やRead()でアーカイブ内ファイルにアクセス可能になる。',
  '// アーカイブを開く\nGX::Archive arc;\nif (arc.Open("Assets.gxarc", "mySecretKey")) {\n    auto data = arc.Read("textures/hero.png");\n    // ...\n}\narc.Close();',
  '• マジックナンバー "GXARC\\0\\0\\0" を検証する\n• パスワードが不正な場合は false を返す\n• 暗号化なしの場合は password を空文字にする'
],

'Archive-Close': [
  'void Close()',
  'アーカイブを閉じてリソースを解放する。エントリ情報や復号鍵をクリアする。Close後にRead()やContains()を呼んでも結果は返されない。再利用するにはOpen()を再度呼ぶ必要がある。',
  '// アーカイブのクローズ\nGX::Archive arc;\narc.Open("Assets.gxarc");\nauto data = arc.Read("config.json");\narc.Close(); // リソース解放',
  '• デストラクタでも自動的に Close される\n• Close 後の Read/Contains は無効な結果を返す\n• 複数回 Close を呼んでも安全'
],

'Archive-Contains': [
  'bool Contains(const std::string& path) const',
  'アーカイブ内に指定パスのファイルが存在するかを判定する。TOCのエントリ一覧を検索して該当パスがあればtrueを返す。Read()の前にファイルの存在を確認したい場合に使用する。',
  '// アーカイブ内ファイルの存在確認\nGX::Archive arc;\narc.Open("Assets.gxarc");\nif (arc.Contains("shaders/PBR.hlsl")) {\n    auto data = arc.Read("shaders/PBR.hlsl");\n}',
  '• Open() が成功していない場合は常に false\n• パスの大文字小文字は完全一致で比較される\n• 線形検索のためエントリ数が非常に多い場合は注意'
],

'Archive-Read': [
  'FileData Read(const std::string& path) const',
  'アーカイブからファイルデータを読み込む。内部的にエントリのオフセットからデータを読み取り、暗号化されていればAES-256-CBCで復号、圧縮されていればLZ4で解凍して返す。失敗時はIsValid()==falseのFileDataを返す。',
  '// アーカイブからファイル読み込み\nGX::Archive arc;\narc.Open("Assets.gxarc", "password123");\nGX::FileData data = arc.Read("models/character.glb");\nif (data.IsValid()) {\n    // data.Data(), data.Size() でアクセス\n}',
  '• 復号→解凍の順で処理される\n• originalSize 分のメモリを確保するためメモリ使用量に注意\n• パスが見つからない場合は IsValid()==false の FileData を返す'
],

'Archive-GetEntries': [
  'const std::vector<ArchiveEntry>& GetEntries() const',
  'アーカイブに格納された全エントリの一覧を取得する。各エントリにはパス、オフセット、圧縮サイズ、元サイズ、フラグが含まれる。デバッグ情報の表示やファイルリストの列挙に使用する。',
  '// 全エントリの列挙\nGX::Archive arc;\narc.Open("Assets.gxarc");\nfor (const auto& e : arc.GetEntries()) {\n    printf("%s (%u bytes)\\n", e.path.c_str(), e.originalSize);\n}',
  '• const 参照を返すため、コピーなしでアクセス可能\n• Open() 前は空の vector が返される\n• エントリ順はアーカイブ作成時の AddFile() の順序'
],

// --- ArchiveWriter class ---
'Archive-SetPassword': [
  'void SetPassword(const std::string& password)',
  'アーカイブの暗号化パスワードを設定する。空文字を設定すると暗号化無効になる。パスワードはSave()時にSHA-256でハッシュ化され、AES-256-CBCの暗号鍵として使用される。Save()を呼ぶ前に設定すること。',
  '// 暗号化アーカイブの作成\nGX::ArchiveWriter writer;\nwriter.SetPassword("secretKey123");\nwriter.AddFile("config.json", "Assets/config.json");\nwriter.Save("Encrypted.gxarc");',
  '• 空文字で暗号化無効\n• パスワードは SHA-256 でハッシュ化されて鍵として使用\n• Save() 前であればいつでも変更可能'
],

'Archive-SetCompression': [
  'void SetCompression(bool enable)',
  'LZ4圧縮の有効/無効を設定する。デフォルトはtrue (圧縮有効)。テクスチャや動画など既に圧縮済みのファイルが多い場合はfalseにすると、パック速度が向上し圧縮オーバーヘッドを回避できる。',
  '// 圧縮無効でアーカイブ作成\nGX::ArchiveWriter writer;\nwriter.SetCompression(false); // 圧縮しない\nwriter.AddFile("video.mp4", "Assets/video.mp4");\nwriter.Save("NoCompress.gxarc");',
  '• デフォルトは true (LZ4圧縮有効)\n• 既に圧縮済みのデータ (PNG, MP4等) には効果が薄い\n• 全ファイルに一括適用される (ファイル個別の設定は不可)'
],

'Archive-AddFile': [
  'void AddFile(const std::string& archivePath, const std::string& diskPath)',
  'ディスク上のファイルをアーカイブに追加する。archivePathはアーカイブ内での格納パス、diskPathはディスク上の実際のファイルパス。Save()を呼ぶまで内部バッファに保持される。複数ファイルを連続して追加可能。',
  '// ディスクファイルの追加\nGX::ArchiveWriter writer;\nwriter.AddFile("textures/hero.png", "Assets/textures/hero.png");\nwriter.AddFile("shaders/PBR.hlsl", "Shaders/PBR.hlsl");\nwriter.AddFile("config.json", "Assets/config.json");\nwriter.Save("Assets.gxarc");',
  '• diskPath が存在しない場合は Save() 時にスキップされる\n• archivePath はアーカイブ内パスであり、ディスクパスと異なっていてよい\n• 同じ archivePath を複数回追加すると後のものが優先される'
],

'Archive-AddFile2': [
  'void AddFile(const std::string& archivePath, const void* data, size_t size)',
  'メモリ上のデータをアーカイブに追加する。動的に生成したデータやファイル以外のソースからのバイト列をアーカイブに格納する場合に使用する。ディスクファイル版のAddFileと同じアーカイブ内パスに格納される。',
  '// メモリデータの追加\nGX::ArchiveWriter writer;\nstd::string json = "{\\\"version\\\": 1}";\nwriter.AddFile("meta.json", json.data(), json.size());\nwriter.Save("Data.gxarc");',
  '• データはコピーされるため、呼び出し後に元データを破棄しても安全\n• size が 0 の場合でもエントリは作成される\n• ディスクファイル版と混在して使用可能'
],

'Archive-Save': [
  'bool Save(const std::string& outputPath)',
  '追加された全ファイルをパックしてアーカイブファイルを出力する。マジックナンバー→TOCヘッダー→TOCエントリ→ファイルデータの順で書き込む。暗号化・圧縮設定に従いデータを処理する。成功でtrue、失敗でfalseを返す。',
  '// アーカイブの保存\nGX::ArchiveWriter writer;\nwriter.SetPassword("key");\nwriter.SetCompression(true);\nwriter.AddFile("data.bin", rawData, rawSize);\nif (!writer.Save("output.gxarc")) {\n    GX::Logger::Error("アーカイブ保存失敗");\n}',
  '• ファイル形式: Magic(8B) + TOCHeader + TOCEntries + FileData\n• 出力先ディレクトリが存在しない場合は失敗する\n• 大量のファイルを含む場合はメモリ使用量に注意 (全データを一度に保持)'
],

// ============================================================
// ArchiveFileProvider  (IO/ArchiveFileProvider.h)  —  page-ArchiveFileProvider
// ============================================================

'ArchiveFileProvider-Open': [
  'bool Open(const std::string& archivePath, const std::string& password = "")',
  '.gxarcアーカイブファイルを開いてプロバイダーとして使用可能にする。内部のArchiveオブジェクトに委譲してアーカイブを開く。成功後にFileSystemにマウントすると、VFS経由でアーカイブ内ファイルにアクセスできるようになる。',
  '// アーカイブプロバイダーのセットアップ\nauto arcProvider = std::make_shared<GX::ArchiveFileProvider>();\nif (arcProvider->Open("Assets.gxarc", "password")) {\n    GX::FileSystem::Instance().Mount("assets", arcProvider);\n}',
  '• 内部で Archive::Open() を呼び出す\n• Open 失敗時は false を返し、マウントしても動作しない\n• 暗号化なしの場合は password を省略可能'
],

'ArchiveFileProvider-Priority': [
  'int Priority() const override',
  'プロバイダーの優先度を返す。ArchiveFileProviderは固定値100を返し、PhysicalFileProvider (デフォルト0) より優先される。同じマウントポイントに両方を登録した場合、アーカイブ内のファイルが優先的に使用される。',
  '// 優先度の確認\nauto arcProvider = std::make_shared<GX::ArchiveFileProvider>();\narcProvider->Open("Assets.gxarc");\nint pri = arcProvider->Priority(); // 100\n\nauto diskProvider = std::make_shared<GX::PhysicalFileProvider>("Assets");\nint diskPri = diskProvider->Priority(); // 0',
  '• 固定値 100 を返す\n• PhysicalFileProvider (0) より優先されるため、開発時はアーカイブをアンマウントすると便利\n• カスタムプロバイダーで 100 より高い値を設定すれば更に優先できる'
],

// ============================================================
// Crypto  (IO/Crypto.h)  —  page-Crypto
// ============================================================

'Crypto-Encrypt': [
  'static std::vector<uint8_t> Encrypt(const void* data, size_t size, const uint8_t key[32], const uint8_t iv[16])',
  'AES-256-CBCでデータを暗号化する。256ビット (32バイト) の鍵と128ビット (16バイト) の初期化ベクトルを使用する。PKCS#7パディングが自動的に適用される。失敗時は空のvectorを返す。Archiveの内部で使用されるが、直接呼び出すことも可能。',
  '// データの暗号化\nuint8_t key[32], iv[16];\nGX::Crypto::GenerateRandomBytes(key, 32);\nGX::Crypto::GenerateRandomBytes(iv, 16);\n\nstd::string text = "Secret data";\nauto encrypted = GX::Crypto::Encrypt(\n    text.data(), text.size(), key, iv);',
  '• Windows BCrypt API (bcrypt.dll) を内部で使用\n• PKCS#7 パディングにより出力サイズは入力より大きくなる\n• iv は暗号化と復号で同じ値を使う必要がある\n• 全 static メソッドなのでインスタンス不要'
],

'Crypto-Decrypt': [
  'static std::vector<uint8_t> Decrypt(const void* data, size_t size, const uint8_t key[32], const uint8_t iv[16])',
  'AES-256-CBCで暗号化されたデータを復号する。Encrypt()と同じ鍵とIVを使用して元のデータを復元する。パディングの除去も自動的に行われる。鍵やIVが不正な場合は空のvectorを返す。',
  '// データの復号\nauto decrypted = GX::Crypto::Decrypt(\n    encrypted.data(), encrypted.size(), key, iv);\nstd::string result(decrypted.begin(), decrypted.end());\n// result == "Secret data"',
  '• Encrypt と同じ key/iv のペアが必要\n• 不正な鍵で復号するとゴミデータまたは空が返る\n• PKCS#7 パディングの検証も行われる\n• 入力サイズが 16 の倍数でない場合は失敗する'
],

'Crypto-SHA256': [
  'static std::array<uint8_t, 32> SHA256(const void* data, size_t size)',
  'SHA-256ハッシュを計算する。任意のデータから固定長32バイトのハッシュ値を生成する。パスワードから暗号鍵への変換、ファイルの整合性チェック、データのフィンガープリント生成などに使用する。Archiveではパスワードからの鍵導出に使われる。',
  '// ハッシュの計算\nstd::string password = "myPassword";\nauto hash = GX::Crypto::SHA256(\n    password.data(), password.size());\n// hash は 32 バイトの std::array',
  '• 出力は固定 32 バイト (256 ビット)\n• 同じ入力に対して常に同じハッシュを返す (決定的)\n• Windows BCrypt API を内部使用\n• Archive ではパスワード → SHA-256 → AES鍵として使用'
],

'Crypto-GenerateRandomBytes': [
  'static void GenerateRandomBytes(uint8_t* buffer, size_t size)',
  '暗号学的に安全な乱数バイト列を生成する。Windows BCryptGenRandom APIを使用しており、暗号鍵やIVの生成に適している。疑似乱数 (rand()) とは異なり、予測不可能な乱数を提供する。',
  '// 安全な乱数の生成\nuint8_t key[32];\nGX::Crypto::GenerateRandomBytes(key, 32); // 256-bit鍵\n\nuint8_t iv[16];\nGX::Crypto::GenerateRandomBytes(iv, 16); // 128-bit IV',
  '• BCryptGenRandom を内部使用 (CSPRNG)\n• ゲームの乱数には GX::Random を使用する方が適切 (性能面)\n• セキュリティ用途 (鍵生成、トークン等) に限定して使用\n• buffer は呼び出し側で確保すること'
],

// ============================================================
// AsyncLoader  (IO/AsyncLoader.h)  —  page-AsyncLoader
// ============================================================

// --- LoadStatus enum ---
'AsyncLoader-Pending': [
  'LoadStatus::Pending',
  'リクエストがキューに入っているが、まだワーカースレッドによる処理が開始されていない状態。Load()を呼んだ直後はこの状態になる。先行するリクエストの処理完了を待機中。',
  '// ステータス確認\nGX::AsyncLoader loader;\nuint32_t id = loader.Load("assets/large.bin",\n    [](GX::FileData& fd) { /* ... */ });\n// 直後は Pending\nassert(loader.GetStatus(id) == GX::LoadStatus::Pending);',
  '• Load() 直後の初期状態\n• ワーカースレッドが空いた時点で Loading に遷移する\n• CancelAll() で Pending 状態のリクエストもキャンセルされる'
],

'AsyncLoader-Loading': [
  'LoadStatus::Loading',
  'ワーカースレッドがファイルの読み込みを実行中の状態。VFS経由でファイルを読み込んでいる最中であり、大きなファイルの場合はこの状態が長く続く可能性がある。',
  '// 読み込み中の表示\nif (loader.GetStatus(id) == GX::LoadStatus::Loading) {\n    // ローディングインジケータを表示\n}',
  '• ワーカースレッドで IO が実行中\n• メインスレッドはブロックされない\n• 完了すると Complete または Error に遷移する'
],

'AsyncLoader-Complete': [
  'LoadStatus::Complete',
  'ファイルの読み込みが正常に完了した状態。次のUpdate()呼び出し時にメインスレッドでコールバックが発火される。コールバック発火後もステータスはCompleteのまま保持される。',
  '// 完了待ち\nif (loader.GetStatus(id) == GX::LoadStatus::Complete) {\n    // コールバックがまだ発火していなければ次のUpdate()で発火\n}',
  '• Update() でコールバックが発火される\n• ステータスは完了後も照会可能\n• FileData の IsValid() も true となる'
],

'AsyncLoader-Error': [
  'LoadStatus::Error',
  'ファイルの読み込みに失敗した状態。ファイルが存在しない、パスが不正、アクセス権限エラーなどの場合に遷移する。コールバックはIsValid()==falseのFileDataで発火される。',
  '// エラー処理\nloader.Load("assets/missing.bin", [](GX::FileData& fd) {\n    if (!fd.IsValid()) {\n        GX::Logger::Error("ファイル読み込みエラー");\n    }\n});',
  '• コールバックは発火されるが FileData は無効 (IsValid()==false)\n• エラーの詳細はログに出力される\n• リトライは新たに Load() を呼ぶ必要がある'
],

// --- AsyncLoader class ---
'AsyncLoader-AsyncLoader': [
  'AsyncLoader()',
  'AsyncLoaderを作成し、ワーカースレッドを起動する。コンストラクタの時点でバックグラウンドスレッドが開始され、Load()でキューに追加されたリクエストを処理する準備が整う。',
  '// AsyncLoaderの作成\nGX::AsyncLoader loader;\n// ワーカースレッドが起動済み\nuint32_t id = loader.Load("assets/data.bin",\n    [](GX::FileData& fd) { /* ... */ });',
  '• コンストラクタでワーカースレッドが自動起動\n• 1つのワーカースレッドで順次処理する\n• 複数の AsyncLoader インスタンスを作成すると複数スレッドになる'
],

'AsyncLoader-Load': [
  'uint32_t Load(const std::string& path, std::function<void(FileData&)> onComplete)',
  '非同期ファイル読み込みリクエストをキューに追加する。pathはVFS経由で解決される。読み込み完了後、次のUpdate()呼び出し時にメインスレッドでonCompleteコールバックが発火される。戻り値のリクエストIDでGetStatus()による状態確認が可能。',
  '// 非同期ファイル読み込み\nGX::AsyncLoader loader;\nuint32_t texId = loader.Load("assets/textures/hero.png",\n    [&texMgr, &device](GX::FileData& fd) {\n        if (fd.IsValid()) {\n            texMgr.CreateTextureFromMemory(\n                device, fd.Data(), fd.Size());\n        }\n    });\n// 毎フレーム Update() を呼ぶこと',
  '• コールバックはメインスレッドの Update() 内で呼ばれるためスレッドセーフ\n• VFS (FileSystem) 経由でファイルを読み込む\n• リクエストIDは 1 から連番で割り当てられる\n• 大量の同時リクエストはメモリ使用量に注意'
],

'AsyncLoader-Update': [
  'void Update()',
  '完了した非同期リクエストのコールバックをメインスレッドで発火する。毎フレームのゲームループ内で呼び出す必要がある。複数のリクエストが同時に完了していた場合、1回のUpdate()で全て発火される。',
  '// メインループでの呼び出し\nwhile (running) {\n    // ... ゲームロジック ...\n    loader.Update(); // コールバック発火\n    // ... 描画 ...\n}',
  '• メインスレッドで呼ぶこと (コールバックがメインスレッドで実行される)\n• 呼び忘れるとコールバックが永久に発火されない\n• 1フレーム内で複数回呼んでも問題ない'
],

'AsyncLoader-CancelAll': [
  'void CancelAll()',
  '保留中の全リクエストをキャンセルする。キューに残っているPending状態のリクエストが破棄され、コールバックは発火されない。既にLoading中のリクエストは完了するが、コールバックは発火されない。シーン切り替え時のクリーンアップに使用する。',
  '// シーン切り替え時のキャンセル\nloader.CancelAll();\n// 新しいシーンのアセットをロード\nloader.Load("assets/newScene/bg.png", callback);',
  '• Loading 中のリクエストの IO は中断されない (完了後に破棄)\n• CancelAll 後も新たに Load() は可能\n• デストラクタでも暗黙的に CancelAll が呼ばれる'
],

'AsyncLoader-GetStatus': [
  'LoadStatus GetStatus(uint32_t requestId) const',
  '指定リクエストIDの現在の読み込み状態を取得する。Load()が返したIDを渡してPending/Loading/Complete/Errorのいずれかを取得する。ローディング画面の進捗表示やデバッグに使用する。',
  '// ステータスベースの進捗表示\nuint32_t id = loader.Load("assets/large.bin", callback);\n// ... 後のフレームで\nswitch (loader.GetStatus(id)) {\n    case GX::LoadStatus::Pending:  /* キュー中 */ break;\n    case GX::LoadStatus::Loading:  /* 読込中 */ break;\n    case GX::LoadStatus::Complete: /* 完了 */ break;\n    case GX::LoadStatus::Error:    /* エラー */ break;\n}',
  '• スレッドセーフ (内部でmutexロック)\n• 存在しないIDを指定した場合の動作は未定義\n• CancelAll 後も以前のIDのステータスは照会可能'
],

// ============================================================
// FileWatcher  (IO/FileWatcher.h)  —  page-FileWatcher
// ============================================================

'FileWatcher-Watch': [
  'void Watch(const std::string& directory, std::function<void(const std::string& path)> onChange)',
  '指定ディレクトリの変更監視を開始する。ReadDirectoryChangesW + OVERLAPPED非同期方式でファイルの作成・変更・削除を検出する。変更があるとonChangeコールバックに変更されたファイルのパスが渡される。コールバックはUpdate()内でメインスレッドから発火される。',
  '// シェーダーディレクトリの監視\nGX::FileWatcher watcher;\nwatcher.Watch("Shaders", [](const std::string& path) {\n    GX::Logger::Info("File changed: %s", path.c_str());\n    // シェーダーホットリロード処理\n});',
  '• サブディレクトリも再帰的に監視される\n• コールバックは Update() 内で発火される (メインスレッドセーフ)\n• 複数ディレクトリを同時に Watch() 可能\n• Windows の ReadDirectoryChangesW API を内部使用'
],

'FileWatcher-Update': [
  'void Update()',
  '保留中のファイル変更通知コールバックを発火する。バックグラウンドスレッドが検出した変更をメインスレッドで処理するため、毎フレーム呼び出す必要がある。ShaderHotReloadシステムで内部的に使用されている。',
  '// メインループでの呼び出し\nGX::FileWatcher watcher;\nwatcher.Watch("Shaders", onChange);\n\nwhile (running) {\n    watcher.Update(); // 変更通知の処理\n    // ... ゲームループ ...\n}',
  '• メインスレッドで呼ぶこと\n• 呼び忘れるとコールバックが発火されない\n• 1フレーム内に複数の変更があった場合、まとめて通知される'
],

'FileWatcher-Stop': [
  'void Stop()',
  '全ての監視を停止する。バックグラウンドスレッドを終了し、監視ハンドルを閉じる。保留中のコールバックも破棄される。再度Watch()を呼べば監視を再開できる。',
  '// 監視の停止\nwatcher.Stop();\n// 必要に応じて再開\nwatcher.Watch("Shaders", newCallback);',
  '• デストラクタでも自動的に Stop される\n• Stop 後は保留中のコールバックも失われる\n• WaitForMultipleObjects で監視スレッドの終了を待つ'
],

// ============================================================
// TCPSocket  (IO/Network/TCPSocket.h)  —  page-TCPSocket
// ============================================================

'TCPSocket-Connect': [
  'bool Connect(const std::string& host, uint16_t port)',
  'TCPサーバーに接続する。ホスト名解決 (DNS) とTCPハンドシェイクを行う。接続成功でtrue、タイムアウトや接続拒否の場合はfalseを返す。ブロッキング呼び出しのため、接続完了まで待機する。Winsock2の初期化は自動的に行われる。',
  '// TCPサーバーへの接続\nGX::TCPSocket socket;\nif (socket.Connect("127.0.0.1", 8080)) {\n    const char* msg = "Hello";\n    socket.Send(msg, (int)strlen(msg));\n    char buf[1024];\n    int received = socket.Receive(buf, sizeof(buf));\n}\nsocket.Close();',
  '• WSAStartup は初回呼び出し時に自動実行される\n• ブロッキング接続のためメインスレッドでの使用は注意\n• IPv4 アドレスまたはホスト名を指定可能\n• 接続失敗時はログにエラー内容が出力される'
],

'TCPSocket-Close': [
  'void Close()',
  'TCP接続を閉じてソケットを解放する。接続中でない場合も安全に呼び出せる。Close後にSend/Receiveを呼ぶとエラーになる。再接続するにはConnect()を再度呼ぶ。',
  '// 接続のクローズ\nGX::TCPSocket socket;\nsocket.Connect("127.0.0.1", 8080);\n// ... 通信 ...\nsocket.Close(); // 切断',
  '• デストラクタでも自動的に Close される\n• graceful shutdown (FIN送信) を行う\n• 複数回呼び出しても安全'
],

'TCPSocket-IsConnected': [
  'bool IsConnected() const',
  'ソケットが接続中かどうかを判定する。Connect()が成功した後でClose()されていない場合にtrueを返す。ただし相手側が切断した場合の検出はReceive()の戻り値(0)で確認する必要がある。',
  '// 接続状態の確認\nGX::TCPSocket socket;\nif (!socket.IsConnected()) {\n    socket.Connect("server.example.com", 443);\n}\nif (socket.IsConnected()) {\n    // 通信可能\n}',
  '• ソケットハンドルの有効性を確認するだけで、実際の接続状態は保証しない\n• 相手側切断は Receive() が 0 を返すことで検出\n• Connect 前は常に false'
],

'TCPSocket-Send': [
  'int Send(const void* data, int size)',
  'データを送信する。送信バッファに書き込み、実際に送信されたバイト数を返す。エラー時は-1を返す。大きなデータの場合、1回の呼び出しで全て送信されない可能性がある(部分送信)。',
  '// データ送信\nGX::TCPSocket socket;\nsocket.Connect("127.0.0.1", 8080);\nstd::string msg = "Hello, Server!";\nint sent = socket.Send(msg.data(), (int)msg.size());\nif (sent < 0) {\n    GX::Logger::Error("送信エラー");\n}',
  '• 戻り値が size より小さい場合は部分送信 (残りを再送する必要がある)\n• エラー時は -1 を返す\n• ノンブロッキングモードでは WSAEWOULDBLOCK の場合も -1\n• 接続が切れている場合もエラーとなる'
],

'TCPSocket-Receive': [
  'int Receive(void* buffer, int bufferSize)',
  '受信バッファからデータを読み込む。実際に受信されたバイト数を返す。エラー時は-1、相手側が接続を閉じた場合は0を返す。ブロッキングモードでは受信データが届くまで待機する。',
  '// データ受信\nchar buffer[4096];\nint received = socket.Receive(buffer, sizeof(buffer));\nif (received > 0) {\n    // buffer に received バイトのデータ\n} else if (received == 0) {\n    // 切断された\n} else {\n    // エラー\n}',
  '• 戻り値 0 は相手側の graceful shutdown を意味する\n• ブロッキングモードではデータ到着まで待機する\n• 1回で bufferSize 分すべて受信される保証はない\n• ノンブロッキング時はデータなしで -1 を返す場合がある'
],

'TCPSocket-SetNonBlocking': [
  'void SetNonBlocking(bool nonBlocking)',
  'ソケットのノンブロッキングモードを設定する。trueに設定するとSend/Receiveが即座に返り、データが準備できていない場合はエラー(-1)を返す。ゲームループ内でブロックせず通信する場合に必須の設定。',
  '// ノンブロッキング通信\nGX::TCPSocket socket;\nsocket.Connect("127.0.0.1", 8080);\nsocket.SetNonBlocking(true);\n// Receive はデータがなければ即座に -1 を返す\nchar buf[1024];\nint n = socket.Receive(buf, sizeof(buf));',
  '• ioctlsocket(FIONBIO) を内部で使用\n• ゲームループ内では true を推奨\n• HasData() と組み合わせてポーリング可能\n• Connect 後に設定すること'
],

'TCPSocket-HasData': [
  'bool HasData() const',
  '読み取り可能な受信データがあるかをノンブロッキングで判定する。select()を使用してソケットの読み取り準備状態をチェックする。Receive()を呼ぶ前の事前チェックとして使用し、不要なブロックを回避する。',
  '// ポーリング受信\nGX::TCPSocket socket;\nsocket.Connect("127.0.0.1", 8080);\nsocket.SetNonBlocking(true);\n\n// ゲームループ内\nif (socket.HasData()) {\n    char buf[1024];\n    int n = socket.Receive(buf, sizeof(buf));\n}',
  '• select() でタイムアウト0のチェックを行う\n• 接続が切れた場合もtrueを返す (Receiveで0が返る)\n• ノンブロッキングモードと組み合わせて使用\n• メインループのポーリングパターンに最適'
],

// ============================================================
// UDPSocket  (IO/Network/UDPSocket.h)  —  page-UDPSocket
// ============================================================

'UDPSocket-Bind': [
  'bool Bind(uint16_t port)',
  'UDPソケットをローカルポートにバインドする。バインド後にReceiveFrom()で指定ポートへのデータを受信できる。送信のみの場合はBind不要 (SendToだけで使用可能)。成功でtrue、ポート使用中等の場合はfalseを返す。',
  '// UDPサーバーとしてバインド\nGX::UDPSocket socket;\nif (socket.Bind(9999)) {\n    char buf[1024];\n    std::string host;\n    uint16_t port;\n    int n = socket.ReceiveFrom(buf, sizeof(buf), host, port);\n}',
  '• 1つのポートに複数ソケットはバインドできない\n• 送信のみならバインド不要\n• WSAStartup は TCPSocket と共有で自動初期化\n• 0 を指定すると OS が空きポートを割り当てる'
],

'UDPSocket-Close': [
  'void Close()',
  'UDPソケットを閉じてリソースを解放する。バインドが解除され、ポートが再利用可能になる。Close後のSendTo/ReceiveFromはエラーとなる。',
  '// ソケットのクローズ\nGX::UDPSocket socket;\nsocket.Bind(9999);\n// ... 通信 ...\nsocket.Close();',
  '• デストラクタでも自動的に Close される\n• 複数回呼び出しても安全\n• Close 後はバインドが解除される'
],

'UDPSocket-SendTo': [
  'int SendTo(const std::string& host, uint16_t port, const void* data, int size)',
  '指定ホスト・ポートにUDPデータグラムを送信する。コネクションレスのため事前接続不要。実際に送信されたバイト数を返し、エラー時は-1を返す。UDPは到着順序やデリバリーを保証しない。',
  '// UDPメッセージ送信\nGX::UDPSocket socket;\nstd::string msg = "ping";\nint sent = socket.SendTo(\n    "192.168.1.100", 9999,\n    msg.data(), (int)msg.size());',
  '• Bind なしでも送信可能 (OS がエフェメラルポートを割り当て)\n• 最大データグラムサイズは約65507バイト (IPv4)\n• UDPは配送保証なし — アプリケーション層で対処が必要\n• ブロードキャストアドレスへの送信も可能'
],

'UDPSocket-ReceiveFrom': [
  'int ReceiveFrom(void* buffer, int bufferSize, std::string& outHost, uint16_t& outPort)',
  'UDPデータグラムを受信し、送信元のホスト・ポート情報も取得する。事前にBind()でポートをバインドしておく必要がある。実際に受信されたバイト数を返し、エラー時は-1を返す。ブロッキングモードでは受信まで待機する。',
  '// UDP受信 (送信元情報あり)\nGX::UDPSocket socket;\nsocket.Bind(9999);\nchar buf[1024];\nstd::string senderHost;\nuint16_t senderPort;\nint n = socket.ReceiveFrom(\n    buf, sizeof(buf), senderHost, senderPort);\nif (n > 0) {\n    // senderHost, senderPort に送信元情報\n}',
  '• Bind() が必要 (バインドなしだと受信できない)\n• outHost/outPort に送信元情報が格納される\n• 1データグラム分のみ受信 (TCP のようなストリームではない)\n• bufferSize より大きなデータグラムは切り捨てられる'
],

'UDPSocket-SetNonBlocking': [
  'void SetNonBlocking(bool nonBlocking)',
  'UDPソケットのノンブロッキングモードを設定する。trueに設定するとReceiveFrom()が即座に返り、データがない場合は-1を返す。ゲームループ内でブロックせずUDP通信を行う場合に使用する。',
  '// ノンブロッキングUDP\nGX::UDPSocket socket;\nsocket.Bind(9999);\nsocket.SetNonBlocking(true);\n// ゲームループ内でポーリング\nchar buf[1024];\nstd::string host; uint16_t port;\nint n = socket.ReceiveFrom(buf, sizeof(buf), host, port);',
  '• ioctlsocket(FIONBIO) を内部で使用\n• リアルタイムゲームでは true を推奨\n• SendTo はバッファが満杯でない限り即座に完了する\n• Bind 後に設定すること'
],

// ============================================================
// HTTPClient  (IO/Network/HTTPClient.h)  —  page-HTTPClient
// ============================================================

// --- HTTPResponse struct ---
'HTTPClient-statusCode': [
  'int statusCode',
  'HTTPResponseのステータスコード。200 (OK)、404 (Not Found)、500 (Internal Server Error) など標準HTTPステータスコードが格納される。通信失敗 (接続不可等) の場合は0になる。IsSuccess()で200番台かどうかを簡易判定できる。',
  '// ステータスコードの確認\nGX::HTTPClient http;\nauto res = http.Get("https://api.example.com/data");\nif (res.statusCode == 200) {\n    // 成功\n} else if (res.statusCode == 404) {\n    // リソースが見つからない\n}',
  '• 0 は通信自体が失敗したことを意味する\n• 200-299 の範囲が成功 (IsSuccess() で判定可能)\n• 301/302 等のリダイレクトは WinHTTP が自動処理する'
],

'HTTPClient-body': [
  'std::string body',
  'HTTPResponseのレスポンスボディ。サーバーから返されたコンテンツ本体をstd::stringとして保持する。JSON APIレスポンスやHTMLページの内容が格納される。バイナリデータの場合はstd::stringに格納されるが、バイナリセーフ。',
  '// レスポンスボディの取得\nauto res = http.Get("https://api.example.com/config.json");\nif (res.IsSuccess()) {\n    std::string json = res.body;\n    // JSON パースなどの処理\n}',
  '• テキスト/バイナリどちらも格納可能\n• 大きなレスポンスはメモリ使用量に注意\n• 空のレスポンス (204 No Content等) では空文字列'
],

'HTTPClient-headers': [
  'std::unordered_map<std::string, std::string> headers',
  'HTTPResponseのレスポンスヘッダー。サーバーから返されたHTTPヘッダーをキー・値のペアで保持する。Content-Type、Content-Length、Set-Cookieなどの情報を取得できる。',
  '// レスポンスヘッダーの確認\nauto res = http.Get("https://api.example.com/data");\nif (res.headers.count("Content-Type")) {\n    std::string ct = res.headers["Content-Type"];\n}',
  '• ヘッダー名は大文字小文字を区別する場合がある\n• 同名ヘッダーが複数ある場合はカンマ結合される\n• レスポンスによっては空の場合もある'
],

'HTTPClient-IsSuccess': [
  'bool IsSuccess() const',
  'HTTPレスポンスが成功 (ステータスコード200-299) かどうかを判定する。通信エラーやサーバーエラーを簡易的にフィルタする際に便利なヘルパーメソッド。細かいエラー処理にはstatusCodeを直接確認する。',
  '// 成功判定\nGX::HTTPClient http;\nauto res = http.Get("https://api.example.com/data");\nif (res.IsSuccess()) {\n    // res.body を処理\n} else {\n    GX::Logger::Error("HTTP Error: %d", res.statusCode);\n}',
  '• statusCode >= 200 && statusCode < 300 で判定\n• 通信失敗 (statusCode == 0) も false\n• 3xx リダイレクトも false (WinHTTP が自動フォローする場合は到達しない)'
],

// --- HTTPClient class ---
'HTTPClient-Get': [
  'HTTPResponse Get(const std::string& url, const std::unordered_map<std::string, std::string>& headers = {})',
  '同期GETリクエストを送信する。レスポンスが返るまでブロックする。URLパースからHTTP通信、レスポンスの受信までを一括で行う。追加ヘッダーはオプションで指定可能。API呼び出しやファイルダウンロードに使用する。',
  '// 同期GETリクエスト\nGX::HTTPClient http;\nauto res = http.Get("https://api.example.com/config");\nif (res.IsSuccess()) {\n    std::string data = res.body;\n}\n\n// カスタムヘッダー付き\nauto res2 = http.Get("https://api.example.com/data",\n    {{ "Authorization", "Bearer token123" }});',
  '• ブロッキング呼び出しのためメインスレッドでは注意\n• 非同期版は GetAsync() を使用\n• WinHTTP API を内部使用\n• HTTPS (TLS) も自動でサポート'
],

'HTTPClient-Post': [
  'HTTPResponse Post(const std::string& url, const std::string& body, const std::string& contentType = "application/json", const std::unordered_map<std::string, std::string>& headers = {})',
  '同期POSTリクエストを送信する。リクエストボディとContent-Typeを指定してサーバーにデータを送信する。デフォルトのContent-Typeは "application/json"。RESTful APIへのデータ送信やフォーム送信に使用する。',
  '// JSON POSTリクエスト\nGX::HTTPClient http;\nstd::string json = "{\\\"score\\\": 1000, \\\"name\\\": \\\"Player1\\\"}";\nauto res = http.Post(\n    "https://api.example.com/scores", json);\nif (res.IsSuccess()) {\n    // 成功\n}',
  '• ブロッキング呼び出し\n• デフォルト Content-Type は "application/json"\n• フォーム送信は Content-Type を "application/x-www-form-urlencoded" に変更\n• 非同期版は PostAsync() を使用'
],

'HTTPClient-GetAsync': [
  'void GetAsync(const std::string& url, std::function<void(HTTPResponse)> callback)',
  '非同期GETリクエストを送信する。別スレッドで通信を行い、完了時にUpdate()でメインスレッドのコールバックを発火する。メインループをブロックしないため、ゲーム中のAPI呼び出しに適している。',
  '// 非同期GETリクエスト\nGX::HTTPClient http;\nhttp.GetAsync("https://api.example.com/leaderboard",\n    [](GX::HTTPResponse res) {\n        if (res.IsSuccess()) {\n            // メインスレッドで結果を処理\n        }\n    });\n// 毎フレーム Update() を呼ぶ\nhttp.Update();',
  '• 内部で std::thread (detached) を使用\n• コールバックは Update() 内でメインスレッドから呼ばれる\n• 複数の非同期リクエストを同時に発行可能\n• Update() を呼び忘れるとコールバックが発火されない'
],

'HTTPClient-PostAsync': [
  'void PostAsync(const std::string& url, const std::string& body, const std::string& contentType, std::function<void(HTTPResponse)> callback)',
  '非同期POSTリクエストを送信する。別スレッドで通信を行い、完了後にUpdate()でコールバックを発火する。スコア送信やログ報告など、レスポンスを待たずに続行したい場合に使用する。',
  '// 非同期POSTリクエスト\nGX::HTTPClient http;\nstd::string json = "{\\\"event\\\": \\\"levelComplete\\\"}";\nhttp.PostAsync(\n    "https://api.example.com/events",\n    json, "application/json",\n    [](GX::HTTPResponse res) {\n        // 送信結果を確認\n    });',
  '• contentType は省略不可 (同期版と異なりデフォルト値なし)\n• detached スレッドで実行される\n• コールバック内で http オブジェクトの寿命に注意\n• Update() でコールバック発火'
],

'HTTPClient-Update': [
  'void Update()',
  '完了した非同期リクエストのコールバックをメインスレッドで発火する。GetAsync/PostAsyncを使用する場合、毎フレームのゲームループ内で呼び出す必要がある。非同期リクエストを使用しない場合は呼び出し不要。',
  '// メインループでの非同期コールバック処理\nGX::HTTPClient http;\nhttp.GetAsync(url, callback);\n\nwhile (running) {\n    http.Update(); // 完了チェック + コールバック発火\n    // ... ゲームループ ...\n}',
  '• メインスレッドで呼ぶこと\n• 同期リクエスト (Get/Post) のみ使用する場合は不要\n• 複数完了リクエストがあれば1回の Update で全て発火\n• mutex で完了キューを保護している'
],

'HTTPClient-SetTimeout': [
  'void SetTimeout(int timeoutMs)',
  'リクエストのタイムアウトをミリ秒単位で設定する。デフォルトは30000ミリ秒 (30秒)。タイムアウトを超えるとリクエストが失敗し、statusCode=0のHTTPResponseが返される。同期・非同期両方のリクエストに適用される。',
  '// タイムアウトの設定\nGX::HTTPClient http;\nhttp.SetTimeout(5000); // 5秒\nauto res = http.Get("https://slow-api.example.com/data");\nif (!res.IsSuccess()) {\n    // タイムアウトまたはエラー\n}',
  '• デフォルトは 30000ms (30秒)\n• 0 を設定するとタイムアウト無効 (非推奨)\n• WinHTTP の接続・受信タイムアウト両方に適用\n• 非同期リクエストにも影響する'
],

// ============================================================
// WebSocket  (IO/Network/WebSocket.h)  —  page-WebSocket
// ============================================================

'WebSocket-Connect': [
  'bool Connect(const std::string& url)',
  'WebSocketサーバーに接続する。ws:// または wss:// のURLを指定する。内部でHTTPアップグレードハンドシェイクを行い、成功すると双方向通信が可能になる。接続後にバックグラウンドの受信スレッドが自動起動する。',
  '// WebSocket接続\nGX::WebSocket ws;\nws.onMessage = [](const std::string& msg) {\n    GX::Logger::Info("Received: %s", msg.c_str());\n};\nif (ws.Connect("wss://echo.example.com")) {\n    ws.Send("Hello WebSocket!");\n}',
  '• WinHTTP WebSocket API を内部使用\n• wss:// は TLS 暗号化接続\n• 接続後に受信スレッドが自動起動する\n• コールバックを先に設定してから Connect すること'
],

'WebSocket-Close': [
  'void Close()',
  'WebSocket接続を閉じる。WebSocketクロージングハンドシェイクを行い、受信スレッドを停止する。Close後にonCloseコールバックが発火される。切断後の再接続にはConnect()を再度呼ぶ。',
  '// WebSocket切断\nGX::WebSocket ws;\nws.onClose = []() {\n    GX::Logger::Info("WebSocket closed");\n};\nws.Connect("wss://server.example.com");\n// ... 通信 ...\nws.Close(); // クロージングハンドシェイク',
  '• デストラクタでも自動的に Close される\n• クロージングハンドシェイクを送信する\n• 受信スレッドが終了するまでブロックする場合がある\n• Close 後は IsConnected() が false になる'
],

'WebSocket-IsConnected': [
  'bool IsConnected() const',
  'WebSocket接続が有効かどうかを判定する。Connect()が成功しClose()が呼ばれていない場合にtrueを返す。サーバー側切断はUpdate()呼び出し時にonCloseで通知される。',
  '// 接続状態の確認\nGX::WebSocket ws;\nif (ws.Connect("wss://server.example.com")) {\n    while (ws.IsConnected()) {\n        ws.Update();\n        // ... 処理 ...\n    }\n}',
  '• atomic<bool> で管理されスレッドセーフ\n• サーバー側切断は受信スレッドが検出して false にする\n• Close() 呼び出し後は即座に false'
],

'WebSocket-Send': [
  'bool Send(const std::string& message)',
  'UTF-8テキストメッセージを送信する。WebSocketテキストフレームとして送信される。チャットメッセージやJSON文字列の送信に使用する。送信成功でtrue、接続切れ等のエラーでfalseを返す。',
  '// テキストメッセージ送信\nGX::WebSocket ws;\nws.Connect("wss://chat.example.com");\nstd::string json = "{\\\"type\\\": \\\"chat\\\", \\\"text\\\": \\\"Hello\\\"}";\nbool ok = ws.Send(json);',
  '• UTF-8 テキストフレームで送信される\n• 大きなメッセージもフレーミングは WinHTTP が自動処理\n• 接続中でない場合は false を返す\n• バイナリデータには SendBinary() を使用'
],

'WebSocket-SendBinary': [
  'bool SendBinary(const void* data, size_t size)',
  'バイナリデータを送信する。WebSocketバイナリフレームとして送信される。画像データ、シリアライズされたゲーム状態、プロトコルバッファなどの送信に使用する。',
  '// バイナリデータ送信\nGX::WebSocket ws;\nws.Connect("wss://game.example.com");\nuint8_t packet[] = { 0x01, 0x02, 0x03 };\nbool ok = ws.SendBinary(packet, sizeof(packet));',
  '• バイナリフレームとして送信される (テキストフレームとは区別される)\n• 受信側は onBinaryMessage コールバックで受け取る\n• 送信成功で true、エラーで false\n• 最大メッセージサイズはサーバー設定に依存'
],

'WebSocket-Update': [
  'void Update()',
  '受信スレッドからのメッセージキューを処理し、コールバックを発火する。onMessage (テキスト)、onBinaryMessage (バイナリ)、onClose (切断)、onError (エラー) のいずれかが該当すれば発火される。毎フレーム呼び出す必要がある。',
  '// メインループでのWebSocket更新\nGX::WebSocket ws;\nws.onMessage = [](const std::string& msg) { /* ... */ };\nws.onClose = []() { /* ... */ };\nws.Connect("wss://server.example.com");\n\nwhile (running) {\n    ws.Update(); // メッセージ処理\n    // ... ゲームループ ...\n}',
  '• メインスレッドで呼ぶこと (コールバックがメインスレッドで実行)\n• 受信スレッドが検出したメッセージをキューから取り出す\n• 複数メッセージが溜まっていれば1回の Update で全て処理\n• mutex でメッセージキューを保護'
],

// --- WebSocket callbacks ---
'WebSocket-onMessage': [
  'std::function<void(const std::string&)> onMessage',
  'テキストメッセージ受信時に呼ばれるコールバック。サーバーからのUTF-8テキストフレームを受信するとUpdate()内で発火される。JSONメッセージやチャットテキストの受信処理に設定する。',
  '// テキストメッセージ受信\nGX::WebSocket ws;\nws.onMessage = [](const std::string& msg) {\n    GX::Logger::Info("Received: %s", msg.c_str());\n    // JSON パースなど\n};\nws.Connect("wss://server.example.com");',
  '• Update() 内でメインスレッドから呼ばれる\n• Connect() 前に設定しておくこと\n• nullptr の場合はメッセージが破棄される\n• 複数メッセージが1フレームで到着することもある'
],

'WebSocket-onBinaryMessage': [
  'std::function<void(const void*, size_t)> onBinaryMessage',
  'バイナリメッセージ受信時に呼ばれるコールバック。サーバーからのバイナリフレームを受信するとUpdate()内で発火される。画像データやシリアライズされたゲームパケットの受信に使用する。',
  '// バイナリメッセージ受信\nGX::WebSocket ws;\nws.onBinaryMessage = [](const void* data, size_t size) {\n    const uint8_t* bytes = static_cast<const uint8_t*>(data);\n    // バイナリプロトコルの解析\n};\nws.Connect("wss://game.example.com");',
  '• ポインタの寿命はコールバック内のみ (コピーが必要なら明示的にコピー)\n• Update() 内でメインスレッドから呼ばれる\n• サイズは size パラメータで取得\n• テキストフレームは onMessage で別に処理される'
],

'WebSocket-onClose': [
  'std::function<void()> onClose',
  'WebSocket接続が閉じられた時に呼ばれるコールバック。サーバー側の切断、ネットワークエラー、またはClose()呼び出しにより発火される。再接続処理やUI更新に使用する。',
  '// 切断コールバック\nGX::WebSocket ws;\nws.onClose = []() {\n    GX::Logger::Info("Connection closed");\n    // 再接続ロジックなど\n};\nws.Connect("wss://server.example.com");',
  '• Update() 内でメインスレッドから呼ばれる\n• Close() 呼び出し時にも発火される\n• 1回の接続につき最大1回発火される\n• onError とは別に発火される場合がある'
],

'WebSocket-onError': [
  'std::function<void(const std::string&)> onError',
  'WebSocket通信でエラーが発生した時に呼ばれるコールバック。エラー内容を示す文字列が引数として渡される。ネットワーク障害やプロトコルエラーの際にログ記録やユーザー通知に使用する。',
  '// エラーコールバック\nGX::WebSocket ws;\nws.onError = [](const std::string& err) {\n    GX::Logger::Error("WebSocket error: %s", err.c_str());\n};\nws.Connect("wss://server.example.com");',
  '• Update() 内でメインスレッドから呼ばれる\n• エラー後は接続が無効になっている場合が多い\n• onClose の前に呼ばれることがある\n• エラー文字列は WinHTTP のエラー情報を含む'
],

// ============================================================
// MoviePlayer  (Movie/MoviePlayer.h)  —  page-MoviePlayer
// ============================================================

// --- MovieState enum ---
'MoviePlayer-Stopped': [
  'MovieState::Stopped',
  '動画が停止中の状態。Open()直後の初期状態、またはStop()呼び出し後の状態。再生位置は先頭にリセットされる。Play()で再生を開始できる。',
  '// 停止状態の確認\nGX::MoviePlayer player;\nplayer.Open("video.mp4", device, texMgr);\n// 初期状態は Stopped\nassert(player.GetState() == GX::MovieState::Stopped);\nplayer.Play(); // 再生開始',
  '• Open() 直後の初期状態\n• Stop() 呼び出しでこの状態に遷移\n• 再生位置は 0.0 にリセットされる\n• Play() で Playing に遷移する'
],

'MoviePlayer-Playing': [
  'MovieState::Playing',
  '動画が再生中の状態。Update()を毎フレーム呼び出すことでデコードが進行し、新しいフレームがテクスチャとして利用可能になる。GetTextureHandle()で現在のフレームを取得しSpriteBatchで描画する。',
  '// 再生中の処理\nplayer.Play();\nwhile (player.GetState() == GX::MovieState::Playing) {\n    if (player.Update(device)) {\n        // 新しいフレームが利用可能\n        int tex = player.GetTextureHandle();\n        spriteBatch.Draw(tex, 0, 0);\n    }\n}',
  '• Update() で毎フレームデコードが必要\n• フレームレートに合わせて自動的にスキップ/待機\n• 末尾到達で IsFinished() が true になる\n• Pause() で一時停止可能'
],

'MoviePlayer-Paused': [
  'MovieState::Paused',
  '動画が一時停止中の状態。Pause()で遷移し、再生位置は保持される。Play()で再開するか、Stop()で先頭に戻ることができる。一時停止中もGetTextureHandle()は最後のフレームを返す。',
  '// 一時停止と再開\nplayer.Play();\n// ... しばらく再生 ...\nplayer.Pause();\ndouble pos = player.GetPosition(); // 一時停止位置\nplayer.Play(); // 同じ位置から再開',
  '• 再生位置が保持される\n• GetTextureHandle() は最後にデコードしたフレームを返す\n• Play() で同じ位置から再開\n• Stop() で先頭に戻る'
],

// --- MoviePlayer class ---
'MoviePlayer-Open': [
  'bool Open(const std::string& filePath, GraphicsDevice& device, TextureManager& texManager)',
  '動画ファイルを開いて再生準備をする。Media Foundationを初期化し、IMFSourceReaderでファイルを開く。動画の幅・高さ・再生時間を取得し、フレームデコード用のテクスチャをTextureManagerに作成する。対応形式はMP4, WMV, AVIなど。',
  '// 動画ファイルを開く\nGX::MoviePlayer player;\nif (player.Open("Assets/cutscene.mp4", device, texMgr)) {\n    printf("Size: %ux%u, Duration: %.1fs\\n",\n           player.GetWidth(), player.GetHeight(),\n           player.GetDuration());\n    player.Play();\n}',
  '• Media Foundation (MFStartup) を内部で初期化\n• RGB32 出力に設定されるため、デコーダーが自動変換\n• テクスチャは TextureManager に 1 ハンドル作成される\n• 対応コーデックは OS にインストールされた Media Foundation デコーダーに依存'
],

'MoviePlayer-Close': [
  'void Close()',
  '動画を閉じてリソースを解放する。IMFSourceReaderの解放、テクスチャの破棄、Media Foundationのシャットダウンを行う。Close後はGetTextureHandle()が無効なハンドルを返す。',
  '// 動画のクローズ\nGX::MoviePlayer player;\nplayer.Open("video.mp4", device, texMgr);\nplayer.Play();\n// ... 再生 ...\nplayer.Close(); // リソース解放',
  '• デストラクタでも自動的に Close される\n• テクスチャハンドルは解放される\n• Media Foundation のシャットダウン (MFShutdown) も行う\n• 複数回呼び出しても安全'
],

'MoviePlayer-Play': [
  'void Play()',
  '動画の再生を開始または再開する。Stopped状態からは先頭から、Paused状態からは中断位置から再生を開始する。再生中に呼んでも影響はない。再生中はUpdate()を毎フレーム呼び出してデコードを進める必要がある。',
  '// 再生開始\nGX::MoviePlayer player;\nplayer.Open("video.mp4", device, texMgr);\nplayer.Play(); // 先頭から再生開始\n\n// ゲームループ\nwhile (!player.IsFinished()) {\n    player.Update(device);\n    int tex = player.GetTextureHandle();\n    spriteBatch.Draw(tex, 0, 0);\n}',
  '• Stopped → Playing: 先頭から再生\n• Paused → Playing: 中断位置から再開\n• Playing → Playing: 何もしない\n• Update() を呼ばないとフレームが進まない'
],

'MoviePlayer-Pause': [
  'void Pause()',
  '動画の再生を一時停止する。現在の再生位置を保持したまま停止する。Play()で同じ位置から再開できる。ポーズメニュー表示中やフォーカス喪失時に使用する。',
  '// 一時停止\nif (pauseRequested) {\n    player.Pause();\n}\n// 再開\nif (resumeRequested) {\n    player.Play();\n}',
  '• 再生位置が保持される\n• 最後のデコード済みフレームが表示され続ける\n• 非再生中に呼んでも影響はない\n• GetPosition() で一時停止位置を取得できる'
],

'MoviePlayer-Stop': [
  'void Stop()',
  '動画の再生を停止し、再生位置を先頭にリセットする。完全に停止して最初からやり直す場合に使用する。Pause()と異なり再生位置は保持されない。',
  '// 停止と最初から再生\nplayer.Stop(); // 先頭にリセット\nplayer.Play(); // 最初から再生',
  '• 再生位置は 0.0 にリセットされる\n• IsFinished() フラグもリセットされる\n• Close() と異なりリソースは解放されない\n• 再度 Play() で先頭から再生可能'
],

'MoviePlayer-Seek': [
  'void Seek(double timeSeconds)',
  '指定時刻にシークする。IMFSourceReaderのSetCurrentPositionを使用してキーフレーム単位でシークする。再生中でも一時停止中でも使用可能。指定時刻はGetDuration()の範囲内であること。',
  '// 動画の任意位置にジャンプ\nplayer.Play();\nplayer.Seek(30.0); // 30秒地点にシーク\n\n// 進捗バー操作\ndouble ratio = sliderValue; // 0.0〜1.0\nplayer.Seek(ratio * player.GetDuration());',
  '• キーフレーム単位のシークのため、正確な位置にはならない場合がある\n• 負の値や Duration を超える値の動作は未定義\n• シーク後の最初の Update() で新しいフレームがデコードされる\n• 圧縮形式によってシーク精度が異なる'
],

'MoviePlayer-Update': [
  'bool Update(GraphicsDevice& device)',
  '毎フレーム呼び出して動画のデコードを進める。フレーム間隔に基づいて次のフレームをデコードし、テクスチャを更新する。新しいフレームがデコードされた場合にtrueを返す。再生中のみデコードが行われ、停止中/一時停止中はfalseを返す。',
  '// 毎フレームの更新\nwhile (running) {\n    if (player.Update(device)) {\n        // 新しいフレームが利用可能\n    }\n    int tex = player.GetTextureHandle();\n    if (tex >= 0) {\n        spriteBatch.Draw(tex, 0.0f, 0.0f);\n    }\n}',
  '• Playing 状態でのみデコードが進行する\n• BGRA→RGBA 変換が内部で行われる\n• TextureManager::UpdateTextureFromMemory でテクスチャを毎フレーム更新\n• フレームレートに基づいて適切なタイミングでデコードされる'
],

'MoviePlayer-GetState': [
  'MovieState GetState() const',
  '現在の再生状態を取得する。Stopped、Playing、Pausedのいずれかを返す。UIの再生ボタン表示切り替えや状態に応じた処理分岐に使用する。',
  '// 再生状態に応じた処理\nswitch (player.GetState()) {\n    case GX::MovieState::Stopped:  /* 停止中 */ break;\n    case GX::MovieState::Playing:  /* 再生中 */ break;\n    case GX::MovieState::Paused:   /* 一時停止中 */ break;\n}',
  '• 軽量な取得操作 (メンバー変数を返すだけ)\n• IsFinished() が true でも状態は Playing のまま\n• 状態遷移は Play()/Pause()/Stop() で行う'
],

'MoviePlayer-GetDuration': [
  'double GetDuration() const',
  '動画の総再生時間を秒単位で返す。Open()で動画を開いた時点で取得される。シークバーの範囲設定や残り時間の計算に使用する。Open()前は0.0を返す。',
  '// 総再生時間の取得\nplayer.Open("video.mp4", device, texMgr);\ndouble total = player.GetDuration();\nprintf("Duration: %.1f seconds\\n", total);\n\n// 進捗率の計算\ndouble progress = player.GetPosition() / player.GetDuration();',
  '• Open() 後に有効な値が返される\n• 秒単位の double 値\n• フォーマットによっては正確でない場合がある\n• Open() 前は 0.0 を返す'
],

'MoviePlayer-GetPosition': [
  'double GetPosition() const',
  '現在の再生位置を秒単位で返す。再生中はUpdate()のたびに更新される。シークバーの表示やタイムコードの表示に使用する。Stopped状態では0.0、Paused状態では停止時の位置を返す。',
  '// 再生位置の表示\ndouble pos = player.GetPosition();\nint min = (int)pos / 60;\nint sec = (int)pos % 60;\nprintf("%02d:%02d / %.0f\\n", min, sec, player.GetDuration());',
  '• 秒単位の double 値\n• Update() のたびに更新される\n• Seek() 後は新しい位置が反映される\n• 精度はデコーダーのタイムスタンプに依存'
],

'MoviePlayer-GetTextureHandle': [
  'int GetTextureHandle() const',
  '現在のフレームのテクスチャハンドルを返す。TextureManagerで管理されるハンドルで、SpriteBatch.Draw()に渡して描画する。Open()前やClose()後は-1を返す。Update()で新フレームがデコードされるたびにテクスチャ内容が更新される。',
  '// テクスチャハンドルで描画\nint tex = player.GetTextureHandle();\nif (tex >= 0) {\n    spriteBatch.Begin();\n    spriteBatch.Draw(tex, 0.0f, 0.0f,\n                     (float)player.GetWidth(),\n                     (float)player.GetHeight());\n    spriteBatch.End();\n}',
  '• Open() で 1 つのテクスチャが作成される\n• -1 は無効なハンドル (Open前/Close後)\n• テクスチャ内容は Update() のたびに上書き更新される\n• SpriteBatch/Image ウィジェットなどで描画に使用'
],

'MoviePlayer-GetWidth': [
  'uint32_t GetWidth() const',
  '動画の幅をピクセル単位で返す。Open()で動画を開いた時点で取得される。描画領域のサイズ計算やアスペクト比の算出に使用する。Open()前は0を返す。',
  '// 動画サイズの取得\nplayer.Open("video.mp4", device, texMgr);\nuint32_t w = player.GetWidth();  // 例: 1920\nuint32_t h = player.GetHeight(); // 例: 1080\nfloat aspect = (float)w / (float)h;',
  '• Open() 後に有効な値が返される\n• デコード解像度 (ファイル内の解像度)\n• Open() 前は 0 を返す\n• スケーリングは SpriteBatch 側で行う'
],

'MoviePlayer-GetHeight': [
  'uint32_t GetHeight() const',
  '動画の高さをピクセル単位で返す。Open()で動画を開いた時点で取得される。GetWidth()とペアでアスペクト比の計算や描画矩形の決定に使用する。Open()前は0を返す。',
  '// アスペクト比を維持した描画\nfloat scale = std::min(\n    screenW / (float)player.GetWidth(),\n    screenH / (float)player.GetHeight());\nfloat drawW = player.GetWidth() * scale;\nfloat drawH = player.GetHeight() * scale;',
  '• Open() 後に有効な値が返される\n• GetWidth() とペアで使用\n• Open() 前は 0 を返す\n• 動画の元解像度がそのまま返される'
],

'MoviePlayer-IsFinished': [
  'bool IsFinished() const',
  '動画が最後まで再生されたかを判定する。最終フレームのデコード完了後にtrueになる。ループ再生する場合はIsFinished()がtrueになったらSeek(0)とPlay()で先頭に戻す。Stop()呼び出しでリセットされる。',
  '// ループ再生\nplayer.Play();\nwhile (running) {\n    player.Update(device);\n    if (player.IsFinished()) {\n        player.Stop();\n        player.Play(); // ループ\n    }\n    // 描画\n}',
  '• 最終フレームデコード後に true になる\n• GetState() は Playing のまま変わらない場合がある\n• Stop() で false にリセットされる\n• Seek() で巻き戻した場合も false にリセットされる'
]

}
