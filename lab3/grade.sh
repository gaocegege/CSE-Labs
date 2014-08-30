#!/bin/bash
./stop.sh >/dev/null 2>&1
./stop.sh >/dev/null 2>&1
./stop.sh >/dev/null 2>&1
./stop.sh >/dev/null 2>&1
./stop.sh >/dev/null 2>&1

score=0

mkdir yfs1 >/dev/null 2>&1
mkdir yfs2 >/dev/null 2>&1

./start.sh

test_if_has_mount(){
	mount | grep -q "yfs_client"
	if [ $? -ne 0 ];
	then
			echo "FATAL: Your YFS client has failed to mount its filesystem!"
			exit
	fi;
	yfs_count=$(ps -e | grep -o "yfs_client" | wc -l)
	extent_count=$(ps -e | grep -o "extent_server" | wc -l)
	
	if [ $yfs_count -ne 2 ];
	then
			echo "error: yfs_client not found (expecting 2)"
			exit
	fi;

	if [ $extent_count -ne 1 ];
	then
			echo "error: extent_server not found"
			exit
	fi;
}
test_if_has_mount

##################################################

# run test 1
./test-lab-3-a.pl yfs1 | grep -q "Passed all"
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

./test-lab-3-b.pl yfs1 | grep -q "Passed all"
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

./test-lab-3-c.pl yfs1 | grep -q "Passed all"
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


./test-lab-3-d.sh yfs1 >tmp.1
./test-lab-3-d.sh yfs2 >tmp.2
lcnt=$(cat tmp.1 tmp.2 | grep -o "Passed SYMLINK" | wc -l)

if [ $lcnt -ne 2 ];
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

rm tmp.1 tmp.2

##################################################################################

./test-lab-3-e.sh yfs1 >tmp.1
./test-lab-3-e.sh yfs2 >tmp.2
lcnt=$(cat tmp.1 tmp.2 | grep -o "Passed BLOB" | wc -l)

if [ $lcnt -ne 2 ];
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

rm tmp.1 tmp.2
##################################################################################
robust(){
./test-lab-3-f.sh yfs1 | grep -q "Passed ROBUSTNESS test"
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



##################################################################################
lab3(){
./test-lab-3-g yfs1 yfs2 | grep -q "test-lab-3-a: Passed all tests."
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
			echo "Passed test-lab-3-a (consistency)"
			#echo $score
		fi
fi
}

lab3

# finally reaches here!
#echo "Passed all tests!"

./stop.sh
echo ""
if [ $score -eq 120 ];
then
	echo ">> Lab 3 OK"
else
	echo "Total score: "$score
fi
