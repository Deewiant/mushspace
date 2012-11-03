#!/bin/bash
if [[ $# -lt 3 ]]; then
   echo >&2 "Usage: $0 <befunge-98 interpreter command> <jobs> <runs>"
   exit 1
fi
bef98=$1
jobs=$2
runs=$3
if [[ ! -x "$bef98" ]]; then
   echo >&2 "'$bef98' is not executable"
   exit 1
fi
if [[ "$jobs" -le 0 ]]; then
   echo >&2 "Job count must be greater than 0, not '$jobs'"
   exit 1
fi
if [[ "$runs" -le 0 ]]; then
   echo >&2 "Run count must be greater than 0, not '$runs'"
   exit 1
fi
resultsdir=results/$(date +%Y%m%d-%H%M%S-$(git rev-parse --short HEAD))
if [[ -e "$resultsdir" ]]; then
   echo >&2 "Results directory '$resultsdir' already exists"
   exit 2
fi
set -e
mkdir -p $(dirname $resultsdir)
mkdir $resultsdir
trap "rmdir $resultsdir 2>/dev/null" EXIT
outputs=$(mktemp)
trap "rm -f $outputs" EXIT
for b98 in befunge98/*.b98; do
   bb98=$(basename "$b98")
   for input in befunge98/inputs/"$bb98"/*; do
      output=$bb98-$(basename "$input")
      echo "$output" >> $outputs
      for (( n=0; n < $runs; ++n )); do
         echo "/usr/bin/time -f'%e %U %S %M' -o $resultsdir/'$output.$n' -- '$bef98' '$b98' <'$input' >/dev/null"
      done
   done
done | parallel --eta -j$jobs
while read -r output; do
   cat $resultsdir/"$output".* > $resultsdir/"$output"
   rm $resultsdir/"$output".*
done < $outputs
