#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdio.h>

using namespace cv;
using namespace std;

const Scalar WHITE = Scalar(255, 255, 255), BLACK = Scalar(0, 0, 0), BLUE = Scalar(255, 0, 0), GREEN = Scalar(0, 255, 0), RED = Scalar(0, 0, 255), YELLOW = Scalar(0, 255, 255);

int main(int argc, const char** argv)
{
    Mat currentFrame, nextFrame, currentFrame_gray, nextFrame_gray, currentFrame_blur, nextFrame_blur, morph, diff, thresh;

    float frameRate, videoTimeElapsed, currentFrameCount, totalFrameCount, frameHeight, frameWidth, fourCC;

    VideoCapture capture;
    capture.open("input/demo-v.webm");
    if(!capture.isOpened())
    {
        return -1;
    }

    frameRate = capture.get(CAP_PROP_FPS);
    totalFrameCount = capture.get(CAP_PROP_FRAME_COUNT);
    frameHeight = capture.get(CAP_PROP_FRAME_HEIGHT);
    frameWidth = capture.get(CAP_PROP_FRAME_WIDTH); 
    fourCC = capture.get(CAP_PROP_FOURCC);

    cout << "Video frame rate: " << frameRate << endl; cout << endl;
    cout << "Video total frame count: " << totalFrameCount << endl; cout << endl;
    cout << "Video frame height: " << frameHeight << endl; cout << endl;
    cout << "Video frame width: " << frameWidth << endl; cout << endl;
    cout << "Video codec: " << fourCC << endl; cout << endl;
    cout << "-------------------------------------------------------------------------------" << endl; cout << endl;

    capture.read(currentFrame);
    capture.read(nextFrame);
    int key = 0;
    bool isRecording = false;
    int detectionCounter = 0;
    int noDetectionCounter = 0;
    int framesStored = 0;
    
    vector<Mat> detectionFrames;
    while(capture.isOpened() && key != 27)
    {
        currentFrameCount = capture.get(CAP_PROP_POS_FRAMES) - 1;
        if(currentFrameCount == totalFrameCount) break;
        //cout << currentFrameCount << endl;
        cvtColor(currentFrame, currentFrame_gray, COLOR_BGR2GRAY);
        cvtColor(nextFrame, nextFrame_gray, COLOR_BGR2GRAY);

        GaussianBlur(currentFrame_gray, currentFrame_blur, Size(5, 5), 0);
        GaussianBlur(nextFrame_gray, nextFrame_blur, Size(5, 5), 0);

        absdiff(currentFrame_blur, nextFrame_blur, diff);

        threshold(diff, thresh, 30, 255.0, THRESH_BINARY);

        morph = thresh.clone();

        for(int i = 0; i < 3; i++)
        {
            dilate(morph, morph, getStructuringElement(MORPH_RECT, Size(7, 7)));
        }

        vector<vector<Point> > contours;
        findContours(morph, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        Mat img_out = nextFrame.clone();

        vector<vector<Point>> convexHulls(contours.size());
        for(int i = 0; i < contours.size(); i++)
        {
            convexHull(contours[i], convexHulls[i]);
        }

        bool isDetection = false;
        for (int i = 0; i < convexHulls.size(); i++)
        {
            Rect rect = boundingRect(convexHulls[i]);
            double aspectRatio, diagonalSize;
            aspectRatio = (double)rect.width / (double)rect.height;
            diagonalSize = sqrt(pow(rect.width, 2) + pow(rect.height, 2));

            if(aspectRatio > 0.3 && aspectRatio < 4.0 && rect.width > 50.0 && rect.height > 50.0 && diagonalSize > 50.0)
            {
                isDetection = true;
                rectangle(img_out, rect.tl(), rect.br(), RED, 1);
            }
        }

        if(isDetection)
        {
            detectionCounter++;
            noDetectionCounter = 0;
        }
        else
        {
            noDetectionCounter++;
            detectionCounter = 0;
        }
        if(detectionCounter >= 10 && isRecording != true)
        {
            isRecording = true;
            cout << "isRecording changed to true" << endl;
            cout << "Started recording" << endl;
        }

        if(isRecording == true)
        {
            detectionFrames.push_back(currentFrame);
            framesStored++;
            if(framesStored >= 9000 || noDetectionCounter >= 300 || currentFrameCount == totalFrameCount - 1)
            {
               if(framesStored >= 9000)
                {
                    cout << "Frames stored >= 9000: " << framesStored << endl;
                }
                if(noDetectionCounter >= 300)
                {
                    cout << "noDetection counter >= 300 " << noDetectionCounter << endl;
                }
                if(currentFrameCount == totalFrameCount - 1)
                {
                    cout << "Last frame reached." << endl;
                }  

                string filename = "output/out" + to_string(currentFrameCount) + ".avi";
                cout << filename << endl;
                VideoWriter video(filename, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), frameRate, Size(frameWidth, frameHeight));
                for(int i = 0; i < detectionFrames.size(); i++)
                {
                    video.write(detectionFrames.front());
                    detectionFrames.erase(detectionFrames.begin());
                }
                video.release();
                framesStored = 0;
                isRecording = false;
                cout << "Video Stored, isRecording changed to false." << endl;
            }
        }

        imshow("Original", currentFrame);
        //imshow("Threshold", thresh);
        //imshow("Morphed", morph);
        //imshow("Output", img_out);

        currentFrame = nextFrame.clone();
        capture.read(nextFrame);

        key = waitKey(30);
    }
    return 0;
}