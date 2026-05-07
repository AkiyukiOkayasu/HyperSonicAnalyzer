# HyperSonic Analyzer

超高サンプルレート対応スペクトラムアナライザー VST3/Standalone プラグイン

768kHz などの超高サンプルレートに対応し、アナログオシレーターの高周波成分を可視化するために設計されたスペクトラムアナライザーです。

## 特徴

- **超高サンプルレート対応**: 768kHz までのサンプルレートに対応
- **大規模FFT**: 65536サンプルのFFTサイズで高い周波数分解能を実現
- **Blackman-Harris窓**: 優れたサイドローブ特性を持つ窓関数を使用
- **リアルタイムカーソル表示**: マウス位置の周波数と振幅をリアルタイムで表示

## スクリーンショット

<img width="1112" height="765" alt="Screenshot 2026-05-07 at 17 11 30" src="https://github.com/user-attachments/assets/dbdeb8ac-f895-4517-a140-84f8e43f7d24" />

## 動作環境

- macOS 10.15 以降
- VST3 対応 DAW または スタンドアロン実行

## Build

### 必要なツール

- CMake 3.22 以降
- C++20 対応コンパイラ (Xcode 14 以降推奨)
- clang-format (コードフォーマット用)

### ビルド手順

```bash
# リポジトリをクローン
git clone https://github.com/your-username/HyperSonicAnalyzer.git
cd HyperSonicAnalyzer

# ビルドディレクトリを作成
mkdir build && cd build

# CMake を実行
cmake .. -DCMAKE_BUILD_TYPE=Release

# ビルド
cmake --build . --config Release
```
