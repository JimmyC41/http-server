#!/bin/sh

sudo sysctl -w net.inet.ip.portrange.hifirst=1024
sudo sysctl -w net.inet.ip.portrange.hilast=65535
sudo sysctl -w net.inet.tcp.msl=1000
sudo sysctl -w net.inet.tcp.fastopen=1

ulimit -n 65536

if ! nc -z localhost 8080; then
    echo "Error: Server not running on localhost:8080"
    exit 1
fi

wrk -t10 -c10k -d30s --latency http://127.0.0.1:8080/hello