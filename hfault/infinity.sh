#!/bin/bash

iter=0

while [ $iter -ge 0 ]; do
	echo "@$iter infinite loop.."
	iter=$((iter + 1))
	sleep 1
done
