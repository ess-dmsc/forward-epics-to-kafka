BDIR=`pwd`
echo "BDIR: $BDIR"
uname -a
ls
ls ..

echo "EPICS_HOST_ARCH: $EPICS_HOST_ARCH"

cd repos
git clone -b release-1.8.0 https://github.com/google/googletest.git
git clone -b master https://github.com/ess-dmsc/streaming-data-types.git
cd "$BDIR"

find . -maxdepth 3


echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo grep env
set | grep -i epics

echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo find /opt/epics
find /opt/epics -maxdepth 1

echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo find DM_ROOT/usr
find $DM_ROOT/usr -maxdepth 2

echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo find /usr/local
find /usr/local -maxdepth 2

echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo find EPICS_BASES_PATH
find $EPICS_BASES_PATH -maxdepth 1

echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo find EPICS_BASE
find $EPICS_BASE -maxdepth 1

echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo /opt/epics/modules/pvDataCPP
find /opt/epics/modules/pvDataCPP -maxdepth 1

echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo /opt/epics/modules/pvAccessCPP
find /opt/epics/modules/pvAccessCPP -maxdepth 1

echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo /opt/epics/modules/pvDatabaseCPP
find /opt/epics/modules/pvDatabaseCPP -maxdepth 1

echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo /opt/epics/modules/normativeTypesCPP
find /opt/epics/modules/normativeTypesCPP -maxdepth 1

echo "-----------------------------------------------------"
echo "-----------------------------------------------------"

mkdir -p build
mkdir -p install
cd build
cmake -DCMAKE_INCLUDE_PATH=../repos/streaming-data-types -DCMAKE_INSTALL_PREFIX=`cd ../install; pwd` ../repos/forward-epics-to-kafka  &&  make VERBOSE=1
