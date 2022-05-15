#!/bin/bash

#used to parse parameters
function parsePara() {

	while getopts "N:n:c:q:r:d" arg
	do
		case $arg in
			N)
				REQ_NUM_NODES="$OPTARG"
				echo "The number of requested nodes: $REQ_NUM_NODES"
				;;
			n)
				REQ_NUM_TASKS="$OPTARG"
				echo "The number of requested tasks: $REQ_NUM_TASKS"
				;;
			r)
				R_NUM_TASKS="$OPTARG"
				echo "The real number of tasks to launch job: $R_NUM_TASKS"
				;;
			c)
				REQ_BINARY_LOC="$OPTARG"
				echo "The full path of command to be executed: $REQ_BINARY_LOC"
				PROG_NAME=${REQ_BINARY_LOC##*/}
				echo "The command name: $PROG_NAME"
				;;
			q)
				WHICH_QUEUE="$OPTARG"
				echo "The queue name is: $WHICH_QUEUE"
				;;
			d)
				H_DEBUG=1
				echo "DEBUG: $H_DEBUG"
				;;
		esac
	done

	return 0
}

function createScript() {

	if [ ! -f stackJob  ]; then
		cp "$PARASTACK_HOME/script.base.s" stackJob
		echo "stackJob is created"
	else
		echo "stackJob already exist, exit -1!"
		exit -1
	fi

	sed -i "s@H_WHICH_QUEUE@$WHICH_QUEUE@" stackJob
	sed -i "s@H_NUM_NODES@$REQ_NUM_NODES@" stackJob
	sed -i "s@H_NUM_TASKS@$REQ_NUM_TASKS@" stackJob
	sed -i "s@R_NUM_TASKS@$R_NUM_TASKS@" stackJob
	sed -i "s@H_FULL_COMMAND_NAME@$REQ_BINARY_LOC@" stackJob
	sed -i "s@H_COMMAND_NAME@$PROG_NAME@" stackJob

	if [ -z "$H_DEBUG" ]; then
		echo "DEBUG: 0"
		sed -i '13,35d' stackJob
	fi
	return 0
} 

#parse parameters
parsePara "$@"

#create script to submit
createScript

#create result directory
resultDir=stackJob.result
if [ ! -d "$resultDir" ]; then
	mkdir "$resultDir"
fi

#submit job by command sbatch script
sbatch stackJob
echo -e "job submitted.\n"

#complete
mv -f stackJob "$resultDir"
echo -e "complete.\n"

