# AZAS (MINI-DOS)について
MINI-DOS for PC-8001/8801/Sharp X1 source and d88 bootable image

MINI-DOSは、ENIX等が発売したいくつかのゲームの開発及びゲームのブートに使用されたDOSです。開発システム全体をAZASと呼びます。ここでは、テスト版を除く最終段階のソースコード及びブート可能なD88フォーマットのイメージを提供します。PC-8801/X1のエミュレータがあれば起動します。

# 実績
- 東京NAMPAストリート 88版/X1版(ENIX)の開発及びブート
- ブレインブレーカー X1版(ENIX)のブート
- HANG-ON 88SR版(パックスエレクトロニカ)の開発及びブート
- 等

※ 全てDISK版。カセット版はDOSを使用していない

※ 実際のゲームのブートには暗号化された特別版MINI-DOSが使用されていることに注意

# Directory Structure
```
├─bootableImages (ブートディスクイメージ)
│  ├─PC-8001_8801 (PC-8001/PC-8801シリーズ用)
│  └─X1 (Sharp X1/X1Turo用)
├─doc (ドキュメントPDF)
├─src (ソースコード)
│  ├─PC-8001_8801 (PC-8001/PC-8801シリーズ用)
│  │  ├─AZA Aki's Zilog mnemonic Asembler (アセンブラAZAのソースコード)
│  │  ├─MINI-DOS Version 0.B (MINI-DOS 80/88のソースコード)
│  │  └─SOCE Screen Oriented Character Editor (エディタSOCEのソースコード)
│  └─X1 (Sharp X1/X1Turo用)
│      └─MINI-DOS Version 0.C (MINI-DOS X1のソースコード)
└─util
    └─ControlZCleaner (任意のファイル群のコントロールZ以降を除去するC#のコマンドラインツールのソース)
```

# 説明

　AZASは、川俣晶が開発したZ80のアセンブラ開発システムです。

　主にPC-8801シリーズのN-BASICモードで使用します。PC-8001およびX1でも動作しますが制限が付きます。

　AZASは主にアセンブラ(AZA)、エディタ(SOCE)、DOS(MINI-DOS)から構成されます。

　AZAはZ80用の2パス絶対アドレス用アセンブラで、16進定数を$記号で記述する可能や、マクロ命令などの独自機能が追加されています。AZA 3.32のソースコードを含む開発ディスクの最終状態を収録しています。

　SOCEはスクリーンエディタです。アセンブラソースの自動フォーマット機能を持ちます。SOCE 1.42のソースコードを含む開発ディスクの最終状態を収録しています。

　MINI-DOSは、NEC Disk BASIC互換のフロッピーディスクを扱うDOSです。mount/removeは不要です。CP/Mのようなリブートも不要です。BASICと同等のスクリーンエディタが使用できます。MINI-DOS 0.B/0.Cのソースコードを含む開発ディスクの最終状態を収録しています。

　使い方の詳細はドキュメントPDFをお読みください。

# PC-8001/8801版の制限事項
PC-8001mkII/mkIISR, PC-8801/mkII/mkIISR以降をサポートします

PC-8001の場合、ROMが1.0の場合はKOM-980(作者が改造してメモリを拡張したPC-8001)と仮定されますが、同等品を持っている人はいないなずなので、動作しないと思った方が良いでしょう。

PC-8001でROMが1.1の場合はMULTICARDが接続されていると仮定して動作します。(メモリ64KBが必要)

拡張メモリの管理はブートローラー(boota.aza)をカスタマイズすることで動作を変更できます。拡張メモリの仕様が異なる環境で使用する場合はここを書き換えると変更できます。MINI-DOSはブートローダーが書き込んだテーブルを元にバンク繰り替えを行います。ここを修正してブートセクタを書き換えればPC-8001+8011/12/13でもメモリが64KBあれば動作可能でしょう。

# MINI-DOSの制限事項
　2D 320KBのディスクのみサポートします。1Dや2HDはサポートしません。


# X1版の制限事項
コマンドライン解析にバグがあるようでコマンドが正しく処理されない場合があります。

外部コマンドは全て用意されていません。動作するものと動作しないものがありますが、動作するものはソースからアセンブルする必要があります。アセンブルはPC-8001/8801版で行います(エミュレータで十分です)。

# アセンブラ(AZA, :as, as.trc) の制限事項

GVRAMを使用してソースをキャッシュする機能は、PC-8801シリーズ上で動作する場合のみ有効です。パス1で読み込んだソースコードは先頭48Kバイト分だけパス2でディスクから読み込みません。他機種では常時ディスクから読み出します。


# エディタ (SOCE, :ed, ed.flc) の制限事項

　書き込み禁止のファイルを開いた場合、これを保存して終了することはできません。書き込み許可に切り換えるためには、:attrコマンドを実行する必要がありますが、これを実行するメモリを空けるためにはエディタを終了させる必要があるためです。

# モニタ (:mon, mon.ovl) の制限事項

　N-BASIC ROMに依存するコードを除去したX1でも動作する版はソースコードからビルドする必要があります。

# 補足説明
　AZAS/MINI-DOSは、本来はPC-8001(初代)のために開発を始めたものですが、メモリ64KBを必要とするので無拡張のPC-8001では動作しません。ですので、PC-8801で動作させることを基本としています。それなら拡張は不要です。また、PC-8801サポートの多くの機能を追加してあります。先行入力の機能はsenko.expを常駐させたときのみ有効であり、PC-8801でのみ使用できます。N-BASICモードで起動していますが先行入力は有効になります。

# ControlZCleanerについて
　NEC 2Dフォーマットのテキストファイルを普通に取り出すと、コントロールZ以降にゴミが付いた状態で取得されます。このゴミを除去するためのツールです。C#で書かれたコンソールアプリです。
