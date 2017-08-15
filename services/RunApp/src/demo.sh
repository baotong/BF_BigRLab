#! /bin/bash

inputFile=$1
dataDir="/opt/upload"

if [ "${inputFile}" == "" ]; then
    echo "No input file!"
    exit -1
fi

# echo ${inputFile}
killall -9 feature.bin

(cd ${dataDir} && ln -sf ${inputFile} demo.dat)
sleep 1
(cd ./FeatureProject2/bin && ./feature.bin -conf demo.json)
cat ${dataDir}/out.dat

