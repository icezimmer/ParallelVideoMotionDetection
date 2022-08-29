// Opencv Library
#include <opencv2/opencv.hpp>

// Fastflow Libraries
#include <ff/ff.hpp>
#include <ff/pipeline.hpp>
#include <ff/farm.hpp>
#include <ff/parallel_for.hpp>
#include <ff/map.hpp>

// Useful libraries
#include <iostream>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <future>
#include <vector>
#include <queue>

using namespace std;	
using namespace ff;
using namespace cv;

#define ushort unsigned short
#define ulong unsigned long
#define BLACK Scalar(0)
#define VIDEOSOURCE "./data/sample.mp4"
#define ERROR(cond,msg) if(cond) { cout << msg << endl; exit(1);}

// Include my source
#include "src/Utimer.cpp"
#include "src/VideoDetection.cpp"
#include "src/Sequential.cpp"
#include "src/NativeParallel.cpp"
#include "src/FastFlow.cpp"

int main(int argc,char* argv[]) {

	ERROR(argc<4,"Wrong arguments:\n\tVersion[ 0 = Sequential| 1 = NativeParallel| 2 = Pipeline(FF)| 3 = Farm(FF)| 4 = MasterWorker(FF))]\n\tThreshold in (0,1]\n\tTime execution[ 0 = False| 1 = True]\n\tOptional: Number of workers (>=0)\n")

	int version = atoi(argv[1]); // Version
	float k     = atof(argv[2]); // Threshold for the move detection (percentage in (0,1])
	int stat    = atoi(argv[3]); // Print elapsed time
	int nw     = argc > 4 ? atoi(argv[4]) : thread::hardware_concurrency();

	if (version == 0) {
		Sequential s(VIDEOSOURCE,k);
		if(stat == 0) s.execute_to_result();
		else if (stat == 1) s.execute_to_stat();
		else if (stat == 2) s.execute_to_stat2();
		else exit(1);		
	}else if (version == 1) {
		NativeParallel np(VIDEOSOURCE,k,nw);
		if(stat == 0) np.execute_to_result();
		else if (stat == 1) np.execute_to_stat();
		else exit(1);	
	}else if (version == 2) {
		Pipe p(VIDEOSOURCE,k,nw);
		if(stat == 0) p.execute_to_result();
		else if (stat == 1) p.execute_to_stat();
		else exit(1);	
	}else if (version == 3) {
		Farm f(VIDEOSOURCE,k,nw);
		if(stat == 0) f.execute_to_result();
		else if (stat == 1) f.execute_to_stat();
		else exit(1);	
	}else if (version == 4) {
		MasterWorker mw(VIDEOSOURCE,k,nw);
		if(stat == 0) mw.execute_to_result();
		else if (stat == 1) mw.execute_to_stat();
		else exit(1);	
	}else exit(1);

	return 0;
}