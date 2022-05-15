#PBS -l nodes=node01.cluster:ppn=TASKS_PER_NODE+node02.cluster:ppn=TASKS_PER_NODE+node04.cluster:ppn=TASKS_PER_NODE+node08.cluster:ppn=TASKS_PER_NODE
##########PBS -l nodes=node01.cluster:ppn=TASKS_PER_NODE+node02.cluster:ppn=TASKS_PER_NODE+node04.cluster:ppn=TASKS_PER_NODE+node08.cluster:ppn=TASKS_PER_NODE+node09.cluster:ppn=TASKS_PER_NODE+node10.cluster:ppn=TASKS_PER_NODE+node11.cluster:ppn=TASKS_PER_NODE+node12.cluster:ppn=TASKS_PER_NODE
#PBS -l walltime=00:20:00

export MODULEPATH=$MODULEPATH:/home/hli035/success/modulefiles
module initlist
module purge

module load gcc-4.7.2
module load mvapich2-2.0
module load libunwind
module load paraStack
module load cblas
module load dyninst
