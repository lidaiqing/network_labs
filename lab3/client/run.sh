#!/bin/sh
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR
./client
echo "/help"
echo "/login ip tcp 128.100.13.232 5000"
