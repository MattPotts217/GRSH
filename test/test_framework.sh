#!/bin/bash

ec="expected"
ac="actual"

if [[ "$1" == "batch" ]]; then
  ec=$(bash $2 | grep -v "grsh> ")
  ac=`./grsh.out $2 | grep -v "grsh> "`
else
  ec=$(bash $2 | grep -v "grsh> ")
  ac=`./grsh.out < $2 | grep -v "grsh> "`
fi

if [[ "$3" == "debug" ]]; then
  echo ${ec}
  echo "------"
  echo ${ac}
fi

if [[ "${ec}" == "${ac}" ]]; then
  echo "Test passes"
else
  echo "$2 fails"
  echo "Expected output: " ${ec}
  echo "Actual output  : " ${ac}
fi

