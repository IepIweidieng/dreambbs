#!/bin/sh
# �Ʀ�]�{������
if [ "${BBSHOME}" = "" ]; then BBSHOME="/home/bbs"; fi
"${BBSHOME}/bin${BBSVER}/topusr" > "${BBSHOME}/gem/@/@pop.new"
mv "${BBSHOME}/gem/@/@pop.new" "${BBSHOME}/gem/@/@pop"
