#!/bin/bash
set -e

SCRIPT_DIR=$(dirname ${BASH_SOURCE[0]})

dataset_path=$1
results_dir=$2

if [[ ! -f $dataset_path ]]; then
    echo "dataset file doesn't exists: $dataset_path"
    exit 1
fi

if [[ ! -d $results_dir ]]; then
    echo "results directory will created at $results_dir"
    mkdir -p $results_dir
else
    read -p "results directory already exists, result files will rewritten, continue (y/n)?" choice
    case "$choice" in
      y|Y ) echo "OK";;
      n|N ) echo "aborting"; exit 1;;
      * ) echo "invalid choice"; exit 1;;
    esac
fi

ids=()
run_num=0
while IFS= read -r line
do
    args=($line)
    range=${args[0]}
    positions=${args[1]}
    iters=${args[2]}

    echo "Running #$run_num for $positions positions and $iters iters"

    MOUNT_PATH=$(realpath "$SCRIPT_DIR/../")
    id=$(docker run --rm -v "$MOUNT_PATH":/build -w /build -d daocasino/daobet-with-cdt:latest bash -c "./cicd/test.sh --range $range --count $iters --columns $positions --out $results_dir/result_$run_num.txt > $results_dir/log_$run_num.txt")
    ids+=($id)

    ((run_num=run_num+1))
done < "$dataset_path"

echo ${ids[@]} > $results_dir/cids
