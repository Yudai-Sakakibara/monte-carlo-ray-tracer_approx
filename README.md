元々のソフトの使い方は[こちら](https://github.com/linusmossberg/monte-carlo-ray-tracer/blob/master/README.md)を参照。
以下、今回のソフトにおける変更点を述べる。

- RISCVコンパイラは[sim-env](https://github.com/shioyadan/sim-env/tree/master)を使って構築する。[sim-env-mtl-approx](http://geopelia.mtl.t.u-tokyo.ac.jp/degawa/sim-env/tree/mtl-approx)はC拡張を使っているが、[Approximate Onikiri](http://geopelia.mtl.t.u-tokyo.ac.jp/degawa/approximate_onikiri/tree/dev_dynamic_adjusting)はそれに対応していない。
- コンパイラはelfの方を使用する。それに伴い、コードには以下の修正を施してある。
    - このコンパイラはthread, filesystemに対応していないので、それらの関連コードは消去・修正してある。
    - コンパイル時にデータ型は確定していなければならないため、templateはヘッダファイルでしか使えない。そのため、それらが存在した```linear-octree.cpp```と```octree.cpp```の中身は、それぞれ```linear-octree.hpp```と```octree.hpp```に統合してある。
- jsonファイルの```cameras```内のパラメータの概要は以下の通り。
    - ```spp1```：近似を行わない前半の、1ピクセルあたりのサンプリング数
    - ```spp2```：近似を行う後半の、1ピクセルあたりのサンプリング数
    - ```edge_threshold```：辺判定で用いる閾値。値域は$[0,256)$である。
    - ```approx_prob```：後半における非エッジピクセルの近似確率。値域は$[0,1]$である。