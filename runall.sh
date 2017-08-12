#!/bin/bash

nemu=build/nemu
cmd="c\nq"

make

echo "compiling testcases..."
if make -C $AM_HOME/tests/cputest ARCH=x86-nemu &> /dev/null; then
  echo "testcases compile OK"
else
  echo "testcases compile error... exit..."
fi

files=`ls $AM_HOME/tests/cputest/build/*-x86-nemu.bin`

for file in $files; do
	base=`basename $file | sed -e 's/-x86-nemu.bin//'`
	printf "[%14s] " $base
	logfile=$base-log.txt
	echo -e $cmd | $nemu $file &> $logfile

	if (grep 'nemu: HIT GOOD TRAP' $logfile > /dev/null) then
		echo -e "\033[1;32mPASS!\033[0m"
		rm $logfile
	else
		echo -e "\033[1;31mFAIL!\033[0m see $logfile for more information"
		if (test -e log.txt) then
			echo -e "\n\n===== the original log.txt =====\n" >> $logfile
			cat log.txt >> $logfile
			rm log.txt
		fi
	fi
done
