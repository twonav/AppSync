#!/bin/bash

#
# Checksum Generator for an stream of bytes
# Sums all bytes.
# CompeGPS_Team S.L. 2020 All Rights Reserved
#


SUM=0

for var in "$@"
do
    SUM=$((SUM+var))
done

echo "Checksum Calculator(byte sum)"
printf "Result: 0x%02x\n" "$SUM"
