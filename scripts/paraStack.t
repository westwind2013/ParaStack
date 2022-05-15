#!/bin/bash
#Program:
#   This program connects the function of two executables,
#   parse & stack, to enable automatic analysis of jobs. 

#Author:
#   Hongbo Li @ UC Riverside

#used to parse parameters
function parsePara() {

    while getopts "N:n:r:c:d" arg
    do
        case $arg in
            N)
                REQ_NUM_NODES="$OPTARG"
                echo "The number of requested nodes: $REQ_NUM_NODES"
                ;;
            n)
                REQ_NUM_TASKS="$OPTARG"
                echo "The number of requested tasks: $REQ_NUM_TASKS"
                TASKS_PER_NODE=$((REQ_NUM_TASKS / REQ_NUM_NODES))
				echo "The number of tasks per node is: $TASKS_PER_NODE"
				;;
            r)
                R_NUM_TASKS="$OPTARG"
                echo "The real number of tasks: $REQ_NUM_TASKS"
				;;
            c)
                REQ_BINARY_LOC="$OPTARG"
                echo "The full path of command to be executed: $REQ_BINARY_LOC"
                TARGET_NAME=${REQ_BINARY_LOC##*/}
				echo "The command name: $TARGET_NAME"
                TARGET_PATH=${REQ_BINARY_LOC%$TARGET_NAME}
				if [ -z $TARGET_PATH ]; then
					TARGET_PATH=./
				fi
				echo "The command path: $TARGET_PATH"
                ;;
            d)
                H_DEBUG=1
                echo "DEBUG: $H_DEBUG"
                ;;
        esac
    done

    return 0
}

# parse parameters
parsePara "$@"


if [ -z "$H_DEBUG" ]; then
	echo "DEBUG: 0"
fi

# create folder to contain results
resultDir=stackJob.result
if [ ! -d "$resultDir" ]; then
	mkdir "$resultDir"
fi

work_root=$(pwd)
cd "$resultDir"

# create job script for target
cp $PARASTACK_HOME/script.base.t paraStack
chmod 700 paraStack
sed -i "s/REQ_NUM_NODES/$REQ_NUM_NODES/g" paraStack
sed -i "s/TASKS_PER_NODE/$TASKS_PER_NODE/g" paraStack
echo "cd $work_root" >> paraStack
echo "mpirun -n $R_NUM_TASKS $work_root/$TARGET_NAME > \$PBS_JOBID.out &" >> paraStack

if [ ! -z "$H_DEBUG" ]; then
	
	echo "while [ \$(cat \$PBS_NODEFILE | wc -l) -eq 0 ]; do" >> paraStack
	echo "	echo \"waiting for nodefile\"" >> paraStack
	echo "		sleep 1" >> paraStack
	echo "done" >> paraStack

	# change nodefile
	echo "cat \"\$PBS_NODEFILE\" | uniq > hhostfile" >> paraStack
	echo "cat hhostfile > \$PBS_JOBID.ana" >> paraStack
	echo "mv hhostfile $work_root/$resultDir" >> paraStack
	echo "PBS_NODEFILE=$work_root/$resultDir/hhostfile" >> paraStack

	echo "mpirun -n $REQ_NUM_NODES $PARASTACK_HOME/stack -n $R_NUM_TASKS -p $TASKS_PER_NODE  -c $TARGET_NAME >> \$PBS_JOBID.ana &" >> paraStack
fi

echo "wait" >> paraStack
echo "mv \$PBS_JOBID.ana \$PBS_JOBID.out $resultDir" >> paraStack
#echo "rm $resultDir/paraStack.o* $resultDir/hhostfile" >> paraStack 

qsub paraStack
echo -e "complete.\n"
