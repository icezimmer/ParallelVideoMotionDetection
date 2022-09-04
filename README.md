# ParallelVideoMotionDetection

Command to compile<br>
```
    ./compile.sh
```
Command to run<br>
```
    ./main version k stat nw
```
where<br>
* version = [0 = sequential<br>
        1 = native parallel<br>
        2 = pipeline (ff) <br>
        3 = farm (ff) <br>
        4 = master-worker (ff)]<br>
* k = Threshold i.e. percentage of pixels for the move detection (k>0 and k=<1)<br>
* out = [0 = compute number of frame with motion detected<br>
        1 = Compute the elapsed time<br>
        2 = Compute the time of the single operation for each frame, and finally the means (for SEQUENTIAL) | Compute overhead (for PARALLEL)]<br>
* nw = number of worker (optional)<br>

Add the flag ``-fopt-info-vec-all`` to see all the optimizated loop (vectorization), with ``|& grep 'optimized: loop'`` at the end

To see the parallel degree<br>
```
    for nw in {1..32}; do ./main 1 0.85 [0 | 1 | 2] $nw; done
```