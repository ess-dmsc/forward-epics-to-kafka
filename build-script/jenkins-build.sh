echo "Build script start"
pwd
mkdir build
mkdir install
cd build
cmake -DCMAKE_INSTALL_PREFIX=`cd ../install; pwd` ../code
