#!/bin/bash

score=0

mkdir myfs >/dev/null 2>&1

./start.sh

test_if_has_mount(){
	mount | grep -q "yfs_client"
	if [ $? -ne 0 ];
	then
			echo "FATAL: Your YFS client has failed to mount its filesystem!"
			exit
	fi;
}
test_if_has_mount

##################################################

# run test 1
./test-lab-2-a.pl yfs1 | grep -q "Passed all"
if [ $? -ne 0 ];
then
        echo "Failed test-A"
        #exit
else

	ps -e | grep -q "yfs_client"
	if [ $? -ne 0 ];
	then
			echo "FATAL: yfs_client DIED!"
			exit
	else
		score=$((score+20))
		#echo $score
		echo "Passed A"
	fi

fi
test_if_has_mount

##################################################

./test-lab-2-b.pl yfs1 | grep -q "Passed all"
if [ $? -ne 0 ];
then
        echo "Failed test-B"
        #exit
else
	
	ps -e | grep -q "yfs_client"
	if [ $? -ne 0 ];
	then
			echo "FATAL: yfs_client DIED!"
			exit
	else
		score=$((score+20))
		#echo $score
		echo "Passed B"
	fi

fi
test_if_has_mount

##################################################

./test-lab-2-c.pl yfs1 | grep -q "Passed all"
if [ $? -ne 0 ];
then
        echo "Failed test-c"
        #exit
else

	ps -e | grep -q "yfs_client"
	if [ $? -ne 0 ];
	then
			echo "FATAL: yfs_client DIED!"
			exit
	else
		score=$((score+20))
		#echo $score
		echo "Passed C"
	fi

fi
test_if_has_mount

##################################################


./test-lab-2-d.sh yfs1 | grep -q "Passed SYMLINK"
if [ $? -ne 0 ];
then
        echo "Failed test-d"
        #exit
else

	ps -e | grep -q "yfs_client"
	if [ $? -ne 0 ];
	then
			echo "FATAL: yfs_client DIED!"
			exit
	else
		score=$((score+20))
		echo "Passed D"
		#echo $score
	fi

fi
test_if_has_mount

##################################################################################

./test-lab-2-e.sh yfs1 | grep -q "Passed BLOB"
if [ $? -ne 0 ];
then
        echo "Failed test-e"
else
        #exit
		ps -e | grep -q "yfs_client"
		if [ $? -ne 0 ];
		then
				echo "FATAL: yfs_client DIED!"
				exit
		else
			score=$((score+20))
			echo "Passed E"
			#echo $score
		fi
fi

test_if_has_mount

##################################################################################
robust(){
./test-lab-2-f.sh yfs1 | grep -q "Passed ROBUSTNESS test"
if [ $? -ne 0 ];
then
        echo "Failed test-f"
else
        #exit
		ps -e | grep -q "yfs_client"
		if [ $? -ne 0 ];
		then
				echo "FATAL: yfs_client DIED!"
				exit
		else
			score=$((score+20))
			echo "Passed F -- Robustness"
			#echo $score
		fi
fi

test_if_has_mount
}

# finally reaches here!
#echo "Passed all tests!"

./stop.sh
echo ""
echo "Total score: "$score
