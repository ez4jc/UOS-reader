#!/bin/bash

set -e

echo "开始打包阅读器..."

cd /home/ZGYD/code/reader

# 清理旧构建
rm -rf build package

# 创建构建目录
mkdir -p build
cd build

# qmake
qmake ../reader.pro

# 编译
make -j$(nproc)

# 创建打包目录
mkdir -p package/opt/reader/bin
mkdir -p package/usr/share/applications

# 复制可执行文件
cp reader package/opt/reader/bin/

# 复制desktop文件
cp ../packaging/reader.desktop package/usr/share/applications/

# 创建DEBIAN目录和control文件
mkdir -p package/DEBIAN

cat > package/DEBIAN/control << 'EOF'
Package: reader
Version: 1.0.0
Section: office
Priority: optional
Architecture: arm64
Depends: libqt5widgets5, libqt5gui5, libqt5core5a, libqt5gui5-gles, libqt5widgets5-gles
Maintainer: User <user@example.com>
Description: TXT文本阅读器
 一个简洁的TXT文本阅读器，支持自动目录识别、透明背景、快捷键隐藏等功能。
EOF

# 设置权限
chmod -R 755 package/opt
chmod -R 755 package/usr

# 创建deb包
cd package
dpkg-deb --build . ../reader_1.0.0_arm64.deb

echo "打包完成: reader_1.0.0_arm64.deb"
echo ""
echo "安装命令: sudo dpkg -i reader_1.0.0_arm64.deb"
echo "卸载命令: sudo dpkg -r reader"
