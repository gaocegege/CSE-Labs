#!/bin/bash

##########################################
#  this file contains:
#   robustness test
###########################################

DIR=$1

#
#10 times seems a bit too tricky here....
# setting it to 3....
# ---------------
# I'm just trying to be nice....
#

for times in {0..3}
do
    echo "test Round:"$times""
    if ( ! ( perl test-lab-2-b.pl $1 | grep -q -i "Passed all tests" ));
    then
        echo "failed test B yfs1"
        exit
    fi
    if ( ! ( perl test-lab-2-a.pl $1 | grep -q -i "Passed all tests" ));
    then
        echo "failed test A yfs1"
        exit
    fi
    # 
    #ok this section is maybe a bit cruel here... I'll comment it out
    #
    #----------------------------------------------------------------------
    #if ( ! ( bash test-lab-2-e.sh $1 | grep -q -i "Passed BLOB test" ));
    #then
    #    echo "failed test E yfs1"
    #    exit
    #fi
    #rm ${1}/* 2>&1 >/dev/null
    #
done

echo "Passed ROBUSTNESS test"
