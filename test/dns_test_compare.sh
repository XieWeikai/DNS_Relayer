#!/bin/sh

python3 dns_test.py l >output_127001.txt
sleep 30
python3 dns_test.py 8.8.8.8 >output_8888.txt
