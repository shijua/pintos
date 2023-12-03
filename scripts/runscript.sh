#!/bin/bash
nowrunning="task2-pcb.sh"
homedir=/homes/wh1322/Documents/year2/pintos_7
cd scripts
rm -rf output
mkdir output

cd $homedir

cd src/vm

../../scripts/task2-pcb.sh | tee ../../scripts/output/output.txt

cd $homedir/scripts/output

grep -i "fail" output.txt | tee tests_failed.txt
grep -i "pass" output.txt | tee tests_passed.txt