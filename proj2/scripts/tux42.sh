#!/bin/bash
ifconfig eth0 up
ifconfig eth0 172.16.y1.1/24
route add -net 172.16.y0.0/24 gw 172.16.y1.253
route -n
echo 1 > /proc/sys/net/ipv4/ip_forward
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts

route add -net default gw 172.16.y1.254
