元々のソフトの使い方は[こちら](https://github.com/linusmossberg/monte-carlo-ray-tracer/blob/master/README.md)を参照。
以下、今回のソフトにおける変更点を述べる。

- jsonファイルの```cameras```内のパラメータの概要は以下の通り。
    - ```spp1```：近似を行わない前半の、1ピクセルあたりのサンプリング数
    - ```spp2```：近似を行う後半の、1ピクセルあたりのサンプリング数
    - ```edge_threshold```：辺判定で用いる閾値。なお、ピクセルのRGB値は8ビット整数で計算するため、```edge_threshold```の値域は[0,256)である。
    - ```approx_prob```：後半における非エッジピクセルの近似確率。値域は[0,1]である。