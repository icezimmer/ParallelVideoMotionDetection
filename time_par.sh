#!/bin/bash

trials=10
for (( nw = 1; nw <= 32; nw++ ))      ### Outer for loop ###
do
    sum=0
    for (( t = 1 ; t <= trials; t++ )) ### Inner for loop ###
    do
        val=$(./main 1 0.85 1  $nw | grep -Eo '[0-9]{1,10}')
        sum=$((sum+val))
    done
    echo $((sum / trials))
done