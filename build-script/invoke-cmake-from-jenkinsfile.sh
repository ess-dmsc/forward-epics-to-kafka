# Factored out from Jenkinsfile because of escape issues

echo 'Epics env vars:'
set | grep -i epics
echo ''

cmake ../code \
-DCMAKE_INCLUDE_PATH=../googletest\;../streaming-data-types\;$DM_ROOT/usr/include\;$DM_ROOT/usr/lib \
-DCMAKE_LIBRARY_PATH=$DM_ROOT/usr/lib \
-Dflatc=$DM_ROOT/usr/bin/flatc \
