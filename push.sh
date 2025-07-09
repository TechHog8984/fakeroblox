#!/bin/bash

FILE=.test_success

if [ ! -f "$FILE" ]; then
  echo "no $FILE! did you forget to test?"
  exit 1
fi

git push || exit 1

rm $FILE
