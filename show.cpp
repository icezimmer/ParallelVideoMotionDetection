#include <iostream>
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

int main(int argc, char * argv[]) {

  VideoCapture cap(argv[1]);
  
  while(true) {
    Mat frame;

    // Capture frame-by-frame
    cap >> frame;

    // If the frame is empty, break immediatelly
    if(frame.empty()){
        cout << "WARNING!: Empty frame."<<endl;
        break;
    }

    // Display the resulting frame
    imshow("Frame", frame );
    waitKey(25);
  }

	// When everything done, release the video capture object
  cap.release();

 // Closes all the frames
  destroyAllWindows();

  return(0);
}