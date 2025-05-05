#!/bin/bash

# 把pcap转换为csv文件。https://www.wireshark.org/docs/dfref/，这里面列出了所有支持的-e字段写法。
echo "Turning .pcap to .csv ..."
tshark -r mypcap-12-3.pcap -T fields -e frame.number -e frame.protocols -e frame.time_delta -e frame.time_relative -e frame.len -e ip.src -e ip.dst -e ip.proto -e udp.srcport -e ip.dsfield -e ip.ttl -e udp.dstport -e infiniband.bth.destqp -e mypfc.pausetime -E header=y -E separator=, > mycsv-12-3.csv

#echo "Analysis .pcap ..."
#tshark -r mypcap-9-1.pcap  -qz io,stat,1 | awk 'NR>12  {print $2","$4","$6","$8}' > traffic_stats.csv
# tshark -r mypcap-11-2.pcap -qz io,stat,1 | awk 'NR>7 && NF==4 {print $1","$2","$3}'
