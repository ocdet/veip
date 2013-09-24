#!/bin/sh
read DES_GW < ~/destination-gw.txt
echo 'Do: make clean'
make clean
echo 'Do: rsync '`whoami`'@'$DES_GW':veip' 
rsync -a . `whoami`@$DES_GW:veip
echo 'Do: make'
make
echo 'Do: sudo ./veip'
sudo ./veip $DES_GW
