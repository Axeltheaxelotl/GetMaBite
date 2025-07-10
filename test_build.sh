#!/bin/bash
cd /home/smasse/Projects/P6/GetMaBite
make clean
make 2>&1 | tee build.log
echo "Build exit code: $?"
ls -la webserv 2>/dev/null && echo "SUCCESS: webserv created" || echo "FAILED: webserv not found"
