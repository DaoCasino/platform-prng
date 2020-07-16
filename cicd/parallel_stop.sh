#!/bin/bash
set -e

results_dir=$1

if [[ ! -f $results_dir/cids ]]; then
    echo "containers ids file not found"
    exit 1
fi

echo "Shutting down containers..."
docker stop $(cat $results_dir/cids)

rm $results_dir/cids
