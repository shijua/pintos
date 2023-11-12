nowrunning="task2-pcb.sh"
homedir=/Users/yifan/Desktop/pintos/pintos_7
cd scripts
rm -rf output
mkdir output

cd $homedir

cd src/userprog

../../scripts/task2-pcb.sh | tee ../../scripts/output/output.txt

cd $homedir/scripts/output

grep -i "fail" output.txt | tee tests_failed.txt
grep -i "pass" output.txt | tee tests_passed.txt