declare -a tasks=(
"args-none" "args-single" "args-multiple" "args-many" "args-dbl-space"
"sc-bad-sp" "sc-bad-arg" "sc-bad-num" "sc-boundary" "sc-boundary-2"
"halt" "exit"
"write-stdin"
"exec-one" "exec-arg" "exec-large-arg" "exec-multiple" "exec-missing"
"exec-over-arg" "exec-over-args"
"exec-bad-ptr"
"wait-simple" "wait-twice" "wait-killed" "wait-load-kill" "wait-bad-pid" "wait-bad-child"
"multi-recurse"
"bad-read" "bad-write" "bad-read2" "bad-write2" "bad-jump" "bad-jump2"
)


if [[ ! "$PWD" =~ src/userprog$ ]]; then
    echo "Error: Directory does not end in src/threads"
    exit 1
fi

make clean

make

for task in "${tasks[@]}";
do
    make build/tests/userprog/$task.result
    # echo $task
done
