class VideoDetection {
    protected:
    
        const int width,height; // Shape of frames
        const int ksize;        // Number of kernel's pixel (square kernel)
        const long fsize;       // Number of frame's pixels
        const int offset;       // (ksize-1)/2
        const float k;          // % of pixels that must be different to trigger "detection"
        Mat* background;        // Background image used to comparisons

    public:
        VideoDetection(const int width,const int height,const float k):
            width(width),height(height),ksize(9),
            fsize(width * height),offset(1),k(k),background(nullptr) { }

        void setBackground(Mat* background) {
            this->background = background;
        }

        /**
        Tranform the multi-channel RGB image into padded graysacle (single-channel).
        For each pixel we make linear combination in order to produce a grayscale pixel.
         */
        void RGBtoGray(const Mat input, Mat* output) {
            int i,j;

            // We take each RGB pixel and we tranform it into grayscale pixel
            for (i = 0; i < height; i++) {
                for (j = 0; j < width; j++){
                    output->at<uchar>(i, j) = (input.at<Vec3b>(i, j)[2] + input.at<Vec3b>(i, j)[1] + input.at<Vec3b>(i, j)[0]) / 3;
                }
            }
        }

        /**
        We apply the convolution to the image, the covolution is performed passing a matrix/kernel
        which is formed by all one (the result is to take a average of neighboors pixel)  to the grayscale image. 
         */
        void smoothing(Mat* input, Mat* output) {
        
            int i,j,c,r,col_LB,col_UB,row_LB,row_UB; 
            //float sum;
            uchar sum;

            // For each pixel on grayscale image
            for (i = 0; i < height ; i++) {
                for (j = 0; j < width ; j++) {
                    sum = 0;
                    if(i>0 && i<height-1 && j>0 && j<width-1) {
                        for(r=-offset;r<=offset;r++)
                            for(c=-offset;c<=offset;c++) {
                                sum += input->at<uchar>(i+r,j+c);
                            }
                        output->at<uchar>(i, j) = sum/ksize;
                    }
                    else {
                        row_LB = i > 0 ? -offset : 0;
                        row_UB = i < height-1 ? offset : 0;
                        col_LB = j > 0 ? -offset : 0;
                        col_UB = j < width-1 ? offset : 0;
                        for(r=row_LB;r<=row_UB;r++)
                            for(c=col_LB;c<=col_UB;c++) {
                                sum += input->at<uchar>(i+r,j+c);
                            }
                        output->at<uchar>(i, j) = sum / ((row_UB-row_LB+1)*(col_UB-col_LB+1));
                    }
                }
            }
        }

        /**
        We compare the source and the background in order to detect the "motion"
         */
        ushort detection(Mat* input) {

            int i,j;
            ulong diff = 0;
            float perc;

            // We compare each pixels to perform the difference
            for (i = 0; i < height; i++) {
                for (j = 0; j < width; j++){
                    // If they are different, we increase the counter
                    diff += background->at<uchar>(i, j) != input->at<uchar>(i, j) ;
                }
            }

            // "Differents pixels" are divided by all pixels to obtain a percentage
            perc = (((float)diff)/fsize);  
            // if perc > k then the frame is "different" from background
            return  perc > k ;       
        }

        ushort composition(const Mat input) {
        
            const ushort layers = 3;
            int i,j,c,r,col_LB,col_UB,row_LB,row_UB; 
            uchar sum;
            float perc;
            ulong diff = 0;

            // For each pixel on grayscale image
            for (i = 0; i < height ; i++) {
                for (j = 0; j < width ; j++) {
                    sum = 0;
                    if(i>0 && i<height-1 && j>0 && j<width-1) {
                        for(r=-offset;r<=offset;r++)
                            for(c=-offset;c<=offset;c++) {
                                sum += (input.at<Vec3b>(i+r, j+c)[2] + input.at<Vec3b>(i+r, j+c)[1] + input.at<Vec3b>(i+r, j+c)[0]) / layers;
                            }
                        diff += background->at<uchar>(i, j) != sum / ksize;
                    }
                    else {
                        row_LB = i > 0 ? -offset : 0;
                        row_UB = i < height-1 ? offset : 0;
                        col_LB = j > 0 ? -offset : 0;
                        col_UB = j < width-1 ? offset : 0;
                        for(r=row_LB;r<=row_UB;r++)
                            for(c=col_LB;c<=col_UB;c++) {
                                sum += (input.at<Vec3b>(i+r, j+c)[2] + input.at<Vec3b>(i+r, j+c)[1] + input.at<Vec3b>(i+r, j+c)[0]) / layers;
                            }
                        diff += background->at<uchar>(i, j) != sum / ( (row_UB-row_LB+1)*(col_UB-col_LB+1) );
                    }
                }
            }

            // "Differents pixels" are divided by all pixels to obtain a percentage
            perc = (((float)diff)/fsize);   
            // if perc > k then the frame is "different" from background
            return  perc > k ;        
        }
        
};