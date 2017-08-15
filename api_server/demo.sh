#! /bin/bash

inputFile=$1
dataDir="/opt/upload"

if [ "${inputFile}" == "" ]; then
    echo "No input file!"
    exit -1
fi

echo $inputFile

# mv ${dataDir}${inputFile} ${dataDir}/demo.dat
# ./FeatureProject2/bin/feature.bin -conf demo.json

