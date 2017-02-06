echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo find /opt/epics
find /opt/epics -maxdepth 3
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
find $EPICS_BASES_PATH -maxdepth 2
echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo find EPICS_BASE
find $EPICS_BASE -maxdepth 2
echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo EPICS_HOST_ARCH
echo $EPICS_HOST_ARCH
echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo /opt/epics/modules/pvDataCPP
find /opt/epics/modules/pvDataCPP -maxdepth 2
echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo /opt/epics/modules/pvAccessCPP
find /opt/epics/modules/pvAccessCPP -maxdepth 2
echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo /opt/epics/modules/pvDatabaseCPP
find /opt/epics/modules/pvDatabaseCPP -maxdepth 2
echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo /opt/epics/modules/normativeTypesCPP
find /opt/epics/modules/normativeTypesCPP -maxdepth 2
echo '-----------------------------------------------------'
echo '-----------------------------------------------------'
echo "Build script start"
pwd
mkdir build
mkdir install
cd build
cmake -DCMAKE_INSTALL_PREFIX=`cd ../install; pwd` -DREQUIRE_GTEST=0 ../repos/forward-epics-to-kafka
