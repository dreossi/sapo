cmake .

if [ $? -ne 0 ]; then
  exit 1
fi

make test

exit $?
