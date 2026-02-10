({

// ============================================================
// Buffer (Graphics/Resource/Buffer.h)
// ============================================================

'Buffer-Create': [
  'bool CreateVertexBuffer(ID3D12Device* device, const void* data, uint32_t size, uint32_t stride)',
  'GPU上に頂点バッファまたはインデックスバッファを作成します。内部でUPLOADヒープにバッファを確保し、指定データをコピーします。\nCreateVertexBufferは頂点データ用、CreateIndexBufferはインデックスデータ用です。初期化時に一度だけ呼び出してください。',
  '// 三角形の頂点バッファを作成\nstruct Vertex { float x, y, z; };\nVertex verts[] = { {0,1,0}, {1,-1,0}, {-1,-1,0} };\nGX::Buffer vb;\nvb.CreateVertexBuffer(device, verts, sizeof(verts), sizeof(Vertex));',
  '* UPLOADヒープを使用するため、CPUとGPU双方からアクセス可能だがGPU読み取り速度はDEFAULTヒープより遅い\n* 静的ジオメトリにはBufferを、毎フレーム更新されるデータにはDynamicBufferを使用すること\n* sizeは全データのバイト数、strideは1頂点あたりのバイト数を指定する'
],

'Buffer-Map': [
  'void* Map()',
  'バッファリソースをCPUアクセス可能な状態にマッピングします。返されたポインタを通じてバッファの内容を読み書きできます。\n使用後は必ずUnmap()を呼び出してください。',
  '// バッファの内容を更新する\nGX::Buffer buf;\nvoid* ptr = buf.Map();\nmemcpy(ptr, newData, dataSize);\nbuf.Unmap();',
  '* Map中にGPUがこのバッファを参照するとデータ競合が発生するため、GPUコマンド完了後に呼ぶこと\n* UPLOADヒープバッファのみマッピング可能（DEFAULTヒープは不可）\n* 失敗時はnullptrが返される'
],

'Buffer-Unmap': [
  'void Unmap()',
  'Map()で取得したCPUアクセスを解除します。Unmap後はMap()で取得したポインタは無効になります。\n必ずMap()とペアで使用してください。',
  '// Map/Unmapのペア使用\nvoid* ptr = buffer.Map();\nmemcpy(ptr, data, size);\nbuffer.Unmap();',
  '* Map()とUnmap()は必ず1対1で呼び出すこと\n* Unmap後にMap()のポインタにアクセスすると未定義動作になる'
],

'Buffer-GetResource': [
  'ID3D12Resource* GetResource() const',
  'バッファの内部D3D12リソースへのポインタを返します。リソースバリアの発行やデバッグ時に使用します。\n通常の描画ではGetVertexBufferView/GetIndexBufferViewを使います。',
  '// リソースバリアの発行例\nID3D12Resource* res = buffer.GetResource();\nD3D12_RESOURCE_BARRIER barrier = {};\nbarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;\nbarrier.Transition.pResource = res;',
  '* 返されるポインタの所有権はBufferクラスが保持する（外部でReleaseしないこと）\n* Bufferが破棄されるとポインタは無効になる'
],

'Buffer-GetGPUAddress': [
  'D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const',
  'バッファのGPU仮想アドレスを取得します。ルートシグネチャでCBVやSRVとして直接バインドする際に使用します。\nVertexBufferViewやIndexBufferViewの内部でも利用されるアドレスです。',
  '// 定数バッファとしてバインド\nD3D12_GPU_VIRTUAL_ADDRESS addr = buffer.GetGPUAddress();\ncmdList->SetGraphicsRootConstantBufferView(0, addr);',
  '* ID3D12Resource::GetGPUVirtualAddress()のラッパー\n* リソースが未作成の場合は0が返される\n* ルートSRVはTexture2D.Sample()には使えず、バッファ用途のみ'
],

'Buffer-GetSize': [
  'uint32_t GetSize() const',
  'バッファの総サイズをバイト単位で返します。作成時に指定したsizeパラメータと同じ値です。\nデバッグやバリデーションに使用します。',
  '// バッファサイズの確認\nGX::Buffer buf;\nbuf.CreateVertexBuffer(device, data, 1024, 32);\nuint32_t sz = buf.GetSize(); // 1024',
  '* Create系メソッドが成功した後のみ有効な値が返る\n* 頂点バッファビューのSizeInBytesと一致する'
],

// ============================================================
// DynamicBuffer (Graphics/Resource/DynamicBuffer.h)
// ============================================================

'DynamicBuffer-Create': [
  'bool Initialize(ID3D12Device* device, uint32_t maxSize, uint32_t stride)',
  'ダブルバッファリング対応のUPLOADヒープバッファを2本作成します。maxSizeは1バッファあたりの最大バイト数です。\nSpriteBatchやPrimitiveBatchの頂点データなど毎フレーム更新されるデータに最適です。',
  '// SpriteBatch用の動的頂点バッファ作成\nGX::DynamicBuffer dynBuf;\nbool ok = dynBuf.Initialize(device, 4096 * 4 * sizeof(Vertex), sizeof(Vertex));\n// 4096スプライト x 4頂点分を確保',
  '* 内部でk_BufferCount(=2)個のバッファを作成しダブルバッファリングを行う\n* maxSizeを超える書き込みは未定義動作になるため余裕を持って確保すること\n* strideは頂点バッファビュー生成時に使用される'
],

'DynamicBuffer-Map': [
  'void* Map(uint32_t frameIndex)',
  '指定フレームのバッファをCPU書き込み用にマッピングします。frameIndexは現在のフレーム番号（0または1）を渡します。\nGPUが読み込み中の前フレームとは異なるバッファが返されるため安全に書き込めます。',
  '// 毎フレームの頂点データ書き込み\nuint32_t fi = swapChain.GetCurrentBackBufferIndex();\nvoid* ptr = dynBuf.Map(fi);\nmemcpy(ptr, vertices, vertCount * sizeof(Vertex));\ndynBuf.Unmap(fi);',
  '* frameIndexはSwapChainのバックバッファインデックスと同期させること\n* 同一フレームで複数回Mapする場合はオフセットを手動管理する必要がある\n* 失敗時はnullptrが返される'
],

'DynamicBuffer-Unmap': [
  'void Unmap(uint32_t frameIndex)',
  '指定フレームのバッファのマッピングを解除します。Map()で取得したポインタは使用不可になります。\nMap/Unmapは必ずペアで呼び出してください。',
  '// Map/Unmapのペア使用\nvoid* p = dynBuf.Map(frameIndex);\n// ... データ書き込み ...\ndynBuf.Unmap(frameIndex);',
  '* Map()とUnmap()のframeIndexは同じ値を渡すこと\n* 描画コマンド発行前にUnmapを完了させること'
],

'DynamicBuffer-GetGPUAddress': [
  'D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32_t frameIndex) const',
  '指定フレームのバッファのGPU仮想アドレスを取得します。定数バッファ用途でSetGraphicsRootConstantBufferViewに渡す際に使用します。\nframeIndexで適切なダブルバッファを選択します。',
  '// 定数バッファとしてバインド\nauto gpuAddr = dynBuf.GetGPUVirtualAddress(frameIndex);\ncmdList->SetGraphicsRootConstantBufferView(0, gpuAddr);',
  '* 毎フレーム正しいframeIndexを渡してバッファを切り替えること\n* Initializeが未完了の場合は0が返される'
],

'DynamicBuffer-GetVBView': [
  'D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView(uint32_t frameIndex, uint32_t usedSize) const',
  '指定フレームの頂点バッファビューを生成して返します。usedSizeは実際に使用したバイト数を指定し、GPU側で無駄な領域を読まないようにします。\nIASetVertexBuffersに渡して使用します。',
  '// 頂点バッファビューの取得とバインド\nuint32_t usedBytes = spriteCount * 4 * sizeof(Vertex);\nauto vbv = dynBuf.GetVertexBufferView(frameIndex, usedBytes);\ncmdList->IASetVertexBuffers(0, 1, &vbv);',
  '* usedSizeはmaxSize以下であること（超過すると不正な読み取りが発生）\n* strideはInitialize時に設定した値が自動で使われる\n* 毎フレームのスプライト数変動に対応するためusedSizeを可変にできる'
],

'DynamicBuffer-GetSize': [
  'uint32_t GetMaxSize() const',
  '1フレーム分のバッファの最大サイズをバイト単位で返します。Initialize時に指定したmaxSizeと同じ値です。\nバッファオーバーフローの事前チェックに使います。',
  '// 書き込み前のオーバーフローチェック\nuint32_t needed = spriteCount * 4 * sizeof(Vertex);\nif (needed > dynBuf.GetMaxSize()) {\n    GX::Logger::Warn(L"Buffer overflow!\");\n    return;\n}',
  '* 総バッファメモリ使用量はGetMaxSize() * k_BufferCount（=2）となる\n* 足りない場合はmaxSizeを大きくして再Initialize（またはフラッシュ分割）'
],

// ============================================================
// Texture (Graphics/Resource/Texture.h)
// ============================================================

'Texture-LoadFromFile': [
  'bool LoadFromFile(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, const std::wstring& filePath, DescriptorHeap* srvHeap, uint32_t srvIndex)',
  '画像ファイルからテクスチャを読み込み、GPUにアップロードします。内部でstb_imageを使用してPNG/JPG/BMP等をデコードし、ステージングバッファ経由でDEFAULTヒープにコピーします。\nSRVも同時に作成され、シェーダーからアクセス可能になります。',
  '// テクスチャファイルの読み込み\nGX::Texture tex;\nbool ok = tex.LoadFromFile(device, cmdQueue,\n    L\"Assets/player.png\", &srvHeap, 0);\nif (!ok) { /* エラー処理 */ }',
  '* stb_imageが対応するフォーマット（PNG, JPG, BMP, TGA, GIF等）を読み込み可能\n* RGBA8形式でGPUにアップロードされる（アルファチャンネルがない画像は自動でA=255が追加）\n* コマンドキューでコピーコマンドを実行するため、呼び出し時にGPU同期が発生する'
],

'Texture-CreateFromMemory': [
  'bool CreateFromMemory(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, const void* pixels, uint32_t width, uint32_t height, DescriptorHeap* srvHeap, uint32_t srvIndex)',
  'CPUメモリ上のRGBAピクセルデータからテクスチャを作成します。フォントアトラスやプロシージャル生成テクスチャなど、ファイルを経由しないテクスチャ作成に使用します。\nピクセルデータはRGBA8（1ピクセル4バイト）形式で渡してください。',
  '// プロシージャルテクスチャの作成\nstd::vector<uint8_t> pixels(256 * 256 * 4, 255);\nGX::Texture tex;\ntex.CreateFromMemory(device, cmdQueue,\n    pixels.data(), 256, 256, &srvHeap, 1);',
  '* pixelsはwidth * height * 4バイトのRGBA8データであること\n* 内部でステージングバッファ→DEFAULTヒープコピーが行われる\n* SoftImageのCreateTextureと組み合わせるとピクセル操作→GPU転送が容易'
],

'Texture-UploadToGPU': [
  'bool UploadToGPU(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, const void* pixels, uint32_t width, uint32_t height, DescriptorHeap* srvHeap, uint32_t srvIndex)',
  'テクスチャデータをGPUのDEFAULTヒープにアップロードします。ステージングバッファ（UPLOADヒープ）を経由してGPU専用メモリにコピーし、SRVを作成します。\nLoadFromFileやCreateFromMemoryの内部で使用されるプライベートメソッドです。',
  '// 通常はLoadFromFileまたはCreateFromMemoryを使用\n// UploadToGPUは内部で自動的に呼ばれる\nGX::Texture tex;\ntex.LoadFromFile(device, cmdQueue, L\"image.png\", &heap, 0);',
  '* UPLOADヒープ→CopyTextureRegion→DEFAULTヒープの2段階転送\n* コマンドキューでフェンス待機するためCPU-GPUの同期コストが発生する\n* 大量テクスチャの読み込み時はAsyncLoaderの使用を検討すること'
],

'Texture-UpdatePixels': [
  'bool UpdatePixels(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, const void* pixels, uint32_t width, uint32_t height)',
  '既存テクスチャのピクセルデータをインプレースで更新します。リソースとSRVは維持されるため、ハンドルやGPUディスクリプタはそのまま使えます。\nフォントアトラスの動的更新や動画フレームの書き換えに使用します。',
  '// フォントアトラスの更新\ntex.UpdatePixels(device, cmdQueue,\n    newAtlasPixels.data(), 2048, 2048);\n// SRVハンドルは変わらないので再バインド不要',
  '* PIXEL_SHADER_RESOURCE→COPY_DEST→コピー→PIXEL_SHADER_RESOURCEのステート遷移が内部で行われる\n* フェンス待機で同期するためフレーム途中での呼び出しはパフォーマンスに影響する\n* width/heightは元のテクスチャサイズと一致させること'
],

'Texture-GetResource': [
  'ID3D12Resource* GetResource() const',
  'テクスチャのD3D12リソースポインタを返します。リソースバリアの発行やCopyResourceなど低レベル操作に使用します。\n通常の描画ではGetSRVGPUHandle()でシェーダーからアクセスします。',
  '// リソースバリア発行例\nD3D12_RESOURCE_BARRIER barrier = {};\nbarrier.Transition.pResource = tex.GetResource();\nbarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;\nbarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;',
  '* Textureが破棄されるとポインタは無効になる\n* 所有権はTextureクラスが保持しているため外部でReleaseしないこと'
],

'Texture-GetWidth': [
  'uint32_t GetWidth() const',
  'テクスチャの幅をピクセル単位で返します。描画時のサイズ計算やアスペクト比の算出に使用します。\nLoadFromFileまたはCreateFromMemory成功後に有効な値が返されます。',
  '// テクスチャのアスペクト比を計算\nfloat aspect = (float)tex.GetWidth() / (float)tex.GetHeight();\nfloat drawH = 100.0f;\nfloat drawW = drawH * aspect;',
  '* 未ロード状態では0が返される\n* stb_imageでデコードされた画像の実サイズが反映される'
],

'Texture-GetHeight': [
  'uint32_t GetHeight() const',
  'テクスチャの高さをピクセル単位で返します。描画時のサイズ計算やUV座標の算出に使用します。\nLoadFromFileまたはCreateFromMemory成功後に有効な値が返されます。',
  '// スプライトシート1コマのUV計算\nfloat frameV = 64.0f / (float)tex.GetHeight();',
  '* 未ロード状態では0が返される\n* GetWidth()と合わせて描画矩形の計算に使用する'
],

'Texture-GetFormat': [
  'DXGI_FORMAT GetFormat() const',
  'テクスチャのDXGIフォーマットを返します。通常はDXGI_FORMAT_R8G8B8A8_UNORMです。\nHDRテクスチャやフォーマット依存の処理分岐に使用します。',
  '// フォーマットに応じた処理\nif (tex.GetFormat() == DXGI_FORMAT_R16G16B16A16_FLOAT) {\n    // HDRテクスチャ用の処理\n}',
  '* LoadFromFileではRGBA8_UNORMが使用される\n* RenderTargetから取得したテクスチャはR16G16B16A16_FLOATの場合がある'
],

// ============================================================
// TextureManager (Graphics/Resource/TextureManager.h)
// ============================================================

'TextureManager-Initialize': [
  'bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue)',
  'テクスチャマネージャーを初期化します。内部でShader-visible SRVディスクリプタヒープ（最大256スロット）を作成します。\nアプリケーション起動時に一度だけ呼び出してください。',
  '// テクスチャマネージャーの初期化\nGX::TextureManager texMgr;\ntexMgr.Initialize(device, cmdQueue);',
  '* k_MaxTextures(=256)個までのテクスチャを同時管理可能\n* SpriteBatchが内部にTextureManagerを持つため、通常は直接作成する必要はない\n* SRVヒープはShader-visible（GPUからアクセス可能）で作成される'
],

'TextureManager-LoadTexture': [
  'int LoadTexture(const std::wstring& filePath)',
  'ファイルパスからテクスチャを読み込み、ハンドル（int）を返します。同じパスで2回目以降の呼び出しはキャッシュから返すため高速です。\nDXLibのLoadGraph()に相当する機能です。失敗時は-1が返されます。',
  '// テクスチャの読み込みと使用\nint handle = texMgr.LoadTexture(L\"Assets/player.png\");\nif (handle < 0) {\n    GX::Logger::Error(L\"Failed to load texture\");\n}\n// 以降handleを使って描画',
  '* パスキャッシュにより同一画像の二重ロードを防止\n* フリーリスト方式でSRVインデックスを管理（解放後に再利用される）\n* ワイド文字列（std::wstring）でパスを指定する'
],

'TextureManager-CreateTextureFromMemory': [
  'int CreateTextureFromMemory(const void* pixels, uint32_t width, uint32_t height)',
  'CPUメモリ上のRGBAピクセルデータからテクスチャを作成しハンドルを返します。フォントアトラスやプロシージャル生成画像のGPUアップロードに使用します。\n失敗時は-1が返されます。',
  '// ピクセルデータからテクスチャ作成\nstd::vector<uint8_t> pixels(128 * 128 * 4);\n// ... ピクセルデータを生成 ...\nint handle = texMgr.CreateTextureFromMemory(\n    pixels.data(), 128, 128);',
  '* パスキャッシュには登録されない（毎回新規ハンドルが割り当てられる）\n* SoftImage::CreateTextureの内部でも使用されている\n* pixelsのデータはコピーされるため、呼び出し後にピクセルバッファを解放可能'
],

'TextureManager-UpdateTextureFromMemory': [
  'bool UpdateTextureFromMemory(int handle, const void* pixels, uint32_t width, uint32_t height)',
  '既存のテクスチャハンドルのピクセルデータをインプレースで更新します。SRVやハンドルは維持されるため、参照元のコードを変更する必要がありません。\nフォントアトラスの遅延アップロードや動画フレーム更新に最適です。',
  '// フォントアトラスの動的更新\ntexMgr.UpdateTextureFromMemory(\n    atlasHandle, atlasPixels.data(), 2048, 2048);\n// atlasHandleを使用する描画コードは変更不要',
  '* 内部でTexture::UpdatePixels()に委譲する\n* GPU同期（フェンス待機）が発生するためフレーム境界での呼び出し推奨\n* width/heightは元のテクスチャと一致させること'
],

'TextureManager-ReleaseTexture': [
  'void ReleaseTexture(int handle)',
  'テクスチャを解放し、ハンドルとSRVインデックスをフリーリストに返却します。解放後のハンドルでGetTextureやDrawGraphを呼ぶと未定義動作になります。\n不要になったテクスチャは積極的に解放してリソースを節約してください。',
  '// テクスチャの解放\ntexMgr.ReleaseTexture(handle);\nhandle = -1; // 無効化しておく',
  '* フリーリスト方式により解放されたインデックスは次のLoadTextureで再利用される\n* 同じパスを再度LoadTextureすると新しいハンドルで再ロードされる（キャッシュも削除される）\n* -1などの無効ハンドルを渡しても安全（何も起こらない）'
],

'TextureManager-GetGPUHandle': [
  'D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(int handle) const',
  'テクスチャハンドルに対応するSRVのGPUディスクリプタハンドルを返します。シェーダーのテクスチャスロットにバインドする際に使用します。\n内部でTexture::GetSRVGPUHandle()に委譲します。',
  '// テクスチャをシェーダーにバインド\nauto gpuHandle = texMgr.GetGPUHandle(handle);\ncmdList->SetGraphicsRootDescriptorTable(1, gpuHandle);',
  '* 無効なハンドルを渡すとクラッシュの原因になる（事前にIsValidで確認推奨）\n* SpriteBatchは内部でこのメソッドを使いテクスチャを自動バインドする'
],

'TextureManager-GetSRVIndex': [
  'int GetSRVIndex(int handle) const',
  'テクスチャハンドルに対応するSRVヒープ内のインデックスを返します。CopyDescriptorsSimpleなどでディスクリプタをコピーする際に使用します。\n通常の描画ではGetGPUHandleの方が便利です。',
  '// SRVインデックスの取得\nint srvIdx = texMgr.GetSRVIndex(texHandle);\n// カスタムヒープへのコピー等に使用',
  '* フリーリストから割り当てられたインデックスのため連番とは限らない\n* DepthOfFieldなどの専用ヒープへのコピー時に使用される'
],

'TextureManager-GetWidth': [
  'uint32_t GetWidth(int handle) const',
  'テクスチャハンドルに対応する画像の幅をピクセル単位で返します。\n内部のTexture::GetWidth()に委譲します。',
  '// テクスチャサイズの取得\nuint32_t w = texMgr.GetWidth(handle);\nuint32_t h = texMgr.GetHeight(handle);',
  '* GetHeight()と合わせてアスペクト比の算出に使用する\n* UV矩形のみのハンドル（分割テクスチャ）では元テクスチャのサイズが返る'
],

'TextureManager-GetHeight': [
  'uint32_t GetHeight(int handle) const',
  'テクスチャハンドルに対応する画像の高さをピクセル単位で返します。\n内部のTexture::GetHeight()に委譲します。',
  '// 描画サイズの計算\nfloat scale = 2.0f;\nfloat w = texMgr.GetWidth(handle) * scale;\nfloat h = texMgr.GetHeight(handle) * scale;',
  '* 分割テクスチャ（リージョン）では元画像全体の高さが返される点に注意\n* スプライトシートのフレームサイズはSpriteSheetクラスで管理する'
],

'TextureManager-IsValid': [
  'bool IsValid(int handle) const',
  'テクスチャハンドルが有効かどうかを判定します。解放済みのハンドルや範囲外の値に対してfalseを返します。\nGetGPUHandleやGetTextureを呼ぶ前の安全性チェックに使用します。',
  '// 安全なテクスチャアクセス\nif (texMgr.IsValid(handle)) {\n    spriteBatch.DrawGraph(100, 50, handle);\n}',
  '* -1（ロード失敗時の戻り値）に対してもfalseが返される\n* ReleaseTexture後のハンドルもfalseになる'
],

// ============================================================
// RenderTarget (Graphics/Resource/RenderTarget.h)
// ============================================================

'RenderTarget-Create': [
  'bool Create(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM)',
  'オフスクリーンレンダーターゲットを作成します。RTVとSRVの両方のディスクリプタヒープを内部で確保します。\nHDRパイプラインではR16G16B16A16_FLOAT、LDRではR8G8B8A8_UNORMを指定します。',
  '// HDRレンダーターゲットの作成\nGX::RenderTarget hdrRT;\nhdrRT.Create(device, 1280, 720,\n    DXGI_FORMAT_R16G16B16A16_FLOAT);\n// ポストエフェクトのping-pong用に2枚作成することが多い',
  '* 内部でRTVヒープ（1スロット）とSRVヒープ（1スロット）を作成する\n* 初期ステートはD3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE\n* ポストエフェクトパイプラインではHDR RT×2 + LDR RT×2の計4枚を使用する'
],

'RenderTarget-TransitionTo': [
  'void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState)',
  'リソースバリアを発行してレンダーターゲットのステートを遷移します。描画先にする時はRENDER_TARGET、テクスチャとして読む時はPIXEL_SHADER_RESOURCEに遷移させます。\n現在のステートと同じステートへの遷移は自動スキップされます。',
  '// レンダーターゲットへの描画と読み取り\nrt.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);\n// ... 描画 ...\nrt.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);\n// テクスチャとして使用',
  '* 内部でm_currentStateを追跡し、同一ステートの冗長バリアを防止する\n* D3D12ではステート不一致でGPU例外が発生するため確実に遷移させること\n* PostEffectチェーンのping-pong RTで頻繁に使用される'
],

'RenderTarget-GetRTVHandle': [
  'D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const',
  'レンダーターゲットビュー（RTV）のCPUディスクリプタハンドルを返します。OMSetRenderTargetsに渡してこのRTを描画先に設定する際に使用します。',
  '// レンダーターゲットを描画先に設定\nauto rtv = rt.GetRTVHandle();\nauto dsv = depthBuffer.GetDSVHandle();\ncmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);',
  '* TransitionToでRENDER_TARGETステートにしてから使用すること\n* 内部RTVヒープから取得されるため、外部でRTVヒープを用意する必要はない'
],

'RenderTarget-GetSRVGPUHandle': [
  'D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const',
  'SRVのGPUディスクリプタハンドルを返します。レンダーターゲットをテクスチャとしてシェーダーから読み取る際に使用します。\nポストエフェクトの入力テクスチャとして渡す場合に使います。',
  '// ポストエフェクトの入力として使用\nrt.TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);\nauto srv = rt.GetSRVGPUHandle();\ncmdList->SetGraphicsRootDescriptorTable(0, srv);',
  '* PIXEL_SHADER_RESOURCEステートで使用すること\n* 描画先（RTV）と入力（SRV）を同時に使うことはD3D12では不可\n* 自前SRVヒープをバインドする必要がある場合はGetSRVHeap()を使用する'
],

'RenderTarget-GetSRVIndex': [
  'int GetSRVIndex() const',
  'SRVヒープ内のインデックスを返します。ディスクリプタコピーなど低レベル操作で使用します。\n内部SRVヒープは1スロットのみのため常に0が返されます。',
  '// SRVインデックスの取得\nint idx = rt.GetSRVIndex();',
  '* 内部SRVヒープのインデックスであり、TextureManagerのSRVヒープとは異なる\n* CopyDescriptorsSimpleでカスタムヒープにコピーする際に使用'
],

'RenderTarget-GetResource': [
  'ID3D12Resource* GetResource() const',
  'レンダーターゲットのD3D12リソースポインタを返します。CopyResourceやResolveなどの低レベル操作に使用します。',
  '// ヒストリバッファへのコピー\ncmdList->CopyResource(\n    historyRT.GetResource(),\n    currentRT.GetResource());',
  '* TAA等のヒストリバッファ更新でCopyResourceに渡す用途がある\n* 所有権はRenderTargetクラスが保持する'
],

'RenderTarget-Clear': [
  'void Clear(ID3D12GraphicsCommandList* cmdList, const float color[4])',
  'レンダーターゲットを指定色でクリアします。毎フレームの描画開始前にバックグラウンド色で初期化するために使用します。\nRENDER_TARGETステートで呼び出す必要があります。',
  '// レンダーターゲットのクリア\nfloat clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };\nrt.TransitionTo(cmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);\ncmdList->ClearRenderTargetView(\n    rt.GetRTVHandle(), clearColor, 0, nullptr);',
  '* ClearRenderTargetViewのラッパー\n* HDR RTの場合はクリア値もHDR範囲の値を使用可能\n* RENDER_TARGETステートでないとGPU例外が発生する'
],

'RenderTarget-GetWidthGetHeight': [
  'uint32_t GetWidth() const / uint32_t GetHeight() const',
  'レンダーターゲットの幅と高さをピクセル単位で返します。ビューポートの設定やリサイズ判定に使用します。\nCreate時に指定したサイズが返されます。',
  '// ビューポートの設定\nD3D12_VIEWPORT vp = {};\nvp.Width  = (float)rt.GetWidth();\nvp.Height = (float)rt.GetHeight();\nvp.MaxDepth = 1.0f;\ncmdList->RSSetViewports(1, &vp);',
  '* リサイズ時はRenderTargetを再作成する必要がある（サイズ変更不可）\n* ポストエフェクトでhalf-res RTを使う場合はGetWidth()/2を計算して作成する'
],

// ============================================================
// DepthBuffer (Graphics/Resource/DepthBuffer.h)
// ============================================================

'DepthBuffer-Create': [
  'bool Create(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT)',
  '深度バッファを作成します（DSVのみ）。3D描画でのZテストに使用します。\nフォーマットはD32_FLOAT（32bit浮動小数点深度）がデフォルトです。',
  '// メインの深度バッファ作成\nGX::DepthBuffer depthBuf;\ndepthBuf.Create(device, 1280, 720);',
  '* DSVヒープ（1スロット）を内部で作成する\n* 初期ステートはD3D12_RESOURCE_STATE_DEPTH_WRITE\n* SRVは作成されないため、シェーダーからの深度読み取りにはCreateWithOwnSRVを使用する'
],

'DepthBuffer-CreateWithOwnSRV': [
  'bool CreateWithOwnSRV(ID3D12Device* device, uint32_t width, uint32_t height)',
  'DSV+自前のshader-visible SRVヒープ付きで深度バッファを作成します。SSAO、DoF、MotionBlurなどポストエフェクトから深度を読み取る場合に使用します。\n内部でD32_FLOATリソースに対してR32_FLOATフォーマットのSRVを作成します。',
  '// SSAO用の深度バッファ作成\nGX::DepthBuffer mainDepth;\nmainDepth.CreateWithOwnSRV(device, 1280, 720);\n// GetSRVGPUHandle()でシェーダーから深度読み取り可能',
  '* D3D12ではD32_FLOATをSRVとして読む場合R32_FLOATフォーマットを指定する\n* Shader-visible SRVヒープが別途作成されるためメモリ使用量が増加する\n* HasOwnSRV()でSRV有無を確認できる'
],

'DepthBuffer-TransitionTo': [
  'void TransitionTo(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES newState)',
  '深度バッファのリソースステートを遷移します。描画時はDEPTH_WRITE、シェーダーから読む時はPIXEL_SHADER_RESOURCEに遷移させます。\n同一ステートへの遷移は自動スキップされます。',
  '// 深度バッファの読み取り用遷移\ndepthBuf.TransitionTo(cmdList,\n    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);\n// SSAOシェーダーで深度を参照\n// ...\ndepthBuf.TransitionTo(cmdList,\n    D3D12_RESOURCE_STATE_DEPTH_WRITE);',
  '* DEPTH_WRITEとPIXEL_SHADER_RESOURCEを同時に指定することはできない\n* ポストエフェクトチェーンでは描画後にSRVに遷移→エフェクト処理→DEPTH_WRITEに戻す流れ'
],

'DepthBuffer-GetDSVHandle': [
  'D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const',
  '深度ステンシルビュー（DSV）のCPUディスクリプタハンドルを返します。OMSetRenderTargetsの第4引数に渡して深度テストを有効にします。',
  '// 深度バッファを描画に設定\nauto rtv = renderTarget.GetRTVHandle();\nauto dsv = depthBuf.GetDSVHandle();\ncmdList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);',
  '* DEPTH_WRITEステートで使用すること\n* 2D描画時は深度テスト無効のため通常は不要'
],

'DepthBuffer-GetSRVGPUHandle': [
  'D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle() const',
  'SRVのGPUディスクリプタハンドルを返します。CreateWithSRVまたはCreateWithOwnSRVで作成した場合のみ有効です。\nSSAOやDoFなどのポストエフェクトが深度を参照する際に使用します。',
  '// SSAOシェーダーに深度テクスチャをバインド\nauto depthSRV = depthBuf.GetSRVGPUHandle();\ncmdList->SetGraphicsRootDescriptorTable(1, depthSRV);',
  '* CreateWithSRV/CreateWithOwnSRVを使わずに作成した場合、無効なハンドルが返される\n* PIXEL_SHADER_RESOURCEステートで使用すること\n* シャドウマップ用のCreateWithSRVは外部SRVヒープを使用する'
],

'DepthBuffer-Clear': [
  'void Clear(ID3D12GraphicsCommandList* cmdList, float depth = 1.0f)',
  '深度バッファを指定した深度値でクリアします。デフォルトは1.0f（最大深度）です。\n毎フレームの描画開始前に呼び出して深度をリセットします。',
  '// 深度バッファのクリア（デフォルト=1.0）\ncmdList->ClearDepthStencilView(\n    depthBuf.GetDSVHandle(),\n    D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);',
  '* DEPTH_WRITEステートで呼び出すこと\n* リバースZを使用する場合はdepth=0.0fを指定する\n* ClearDepthStencilViewのラッパー'
],

// ============================================================
// SoftImage (Graphics/Resource/SoftImage.h)
// ============================================================

'SoftImage-Create': [
  'bool Create(uint32_t width, uint32_t height)',
  '指定サイズの空のCPU画像バッファを作成します。全ピクセルは0x00000000（透明黒）で初期化されます。\nプロシージャル生成やピクセル単位の画像操作に使用します。',
  '// 256x256の空画像を作成\nGX::SoftImage img;\nimg.Create(256, 256);\n// DrawPixelでピクセルを描き込む',
  '* 内部でwidth * height * 4バイトのメモリを確保する（RGBA8形式）\n* GPUリソースは作成されない（CPU側のみ）\n* CreateTexture()でGPUにアップロード可能'
],

'SoftImage-LoadFromFile': [
  'bool LoadFromFile(const std::wstring& filePath)',
  'ファイルから画像を読み込みCPUメモリに展開します。stb_imageを内部で使用してデコードします。\n読み込み後にGetPixel/DrawPixelでピクセル操作が可能です。',
  '// 画像ファイルの読み込みとピクセル操作\nGX::SoftImage img;\nimg.LoadFromFile(L\"Assets/heightmap.png\");\nuint32_t pixel = img.GetPixel(10, 20);',
  '* PNG, JPG, BMP, TGA, GIF等の形式に対応\n* 全ピクセルがRGBA8形式でメモリに展開される\n* 大きな画像はメモリを大量に消費する（4096x4096で64MB）'
],

'SoftImage-SetPixel': [
  'void DrawPixel(int x, int y, uint32_t color)',
  '指定座標のピクセルをカラー値で書き込みます。color形式は0xAARRGGBB（DXLib互換）です。\n範囲外の座標は無視されます。',
  '// グラデーション画像の生成\nGX::SoftImage img;\nimg.Create(256, 256);\nfor (int y = 0; y < 256; y++) {\n    for (int x = 0; x < 256; x++) {\n        uint32_t c = 0xFF000000 | (x << 16) | (y << 8);\n        img.DrawPixel(x, y, c);\n    }\n}',
  '* 0xAARRGGBB形式でカラーを指定（Aが最上位バイト）\n* 範囲外アクセスは安全にスキップされる\n* 大量のピクセル操作はCPU負荷が高いため初期化時のみの使用を推奨'
],

'SoftImage-GetPixel': [
  'uint32_t GetPixel(int x, int y) const',
  '指定座標のピクセルカラーを0xAARRGGBB形式で返します。ハイトマップの読み取りやコリジョンマップの判定に使用します。\n範囲外の座標では0が返されます。',
  '// ハイトマップから高さを取得\nGX::SoftImage heightmap;\nheightmap.LoadFromFile(L\"Assets/terrain_height.png\");\nuint32_t pixel = heightmap.GetPixel(x, y);\nuint8_t height = (pixel >> 16) & 0xFF; // R成分',
  '* 0xAARRGGBB形式の返り値からビットシフトで各成分を取り出す\n* 範囲外では0x00000000が返される\n* 頻繁なGetPixelアクセスにはGetPixels()で生データを直接参照する方が高速'
],

'SoftImage-GetPixels': [
  'const uint8_t* GetPixels() const',
  'ピクセルデータの生ポインタを返します。RGBA順で1ピクセル4バイト、行方向にwidth個並んでいます。\n一括処理やTextureManager::CreateTextureFromMemoryへの直接渡しに使用します。',
  '// 生データを直接操作\nconst uint8_t* raw = img.GetPixels();\nfor (uint32_t i = 0; i < img.GetWidth() * img.GetHeight() * 4; i += 4) {\n    uint8_t r = raw[i+0];\n    uint8_t g = raw[i+1];\n    uint8_t b = raw[i+2];\n    uint8_t a = raw[i+3];\n}',
  '* RGBA順（R=offset+0, G=offset+1, B=offset+2, A=offset+3）\n* const版なので書き込みはDrawPixelを使用すること\n* TextureManager::CreateTextureFromMemoryに直接渡せる'
],

'SoftImage-GetWidthGetHeight': [
  'uint32_t GetWidth() const / uint32_t GetHeight() const',
  '画像の幅と高さをピクセル単位で返します。Create()またはLoadFromFile()成功後に有効な値が返されます。',
  '// 画像サイズの取得\nuint32_t w = img.GetWidth();\nuint32_t h = img.GetHeight();',
  '* 未初期化状態では0が返される\n* LoadFromFileの場合はファイルの実画像サイズが反映される'
],

'SoftImage-CreateTexture': [
  'int CreateTexture(TextureManager& textureManager)',
  'SoftImageの内容をGPUテクスチャとしてアップロードし、テクスチャハンドルを返します。失敗時は-1が返されます。\nピクセル操作後にGPU描画で使用する場合に呼び出します。',
  '// SoftImageからGPUテクスチャを作成\nGX::SoftImage img;\nimg.Create(64, 64);\n// ... ピクセル操作 ...\nint texHandle = img.CreateTexture(textureManager);\n// 以降SpriteBatchで描画可能\nspriteBatch.DrawGraph(100, 100, texHandle);',
  '* 内部でTextureManager::CreateTextureFromMemoryに委譲する\n* 返されたハンドルはSpriteBatchのDrawGraph等で使用できる\n* SoftImage変更後に再度CreateTextureを呼ぶと別のハンドルが割り当てられる（既存ハンドルは手動解放が必要）'
],

// ============================================================
// SpriteBatch (Graphics/Rendering/SpriteBatch.h)
// ============================================================

'SpriteBatch-Initialize': [
  'bool Initialize(ID3D12Device* device, ID3D12CommandQueue* cmdQueue, uint32_t screenWidth, uint32_t screenHeight)',
  'SpriteBatchを初期化します。内部でDynamicBuffer、IndexBuffer、PSO（全ブレンドモード分）、TextureManager、RootSignatureを作成します。\n画面サイズに基づく正射影行列も設定されます。',
  '// SpriteBatchの初期化\nGX::SpriteBatch spriteBatch;\nspriteBatch.Initialize(device, cmdQueue, 1280, 720);',
  '* k_MaxSprites(=4096)個のスプライトをバッチ処理可能\n* 6種のBlendMode用PSOが全て作成される\n* TextureManagerも内部で初期化されるため別途作成不要'
],

'SpriteBatch-Begin': [
  'void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex)',
  'バッチ描画を開始します。Draw系メソッドの呼び出し前に必ず実行してください。\nDynamicBufferをマッピングし、描画ステートを初期化します。',
  '// フレーム描画のフロー\nspriteBatch.Begin(cmdList, frameIndex);\nspriteBatch.DrawGraph(0, 0, bgHandle);\nspriteBatch.DrawRotaGraph(400, 300, 1.0f, angle, playerHandle);\nspriteBatch.End();',
  '* Begin/Endは必ずペアで呼び出すこと\n* frameIndexはSwapChainのバックバッファインデックスと同期させる\n* Begin中にスクリーンサイズ変更やテクスチャロードをしないこと'
],

'SpriteBatch-End': [
  'void End()',
  'バッチ描画を終了し、蓄積された全スプライトをGPUにフラッシュします。内部でFlush()が呼ばれ、DrawCallが発行されます。\nカスタム射影行列はリセットされます。',
  '// バッチ描画の完了\nspriteBatch.Begin(cmdList, frameIndex);\n// ... 各種Draw呼び出し ...\nspriteBatch.End(); // ここで実際のGPU描画が発行される',
  '* テクスチャやブレンドモードが切り替わるたびに内部でもFlushされる\n* End()後のDraw呼び出しは未定義動作\n* SetProjectionMatrixで設定したカスタム行列はEnd()でリセットされる'
],

'SpriteBatch-Draw': [
  'void DrawGraph(float x, float y, int handle, bool transFlag = true)',
  'テクスチャを原寸サイズで描画します。DXLibのDrawGraph()互換です。\n(x, y)はスクリーン座標の左上位置を指定します。',
  '// 背景画像の描画\nspriteBatch.Begin(cmdList, frameIndex);\nspriteBatch.DrawGraph(0, 0, bgHandle);  // 原寸描画\nspriteBatch.End();',
  '* handleはTextureManager::LoadTexture()で取得したハンドルを使用\n* transFlag=trueでアルファブレンド有効（透過描画）\n* テクスチャサイズが自動で描画サイズに使用される'
],

'SpriteBatch-Draw2': [
  'void DrawExtendGraph(float x1, float y1, float x2, float y2, int handle, bool transFlag = true)',
  'テクスチャを指定矩形に拡大縮小して描画します。DXLibのDrawExtendGraph()互換です。\n(x1,y1)が左上、(x2,y2)が右下の矩形座標です。',
  '// テクスチャを2倍に拡大描画\nspriteBatch.DrawExtendGraph(\n    100, 100, 100 + 128, 100 + 128,\n    handle);',
  '* 元のテクスチャサイズに関係なく指定矩形にフィットする\n* アスペクト比の維持は手動で計算する必要がある\n* 負のサイズを指定すると反転描画になる'
],

'SpriteBatch-DrawRect': [
  'void DrawRectGraph(float x, float y, int srcX, int srcY, int w, int h, int handle, bool transFlag = true)',
  'テクスチャの一部矩形を切り出して描画します。DXLibのDrawRectGraph()互換です。\nスプライトシートの個別フレーム描画に使用します。',
  '// スプライトシートから1コマを描画\nint frameX = (frameIdx % 4) * 64;\nint frameY = (frameIdx / 4) * 64;\nspriteBatch.DrawRectGraph(\n    100, 100, frameX, frameY, 64, 64, sheetHandle);',
  '* (srcX, srcY, w, h)はテクスチャ上のピクセル座標で指定\n* 切り出し範囲がテクスチャサイズを超える場合の動作はクランプされる\n* SpriteSheetクラスを使うとUV計算が自動化される'
],

'SpriteBatch-DrawRotated': [
  'void DrawRotaGraph(float cx, float cy, float extRate, float angle, int handle, bool transFlag = true)',
  'テクスチャを回転・拡大描画します。DXLibのDrawRotaGraph()互換です。\n(cx, cy)は回転中心座標、extRateは拡大率、angleはラジアン単位の回転角です。',
  '// プレイヤーキャラの回転描画\nfloat angle = atan2f(dy, dx);\nspriteBatch.DrawRotaGraph(\n    playerX, playerY, 1.0f, angle, playerHandle);',
  '* 回転中心はテクスチャの中心（幅/2, 高さ/2）が基準\n* extRate=1.0で等倍、2.0で2倍拡大\n* angle=0でテクスチャが正位置（回転なし）'
],

'SpriteBatch-DrawExtend': [
  'void DrawRectExtendGraph(float dstX, float dstY, float dstW, float dstH, int srcX, int srcY, int srcW, int srcH, int handle, bool transFlag = true)',
  '矩形切り出し＋拡大縮小描画を行います。スプライトシートのフレームをリサイズして描画する場合に使用します。\nGUI要素のスケーリング描画にも活用されます。',
  '// スプライトシートの1フレームを拡大描画\nspriteBatch.DrawRectExtendGraph(\n    100, 100, 128, 128,  // 描画先\n    0, 0, 64, 64,         // ソース矩形\n    sheetHandle);',
  '* dstX/Y/W/Hは描画先のスクリーン座標とサイズ\n* srcX/Y/W/Hはテクスチャ上のピクセル座標とサイズ\n* GUI ScalingではFontManagerのグリフ描画に使用される'
],

'SpriteBatch-SetBlendMode': [
  'void SetBlendMode(BlendMode mode)',
  'ブレンドモードを変更します。モード変更時に内部バッチがフラッシュされ、対応するPSOに切り替わります。\n以降のDraw呼び出しは新しいブレンドモードで描画されます。',
  '// 加算ブレンドでエフェクト描画\nspriteBatch.SetBlendMode(GX::BlendMode::Add);\nspriteBatch.DrawGraph(x, y, effectHandle);\nspriteBatch.SetBlendMode(GX::BlendMode::Alpha); // 戻す',
  '* ブレンドモード切替は内部フラッシュが発生するためDrawCallが増える\n* 同じモードのスプライトはなるべく連続して描画するのがパフォーマンス的に有利\n* デフォルトはBlendMode::Alpha'
],

'SpriteBatch-SetColor': [
  'void SetDrawColor(float r, float g, float b, float a = 1.0f)',
  '以降のDraw呼び出しに乗算されるカラーを設定します。テクスチャの色調変更や半透明化に使用します。\n各成分は0.0～1.0の範囲で指定します。',
  '// 半透明描画\nspriteBatch.SetDrawColor(1.0f, 1.0f, 1.0f, 0.5f);\nspriteBatch.DrawGraph(x, y, handle);\n// ダメージ時の赤フラッシュ\nspriteBatch.SetDrawColor(1.0f, 0.3f, 0.3f, 1.0f);\nspriteBatch.DrawGraph(x, y, handle);\nspriteBatch.SetDrawColor(1.0f, 1.0f, 1.0f, 1.0f); // 元に戻す',
  '* テクスチャの各ピクセルにこの色が乗算される（頂点カラー）\n* (1,1,1,1)で元のテクスチャ色がそのまま使われる\n* End()でリセットされないため、描画後に手動で(1,1,1,1)に戻すこと'
],

'SpriteBatch-SetProjectionMatrix': [
  'void SetProjectionMatrix(const DirectX::XMMATRIX& matrix)',
  'カスタムの正射影行列を設定します。Camera2Dやスケーリング対応で射影行列を差し替える際に使用します。\nEnd()呼び出し時にデフォルト行列にリセットされます。',
  '// Camera2Dとの連携\nGX::Camera2D camera;\ncamera.SetPosition(100, 50);\ncamera.SetZoom(2.0f);\nauto vp = camera.GetViewProjectionMatrix(1280, 720);\nspriteBatch.SetProjectionMatrix(vp);',
  '* GUI Scalingでデザイン解像度の行列を設定する用途にも使われる\n* End()で自動リセットされるため毎フレーム設定が必要\n* ResetProjectionMatrix()で手動リセットも可能'
],

// SpriteBatch enum values
'SpriteBatch-Alpha': [
  'BlendMode::Alpha',
  '通常のアルファブレンド（半透明描画）モードです。src * srcAlpha + dst * (1 - srcAlpha) の合成式で描画します。\nSpriteBatchのデフォルトブレンドモードです。',
  '// アルファブレンド（デフォルト）\nspriteBatch.SetBlendMode(GX::BlendMode::Alpha);\nspriteBatch.DrawGraph(x, y, handle);',
  '* ほとんどの2D描画はこのモードで行う\n* 透過PNGの描画に適している'
],

'SpriteBatch-Add': [
  'BlendMode::Add',
  '加算合成モードです。src + dst の合成式で、描画先の色に描画元の色を加算します。\n光源、パーティクル、爆発エフェクトなどの発光表現に最適です。',
  '// パーティクルを加算合成で描画\nspriteBatch.SetBlendMode(GX::BlendMode::Add);\nfor (auto& p : particles) {\n    spriteBatch.DrawRotaGraph(p.x, p.y, p.scale, p.angle, particleHandle);\n}\nspriteBatch.SetBlendMode(GX::BlendMode::Alpha);',
  '* 白に近い色ほど明るくなる（黒は影響なし）\n* 複数回描画すると色がどんどん明るくなる（飽和で白になる）'
],

'SpriteBatch-Subtract': [
  'BlendMode::Sub',
  '減算合成モードです。dst - src の合成式で、描画先の色から描画元の色を減算します。\n影や暗くする演出に使用します。',
  '// 減算ブレンドで影を表現\nspriteBatch.SetBlendMode(GX::BlendMode::Sub);\nspriteBatch.DrawGraph(shadowX, shadowY, shadowHandle);\nspriteBatch.SetBlendMode(GX::BlendMode::Alpha);',
  '* 白を描画すると描画先が黒くなる\n* 使用頻度は低めだがシルエット表現等に有用'
],

'SpriteBatch-Multiply': [
  'BlendMode::Mul',
  '乗算合成モードです。src * dst の合成式で、色を掛け合わせます。\n影や色調補正、グラデーションマップの適用に使用します。',
  '// 乗算で画面全体を暗くする\nspriteBatch.SetBlendMode(GX::BlendMode::Mul);\nspriteBatch.DrawGraph(0, 0, darkOverlayHandle);\nspriteBatch.SetBlendMode(GX::BlendMode::Alpha);',
  '* 白(1,1,1)を乗算しても変化なし、黒(0,0,0)で完全に黒くなる\n* Photoshopの乗算レイヤーと同じ効果'
],

'SpriteBatch-Screen': [
  'BlendMode::Screen',
  'スクリーン合成モードです。1 - (1 - src) * (1 - dst) の合成式で、画面を明るくする合成を行います。\n加算より自然な明るさの表現が可能です。',
  '// スクリーン合成でソフトな光表現\nspriteBatch.SetBlendMode(GX::BlendMode::Screen);\nspriteBatch.DrawGraph(x, y, lightHandle);\nspriteBatch.SetBlendMode(GX::BlendMode::Alpha);',
  '* 加算合成と異なり白で飽和しにくい自然な明るさ\n* Photoshopのスクリーンレイヤーと同じ効果'
],

'SpriteBatch-None': [
  'BlendMode::None',
  'ブレンドなし（不透明描画）モードです。描画先の色を完全に上書きします。\nアルファテスト不要な完全不透明テクスチャの描画に使用します。',
  '// 不透明な背景描画\nspriteBatch.SetBlendMode(GX::BlendMode::None);\nspriteBatch.DrawGraph(0, 0, bgHandle);\nspriteBatch.SetBlendMode(GX::BlendMode::Alpha);',
  '* ブレンド計算をスキップするため最も高速\n* アルファチャンネルは完全に無視される'
],

// ============================================================
// PrimitiveBatch (Graphics/Rendering/PrimitiveBatch.h)
// ============================================================

'PrimitiveBatch-Initialize': [
  'bool Initialize(ID3D12Device* device, uint32_t screenWidth, uint32_t screenHeight)',
  'PrimitiveBatchを初期化します。三角形用・線分用のDynamicBuffer、定数バッファ、2つのPSO（三角形/線分）を作成します。\n画面サイズに基づく正射影行列が設定されます。',
  '// PrimitiveBatchの初期化\nGX::PrimitiveBatch primBatch;\nprimBatch.Initialize(device, 1280, 720);',
  '* 三角形最大4096*3頂点、線分最大4096*2頂点をバッチ処理可能\n* テクスチャは使用しない単色描画専用\n* SpriteBatchとは別のPSOを使うため混在描画時はEnd/Beginが必要'
],

'PrimitiveBatch-Begin': [
  'void Begin(ID3D12GraphicsCommandList* cmdList, uint32_t frameIndex)',
  'プリミティブバッチ描画を開始します。Draw系メソッドの呼び出し前に実行してください。\nDynamicBufferをマッピングし描画ステートを初期化します。',
  '// プリミティブ描画のフロー\nprimBatch.Begin(cmdList, frameIndex);\nprimBatch.DrawBox(10, 10, 200, 150, 0xFFFF0000, true);\nprimBatch.DrawCircle(300, 200, 50, 0xFF00FF00, true);\nprimBatch.End();',
  '* Begin/Endは必ずペアで呼び出すこと\n* SpriteBatchのBegin/End内で呼ぶとPSOが競合するため、必ず別のBegin/Endブロックで使用する'
],

'PrimitiveBatch-End': [
  'void End()',
  'プリミティブバッチ描画を終了し、蓄積された全プリミティブをGPUにフラッシュします。\n三角形と線分を別々にフラッシュし、それぞれDrawCallを発行します。',
  '// バッチ描画の完了\nprimBatch.Begin(cmdList, frameIndex);\n// ... 各種Draw呼び出し ...\nprimBatch.End();',
  '* 三角形プリミティブと線分プリミティブで別々のDrawCallが発行される\n* End()後のDraw呼び出しは未定義動作'
],

'PrimitiveBatch-DrawLine': [
  'void DrawLine(float x1, float y1, float x2, float y2, uint32_t color, int thickness = 1)',
  '2点間の線分を描画します。DXLibのDrawLine()互換です。\ncolor形式は0xAARRGGBBです。thicknessで線の太さを指定できます。',
  '// 線分の描画\nprimBatch.DrawLine(100, 100, 400, 300, 0xFFFFFFFF);\n// 太い赤線\nprimBatch.DrawLine(0, 0, 640, 480, 0xFFFF0000, 3);',
  '* thickness > 1の場合、内部で矩形（三角形2枚）として描画される\n* thickness=1はD3D12のLineList primitiveを使用\n* 色の形式は0xAARRGGBB（A=255で不透明）'
],

'PrimitiveBatch-DrawRect': [
  'void DrawBox(float x1, float y1, float x2, float y2, uint32_t color, bool fill)',
  '矩形を描画します。DXLibのDrawBox()互換です。\nfill=trueで塗りつぶし、falseで枠線のみです。',
  '// 塗りつぶし矩形\nprimBatch.DrawBox(50, 50, 200, 150, 0xFF0000FF, true);\n// 枠線のみ\nprimBatch.DrawBox(50, 50, 200, 150, 0xFFFFFFFF, false);',
  '* fill=trueは三角形2枚、fill=falseは4本の線分で描画される\n* (x1,y1)が左上、(x2,y2)が右下\n* HPバーやデバッグ表示によく使用される'
],

'PrimitiveBatch-DrawCircle': [
  'void DrawCircle(float cx, float cy, float r, uint32_t color, bool fill, int segments = 32)',
  '円を描画します。DXLibのDrawCircle()互換です。\nsegmentsで円の滑らかさ（多角形の辺数）を指定します。',
  '// 塗りつぶし円\nprimBatch.DrawCircle(300, 200, 50, 0xFF00FF00, true);\n// 高精度な円（64分割）\nprimBatch.DrawCircle(300, 200, 100, 0xFFFFFF00, false, 64);',
  '* segments数が大きいほど滑らかだが頂点数が増える（デフォルト32で十分な円に見える）\n* fill=trueは三角形ファンで描画される\n* 非常に大きい半径では分割数を増やすと良い'
],

'PrimitiveBatch-DrawTriangle': [
  'void DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, uint32_t color, bool fill)',
  '三角形を描画します。DXLibのDrawTriangle()互換です。\n3頂点のスクリーン座標を指定します。',
  '// 塗りつぶし三角形\nprimBatch.DrawTriangle(\n    300, 100,  // 頂点1\n    200, 300,  // 頂点2\n    400, 300,  // 頂点3\n    0xFFFF0000, true);',
  '* fill=trueは1つの三角形、fill=falseは3本の線分で描画される\n* 頂点の巻き方向（CW/CCW）は描画に影響しない（カリングなし）'
],

'PrimitiveBatch-DrawOval': [
  'void DrawOval(float cx, float cy, float rx, float ry, uint32_t color, bool fill, int segments = 32)',
  '楕円を描画します。DXLibのDrawOval()互換です。\nrxが水平方向の半径、ryが垂直方向の半径です。',
  '// 楕円の描画\nprimBatch.DrawOval(400, 300, 100, 50, 0xFF00FFFF, true);\n// 影の表現（潰れた楕円）\nprimBatch.DrawOval(playerX, playerY + 50, 30, 10, 0x80000000, true);',
  '* rx == ryの場合はDrawCircleと同じ結果になる\n* 影やUI装飾によく使用される\n* segments数はDrawCircleと同じ意味'
],

'PrimitiveBatch-DrawPixel': [
  'void DrawPixel(float x, float y, uint32_t color)',
  '1ピクセルを描画します。DXLibのDrawPixel()互換です。\n内部では極小の矩形（三角形2枚）として描画されます。',
  '// パーティクルの点描画\nfor (auto& p : particles) {\n    primBatch.DrawPixel(p.x, p.y, p.color);\n}',
  '* 実際には1x1ピクセルの矩形として描画される\n* 大量の点描画にはSpriteBatchの小さなテクスチャの方が効率的\n* デバッグ用途やシンプルなエフェクトに適している'
],

'PrimitiveBatch-DrawSolidRect': [
  'void DrawSolidRect(float x, float y, float w, float h, uint32_t color)',
  '塗りつぶし矩形を描画します。DrawBox(x, y, x+w, y+h, color, true) の短縮版です。\n位置とサイズで指定するためUI描画で使いやすい形式です。',
  '// HPバーの描画\nfloat hpRatio = currentHP / maxHP;\nprimBatch.DrawSolidRect(10, 10, 200 * hpRatio, 20, 0xFF00FF00);\nprimBatch.DrawBox(10, 10, 210, 30, 0xFFFFFFFF, false);',
  '* DrawBox(fill=true)のラッパーで、(x,y,w,h)形式の引数を取る\n* UIレイアウトで座標+サイズ形式が使いやすい場合に推奨'
],

// ============================================================
// Camera2D (Graphics/Rendering/Camera2D.h)
// ============================================================

'Camera2D-SetPosition': [
  'void SetPosition(float x, float y)',
  'カメラのワールド座標位置を設定します。この値がビュー行列の平行移動成分に反映され、画面がスクロールします。\nプレイヤー追従やマップスクロールに使用します。',
  '// プレイヤー追従カメラ\ncamera.SetPosition(playerX - screenW / 2, playerY - screenH / 2);',
  '* (0,0)がデフォルト位置（画面左上がワールド原点）\n* 正のXで画面が左にスクロール、正のYで画面が上にスクロール\n* スムーズ追従にはLerpを使って徐々に目標位置に近づけると良い'
],

'Camera2D-SetZoom': [
  'void SetZoom(float zoom)',
  'ズーム倍率を設定します。1.0で等倍、2.0で2倍拡大、0.5で半分に縮小されます。\nビュー行列のスケール成分に反映されます。',
  '// マウスホイールでズーム\nfloat zoomLevel = 1.0f;\nzoomLevel += mouse.GetWheelDelta() * 0.1f;\nzoomLevel = std::clamp(zoomLevel, 0.1f, 5.0f);\ncamera.SetZoom(zoomLevel);',
  '* 0以下の値は設定しないこと（行列が不正になる）\n* ズーム中心はカメラ位置（SetPosition）の点になる'
],

'Camera2D-SetRotation': [
  'void SetRotation(float radians)',
  'カメラの回転角をラジアン単位で設定します。画面全体がこの角度で回転します。\n演出やエフェクトで画面を揺らす場合に使用します。',
  '// 画面シェイク演出\nfloat shake = sinf(timer * 30.0f) * 0.05f * intensity;\ncamera.SetRotation(shake);',
  '* 正の値で反時計回り\n* 2Dゲームでは通常0（回転なし）のまま使用\n* 画面シェイクではSetPositionとSetRotationの組み合わせが効果的'
],

'Camera2D-GetViewMatrix': [
  'XMMATRIX GetViewProjectionMatrix(uint32_t screenWidth, uint32_t screenHeight) const',
  'カメラの位置・ズーム・回転を反映したビュープロジェクション行列を計算して返します。\nSpriteBatch::SetProjectionMatrixに渡して2Dカメラを適用します。',
  '// Camera2Dの適用\nauto vp = camera.GetViewProjectionMatrix(1280, 720);\nspriteBatch.SetProjectionMatrix(vp);\nprimBatch.SetProjectionMatrix(vp);',
  '* 内部で平行移動→回転→スケール→正射影の順で行列を合成する\n* screenWidth/Heightは正射影行列の計算に必要\n* SpriteBatchとPrimitiveBatch両方に設定すること'
],

'Camera2D-GetProjectionMatrix': [
  'XMMATRIX GetProjectionMatrix(float w, float h) const',
  '正射影行列を取得します。カメラのズームが反映された射影行列です。\nビュー行列と分離して使用する場合のヘルパーです。',
  '// 射影行列のみ取得\nauto proj = camera.GetProjectionMatrix(1280, 720);',
  '* GetViewProjectionMatrixの内部でも使用されている\n* カスタムビュー行列と組み合わせる場合に使用する'
],

'Camera2D-ScreenToWorld': [
  'XMFLOAT2 ScreenToWorld(float sx, float sy, float sw, float sh) const',
  'スクリーン座標をワールド座標に変換します。マウスクリック位置をゲーム内座標に変換する際に使用します。\nカメラの位置・ズーム・回転を考慮した逆変換を行います。',
  '// マウス位置をワールド座標に変換\nauto worldPos = camera.ScreenToWorld(\n    mouseX, mouseY, 1280, 720);\n// worldPos.x, worldPos.y がゲーム内座標',
  '* sw, shはスクリーンサイズを指定する\n* ズームや回転が適用されている場合でも正確に逆変換される\n* UI要素のクリック判定にはスクリーン座標のまま使用すること'
],

'Camera2D-WorldToScreen': [
  'XMFLOAT2 WorldToScreen(float wx, float wy, float sw, float sh) const',
  'ワールド座標をスクリーン座標に変換します。ゲーム内オブジェクトの画面上の位置を取得する際に使用します。\n名前表示やHP表示の座標計算に便利です。',
  '// 敵のHP表示位置を計算\nauto screenPos = camera.WorldToScreen(\n    enemy.x, enemy.y - 30, 1280, 720);\ntextRenderer.DrawString(fontHandle,\n    screenPos.x, screenPos.y, L\"HP: 100\");',
  '* ScreenToWorldの逆変換\n* 画面外の座標も計算される（クリッピングは別途行う必要がある）'
],

// ============================================================
// FontManager (Graphics/Rendering/FontManager.h)
// ============================================================

'FontManager-Initialize': [
  'bool Initialize(ID3D12Device* device, TextureManager* textureManager)',
  'FontManagerを初期化します。DirectWriteファクトリ、D2Dファクトリ、WICファクトリを作成します。\nフォント作成前に必ず呼び出してください。',
  '// FontManagerの初期化\nGX::FontManager fontMgr;\nfontMgr.Initialize(device, &textureManager);',
  '* DirectWrite/D2D1/WICの各COMファクトリが作成される\n* textureManagerはアトラステクスチャのGPUアップロードに使用される\n* 初期化後はCreateFontでフォントを作成する'
],

'FontManager-CreateFont': [
  'int CreateFont(const std::wstring& fontName, int fontSize, bool bold = false, bool italic = false)',
  'フォントを作成してハンドルを返します。指定フォント名・サイズでDirectWriteのTextFormatを作成し、ASCII+日本語基本文字をプリラスタライズします。\n失敗時は-1が返されます。',
  '// フォントの作成\nint font16 = fontMgr.CreateFont(L\"MS Gothic\", 16);\nint fontBold = fontMgr.CreateFont(L\"Yu Gothic UI\", 24, true);\nint fontItalic = fontMgr.CreateFont(L\"Arial\", 20, false, true);',
  '* ASCII(32-126) + ひらがな + カタカナ + CJK句読点 + 全角記号（約430文字）がプリラスタライズされる\n* アトラスサイズは2048x2048ピクセル\n* 漢字は初回使用時にオンデマンドでラスタライズ（GetGlyphInfo内で自動実行）\n* k_MaxFonts(=64)個まで同時管理可能'
],

'FontManager-DestroyFont': [
  'void Shutdown()',
  '全フォントリソースを解放し、FontManagerを終了状態にします。アプリケーション終了時に呼び出してください。\n個別のフォント解放は不要で、全フォントが一括解放されます。',
  '// 終了処理\nfontMgr.Shutdown();',
  '* 全アトラステクスチャとDirectWriteリソースが解放される\n* Shutdown後のCreateFontやGetGlyphInfo呼び出しは未定義動作\n* Application::Shutdown()内で呼び出すのが一般的'
],

'FontManager-GetGlyphInfo': [
  'const GlyphInfo* GetGlyphInfo(int fontHandle, wchar_t ch)',
  '指定文字のグリフ情報（アトラス上のUV座標、サイズ、オフセット等）を取得します。未ラスタライズの文字は自動的にラスタライズされアトラスに追加されます。\n失敗時はnullptrが返されます。',
  '// グリフ情報の取得\nconst GX::GlyphInfo* glyph = fontMgr.GetGlyphInfo(fontHandle, L\'A\');\nif (glyph) {\n    float glyphW = (float)glyph->width;\n    float advance = glyph->advance;\n}',
  '* 漢字の初回呼び出し時はCPU側でラスタライズが実行される（atlasDirtyフラグが立つ）\n* GPUへの反映はFlushAtlasUpdates()呼び出し時に行われる\n* TextRendererが内部で使用するため、通常は直接呼ぶ必要はない'
],

'FontManager-GetAtlasTextureHandle': [
  'int GetAtlasTextureHandle(int fontHandle) const',
  'フォントのアトラステクスチャハンドルを返します。SpriteBatch::DrawRectGraphでグリフを描画する際のテクスチャハンドルとして使用します。\n失敗時は-1が返されます。',
  '// アトラステクスチャの取得\nint atlasHandle = fontMgr.GetAtlasTextureHandle(fontHandle);\n// SpriteBatchでグリフを描画\nspriteBatch.DrawRectGraph(\n    x, y, glyph->u0 * 2048, glyph->v0 * 2048,\n    glyph->width, glyph->height, atlasHandle);',
  '* 1フォントにつき1枚のアトラステクスチャ（2048x2048）を使用\n* TextRendererが内部で使用するため、通常は直接呼ぶ必要はない\n* 同一フォントの全グリフが1枚のテクスチャにまとまるため描画効率が良い'
],

'FontManager-GetLineHeight': [
  'int GetLineHeight(int fontHandle) const',
  'フォントの行の高さ（ピクセル）を返します。テキストの折り返しや行間計算に使用します。\nDirectWriteのフォントメトリクスから算出された値です。',
  '// 複数行テキストの描画位置計算\nint lineH = fontMgr.GetLineHeight(fontHandle);\nfor (int i = 0; i < lines.size(); i++) {\n    textRenderer.DrawString(fontHandle,\n        x, y + i * lineH, lines[i], 0xFFFFFFFF);\n}',
  '* フォントサイズに比例した値が返される\n* 行間を調整したい場合はこの値に係数を掛ける\n* 無効なハンドルでは0が返される'
],

'FontManager-FlushAtlasUpdates': [
  'void FlushAtlasUpdates()',
  'CPU側で更新されたアトラスデータをGPUにアップロードします。GetGlyphInfoで新しい漢字がラスタライズされた場合にdirtyフラグが立ち、このメソッドでGPUに反映されます。\nフレーム境界（BeginFrame後、描画前）で毎フレーム呼び出してください。',
  '// フレーム開始時にアトラス更新\napp.BeginFrame();\nfontMgr.FlushAtlasUpdates(); // dirtyなアトラスをGPUにアップロード\n// ... 描画処理 ...',
  '* dirtyフラグが立っていない場合は何もしない（軽量）\n* 内部でTextureManager::UpdateTextureFromMemoryを使用\n* フレーム描画中に呼ぶとGPU同期コストが発生するためフレーム境界で呼ぶこと\n* 日本語テキストでは初回表示時に漢字の遅延ロードが頻繁に発生する'
],

'FontManager-Shutdown': [
  'void Shutdown()',
  '全フォントリソースを解放しFontManagerを終了します。COMオブジェクト、アトラスバッファ、テクスチャハンドルが全て解放されます。\nアプリケーション終了時に一度だけ呼び出します。',
  '// アプリケーション終了時\nfontMgr.Shutdown();\n// fontMgrはもう使用しない',
  '* 全FontEntry（TextFormat、グリフマップ、アトラスピクセル）が解放される\n* テクスチャハンドルはTextureManagerから自動解放される'
],

// FontManager struct: GlyphInfo
'FontManager-GlyphInfo': [
  'struct GlyphInfo { float u0, v0, u1, v1; int width, height, offsetX, offsetY; float advance; }',
  '1文字分のグリフ描画情報を保持する構造体です。アトラステクスチャ上のUV座標、グリフのピクセルサイズ、ベースラインからのオフセット、次の文字への送り幅を含みます。\nGetGlyphInfo()で取得し、SpriteBatchでの文字描画に使用します。',
  '// GlyphInfoを使った手動テキスト描画\nfloat cursorX = startX;\nfor (wchar_t ch : text) {\n    const GX::GlyphInfo* g = fontMgr.GetGlyphInfo(fontHandle, ch);\n    if (!g) continue;\n    spriteBatch.DrawRectGraph(\n        cursorX + g->offsetX, startY + g->offsetY,\n        (int)(g->u0 * 2048), (int)(g->v0 * 2048),\n        g->width, g->height, atlasHandle);\n    cursorX += g->advance;\n}',
  '* u0/v0/u1/v1は0.0～1.0の正規化UV座標\n* advanceはカーニングを含まない基本送り幅\n* offsetX/Yはベースラインからのオフセット（文字配置の微調整に使用）\n* TextRendererがこの構造体を内部で使用して文字列描画を行う'
],

// ============================================================
// TextRenderer (Graphics/Rendering/TextRenderer.h)
// ============================================================

'TextRenderer-Initialize': [
  'void Initialize(SpriteBatch* spriteBatch, FontManager* fontManager)',
  'TextRendererを初期化します。内部でSpriteBatchとFontManagerへのポインタを保持します。\nSpriteBatchとFontManagerの初期化完了後に呼び出してください。',
  '// TextRendererの初期化\nGX::TextRenderer textRenderer;\ntextRenderer.Initialize(&spriteBatch, &fontManager);',
  '* SpriteBatchのBegin/End内でDrawStringを呼ぶ形で使用する\n* FontManagerは文字のグリフ情報取得に使用される\n* ポインタを保持するため、SpriteBatch/FontManagerの寿命に注意'
],

'TextRenderer-DrawString': [
  'void DrawString(int fontHandle, float x, float y, const std::wstring& text, uint32_t color = 0xFFFFFFFF)',
  '文字列を描画します。SpriteBatchのBegin/End内で呼び出してください。\n各文字をグリフ単位でSpriteBatch::DrawRectGraphに渡して描画します。DXLibのDrawString互換です。',
  '// テキスト描画\nspriteBatch.Begin(cmdList, frameIndex);\ntextRenderer.DrawString(fontHandle,\n    100, 50, L\"Hello, World!\", 0xFFFFFFFF);\ntextRenderer.DrawString(fontHandle,\n    100, 80, L\"HP: 100\", 0xFF00FF00);\nspriteBatch.End();',
  '* SpriteBatchのBegin/End内で呼ぶこと（内部でDrawRectGraphを使用するため）\n* color形式は0xAARRGGBB\n* 改行文字(\\n)には対応していない（手動で行送りする必要がある）\n* 未ラスタライズの漢字は自動的にGetGlyphInfoで遅延ロードされる'
],

'TextRenderer-GetStringWidth': [
  'int GetStringWidth(int fontHandle, const std::wstring& text)',
  '文字列の描画幅をピクセル単位で返します。テキストの中央揃えや右揃えの計算に使用します。\n各文字のadvance値を合算して幅を計算します。',
  '// テキストの中央揃え\nint textW = textRenderer.GetStringWidth(fontHandle, L\"Game Over\");\nfloat x = (screenWidth - textW) / 2.0f;\ntextRenderer.DrawString(fontHandle, x, 300, L\"Game Over\");',
  '* 実際のレンダリングは行わず計算のみ（軽量）\n* 最後の文字のadvanceまで含む（文字間スペースを含む）\n* 日本語文字の場合は全角幅で計算される'
],

'TextRenderer-GetLineHeight': [
  'float GetLineHeight(int fontHandle) const',
  'フォントの行の高さを返します。FontManager::GetLineHeight()に委譲します。\n複数行テキストの行間計算に使用します。',
  '// 行間を考慮したテキスト描画\nfloat lineH = textRenderer.GetLineHeight(fontHandle);\nfor (int i = 0; i < lines.size(); i++) {\n    textRenderer.DrawString(fontHandle,\n        x, y + i * lineH, lines[i]);\n}',
  '* FontManager::GetLineHeight()のラッパー\n* 行間を広げたい場合は lineH * 1.5f のように係数を掛ける'
],

// ============================================================
// SpriteSheet (Graphics/Rendering/SpriteSheet.h)
// ============================================================

'SpriteSheet-Load': [
  'static bool LoadDivGraph(TextureManager& textureManager, const std::wstring& filePath, int allNum, int xNum, int yNum, int xSize, int ySize, int* handleArray)',
  '画像ファイルを均等分割して複数のテクスチャハンドルを作成します。DXLibのLoadDivGraph()互換です。\n内部でTextureManager::CreateRegionHandlesを使用してUV矩形ハンドルを生成します。',
  '// 4x3のスプライトシートを読み込み\nint handles[12];\nGX::SpriteSheet::LoadDivGraph(\n    textureManager,\n    L\"Assets/player_walk.png\",\n    12, 4, 3,   // 12分割 (4列 x 3行)\n    64, 64,     // 各コマ 64x64\n    handles);\n// handles[0]～handles[11] で各コマを描画可能',
  '* handleArrayはallNum個以上の要素を持つ配列を渡すこと\n* xNum * yNum >= allNum であること\n* 各ハンドルはTextureManager::GetRegionでUV情報を持つリージョンハンドル\n* 返されたハンドルはSpriteBatch::DrawGraphで直接使用可能'
],

'SpriteSheet-Draw': [
  'void Draw(SpriteBatch& spriteBatch, int frame, float x, float y)',
  '指定フレーム番号のスプライトを描画します。SpriteBatchのBegin/End内で呼び出してください。\nframeは0始まりのインデックスです。',
  '// アニメーションフレームの描画\nint frame = (int)(timer / 0.1f) % 12;\nspriteSheet.Draw(spriteBatch, frame, playerX, playerY);',
  '* frameはLoadで作成したハンドル配列のインデックスに対応\n* 範囲外のframeを指定すると未定義動作\n* 内部でSpriteBatch::DrawRectGraphを使用'
],

'SpriteSheet-Draw2': [
  'void Draw(SpriteBatch& spriteBatch, int frame, float x, float y, float w, float h)',
  '指定フレーム番号のスプライトをサイズ指定で描画します。フレームの拡大縮小描画に使用します。',
  '// 2倍サイズで描画\nspriteSheet.Draw(spriteBatch, frame,\n    playerX, playerY, 128, 128);',
  '* 元のフレームサイズに関係なく指定サイズに拡大/縮小される\n* アスペクト比の維持は呼び出し側で管理する'
],

'SpriteSheet-GetFrameCount': [
  'int GetFrameCount() const',
  '分割テクスチャの総フレーム数を返します。Loadで指定したallNumと同じ値です。\nアニメーションのフレーム上限チェックに使用します。',
  '// フレーム数の確認\nint count = spriteSheet.GetFrameCount();\nint frame = currentFrame % count; // ループ',
  '* Load前は0が返される\n* アニメーションのフレーム範囲チェックに使用する'
],

'SpriteSheet-GetTextureHandle': [
  'int GetTextureHandle() const',
  '元テクスチャのハンドルを返します。分割前の完全な画像テクスチャのハンドルです。\nテクスチャ全体を描画したい場合やデバッグに使用します。',
  '// 元テクスチャ全体を表示（デバッグ用）\nint fullHandle = spriteSheet.GetTextureHandle();\nspriteBatch.DrawGraph(0, 0, fullHandle);',
  '* 分割フレームではなく元画像全体のハンドル\n* 各フレームのハンドルはLoad時のhandleArrayに格納される'
],

'SpriteSheet-GetFrameWidthGetFrameHeight': [
  'int GetFrameWidth() const / int GetFrameHeight() const',
  '1フレームの幅と高さをピクセル単位で返します。Loadで指定したxSize/ySizeと同じ値です。\n描画サイズの計算やコリジョン矩形の算出に使用します。',
  '// コリジョン矩形の計算\nfloat fw = (float)spriteSheet.GetFrameWidth();\nfloat fh = (float)spriteSheet.GetFrameHeight();\n// プレイヤーの当たり判定矩形\nfloat left = playerX, top = playerY;\nfloat right = playerX + fw, bottom = playerY + fh;',
  '* Load前は0が返される\n* 全フレームが同一サイズ（均等分割）であることが前提'
],

// ============================================================
// Animation2D (Graphics/Rendering/Animation2D.h)
// ============================================================

'Animation2D-SetSpriteSheet': [
  'void AddFrames(const int* handles, int count, float frameDuration)',
  'アニメーションフレームを追加します。LoadDivGraphで取得したハンドル配列とフレーム数、1フレームあたりの表示時間（秒）を指定します。\nこのメソッドで登録したフレームがUpdate()で順番に再生されます。',
  '// アニメーションフレームの登録\nint handles[8];\nGX::SpriteSheet::LoadDivGraph(texMgr, L\"walk.png\",\n    8, 4, 2, 64, 64, handles);\nGX::Animation2D anim;\nanim.AddFrames(handles, 8, 0.1f); // 8フレーム、各0.1秒',
  '* handlesの内容はコピーされるため呼び出し後に配列を解放可能\n* frameDurationはフレームレートの逆数（60FPSなら約0.0167秒）\n* 複数回AddFramesを呼ぶとフレームが追加される'
],

'Animation2D-AddAnimation': [
  'void SetLoop(bool loop)',
  'アニメーションのループ設定を変更します。loop=trueで最終フレーム後に先頭に戻り、falseで最終フレームで停止します。\nデフォルトはtrue（ループ再生）です。',
  '// ワンショットアニメーション（爆発等）\nGX::Animation2D explosion;\nexplosion.AddFrames(explHandles, 12, 0.05f);\nexplosion.SetLoop(false); // ループしない\n// IsFinished()で終了検知',
  '* ループ無効時はIsFinished()でアニメーション終了を検知できる\n* 死亡アニメーションや爆発エフェクトではfalseに設定する'
],

'Animation2D-Play': [
  'void SetSpeed(float speed)',
  'アニメーションの再生速度倍率を設定します。デフォルトは1.0です。\n2.0で2倍速、0.5で半分の速度で再生されます。',
  '// 走り状態では2倍速\nif (isRunning) {\n    anim.SetSpeed(2.0f);\n} else {\n    anim.SetSpeed(1.0f);\n}',
  '* 負の値は非対応（逆再生はサポートされない）\n* Update()のdeltaTimeに乗算される形で反映される\n* 0を設定するとアニメーションが停止する'
],

'Animation2D-Stop': [
  'void Reset()',
  'アニメーションを先頭フレームにリセットします。タイマーとフレームカウンタが初期化され、finishedフラグもクリアされます。\nワンショットアニメーションの再使用時に呼び出します。',
  '// アニメーションのリセット\nanim.Reset();\n// 先頭フレームから再生開始',
  '* Reset後のUpdate()で先頭フレームから再生される\n* ループアニメーションでは通常呼ぶ必要はない'
],

'Animation2D-Update': [
  'void Update(float deltaTime)',
  'アニメーションを時間進行させます。deltaTime（経過秒数）に基づいてフレームを進めます。\nゲームループ内で毎フレーム呼び出してください。',
  '// 毎フレームのアニメーション更新\nfloat dt = timer.GetDeltaTime();\nanim.Update(dt);\n// 描画\nint handle = anim.GetCurrentHandle();\nspriteBatch.DrawGraph(x, y, handle);',
  '* deltaTimeはTimer::GetDeltaTime()等から取得する（秒単位）\n* SetSpeed()の倍率が内部でdeltaTimeに乗算される\n* フレームが切り替わるとm_currentFrameが更新される'
],

'Animation2D-Draw': [
  'int GetCurrentHandle() const',
  '現在のフレームに対応するテクスチャハンドルを返します。SpriteBatch::DrawGraphに渡して描画します。\nUpdate()で更新されたフレームのハンドルが返されます。',
  '// 現在フレームの描画\nint handle = anim.GetCurrentHandle();\nspriteBatch.DrawGraph(playerX, playerY, handle);\n// 回転描画も可能\nspriteBatch.DrawRotaGraph(\n    playerX, playerY, 1.0f, angle, handle);',
  '* AddFramesでフレームが未登録の場合は-1が返される\n* 返されたハンドルはSpriteBatchの全Draw系メソッドで使用可能'
],

'Animation2D-GetCurrentFrame': [
  'int GetCurrentFrameIndex() const',
  '現在再生中のフレームインデックス（0始まり）を返します。フレーム番号に応じた処理（足音SE、ヒットフレーム等）に使用します。',
  '// 特定フレームでSE再生\nif (anim.GetCurrentFrameIndex() == 3) {\n    soundPlayer.Play(footstepSE);\n}',
  '* 0からAddFrames()で登録したcount-1までの範囲\n* ループ時は自動的に0に戻る'
],

'Animation2D-IsPlaying': [
  'bool IsFinished() const',
  'アニメーションが終了したかどうかを返します。非ループアニメーションが最終フレームに達した時にtrueになります。\nループアニメーションでは常にfalseです。',
  '// ワンショットアニメーションの終了検知\nif (explosion.IsFinished()) {\n    // 爆発エフェクトを削除\n    RemoveEffect(explosion);\n}',
  '* SetLoop(true)の場合は常にfalseが返される\n* Reset()を呼ぶとfalseに戻る\n* 終了後もGetCurrentHandle()は最終フレームのハンドルを返す'
]

})
