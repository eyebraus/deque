
for t in 2 4 8 16 32 64
do
    for w in "stack" "queue" "random"
    do
        for i in 1 2 3 4 5
        do
            echo iteration $t:timing:$w:$i
            exp/nonblock -t $t -e timing -w $w -o 5000 > exp/results/nonblock_timing_"$w"_"$t"_"$i".out
            sleep 2
        done
    done
done

for t in 2 4 8 16 32 64
do
    for w in "stack" "queue" "random"
    do
        for i in 1 2 3 4 5
        do
            echo iteration $t:throughput:$w:$i
            exp/nonblock -t $t -e throughput -w $w -o 2 > exp/results/nonblock_throughput_"$w"_"$t"_"$i".out
            sleep 2
        done
    done
done
