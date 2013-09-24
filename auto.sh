#!/bin/sh
# Copyright (c) 2012, 2013 Open Cloud Demonstration Experiments Taskforce.
# All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
read DES_GW < ~/destination-gw.txt
echo 'Do: make clean'
make clean
echo 'Do: rsync '`whoami`'@'$DES_GW':veip' 
rsync -a . `whoami`@$DES_GW:veip
echo 'Do: make'
make
echo 'Do: sudo ./veip'
sudo ./veip $DES_GW
