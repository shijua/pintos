#!/bin/bash
declare -a tasksuser=(
"args-none"
# "args-none" "args-single" "args-multiple" "args-many" "args-dbl-space"
# "page-linear" "page-parallel" "page-shuffle" "page-merge-seq" "page-merge-par" 
# "pt-bad-addr" "pt-bad-read" "pt-write-code" "pt-write-code2" "pt-grow-bad"
# "sc-bad-sp" "sc-bad-arg" "sc-bad-num" "sc-boundary" "sc-boundary-2"
# "halt" "exit"
# "write-stdin"
# "exec-one" "exec-arg" "exec-large-arg" "exec-multiple" "exec-missing"
# "exec-over-arg" "exec-over-args"
# "exec-bad-ptr"
# "wait-simple" "wait-twice" "wait-killed" "wait-load-kill" "wait-bad-pid" "wait-bad-child"
# "multi-recurse"
# "bad-read" "bad-write" "bad-read2" "bad-write2" "bad-jump" "bad-jump2"
)



declare -a tasksvm=(
"page-linear" "page-parallel" "page-shuffle" "page-merge-seq" "page-merge-par" 
"pt-bad-addr" "pt-bad-read" "pt-write-code" "pt-write-code2" "pt-grow-bad"
"page-merge-mm" "page-merge-stk"
"pt-grow-stack" "pt-grow-stk-sc" "pt-big-stk-obj" "pt-grow-pusha"
"mmap-read" "mmap-write" "mmap-shuffle" "mmap-twice" "mmap-unmap" "mmap-exit" 
"mmap-clean" "mmap-close" "mmap-remove" "mmap-bad-fd" "mmap-inherit" "mmap-null" 
"mmap-zero" "mmap-misalign" "mmap-over-code" "mmap-over-data" "mmap-over-stk" "mmap-overlap"
)


if [[ ! "$PWD" =~ src/vm$ ]]; then
    echo "Error: Directory does not end in src/threads"
    exit 1
fi

# make clean

make

for task in "${tasksuser[@]}";
do
    make build/tests/userprog/$task.result
    # echo $task
done


for task in "${tasksvm[@]}";
do
    make build/tests/vm/$task.result
    # echo $task
done
