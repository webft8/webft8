#!/bin/bash -x
set -e

cd ft8_lib && make 
for (( i=0; i < 10; i++ )) ; do
    VALUE=`python3 -c 'import random; print(random.randint(0, 2361183241434822606847))'`
    VALUE_HEX=`python3 -c 'print(hex('${VALUE}')[2:].zfill(18).upper())'`
    echo "int: $VALUE"
    echo "expected hex: $VALUE_HEX"
    ./gen_ft8 "$VALUE_HEX" /tmp/out.wav 2>/dev/null  && ./decode_ft8 /tmp/out.wav 2>/dev/null | grep "$VALUE_HEX"
    echo "OK!"
done
echo "TEST COMPLETE. LOOKS GOOD."