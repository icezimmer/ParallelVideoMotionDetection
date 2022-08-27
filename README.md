# ParallelVideoMotionDetection

Command to compile
    ./compile.sh
Command to run
    ./main a b
where
    a=Threshold i.e. percentage of pixels for the move detection (k>0 and k=<1)
    b= [0 = compute number of frame with motion detected
        1 = Compute the elapsed time
        2 = Compute the time of the single operation for each frame, and finally the means]

To see the parallel degree
    for nw in {1..32}; do ./main 1 0.85 [1 | 2] $nw; done