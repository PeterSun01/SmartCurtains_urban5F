#!/bin/bash

for((i=1;i<=4;i++));
do
{
    echo "download ttyS$i"
    {
        make flash -j8 ESPPORT=/dev/ttyS$i ESPBAUD=921600
    }>/dev/null
}&
done
wait
echo download over