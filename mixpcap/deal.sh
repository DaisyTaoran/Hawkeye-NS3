#!/bin/bash

# 把pcap转换为csv文件。https://www.wireshark.org/docs/dfref/，这里面列出了所有支持的-e字段写法。
echo "Turning .pcap to .csv ..."
tshark -r mypcap-11-2.pcap -T fields -e frame.number -e frame.protocols -e frame.time_delta -e frame.time_relative -e ip.src -e ip.dst -e udp.srcport -e ip.dsfield -e ip.ttl -e udp.dstport -e infiniband.bth.destqp -E header=y -E separator=, > mycsv-11-2.csv

