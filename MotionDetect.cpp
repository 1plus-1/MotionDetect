/*
* write something here!
*/
#include <opencv2/opencv.hpp>
#include <ctime>
#include <string>
#include <regex>
#include <time.h>

//#define DEBUG

#define IMAGE_WIDTH 640
#define IMAGE_HEIGHT 360
#define CONTOUR_PIXEL_NUMBER_THRESH  500//pixels
#define RECORDING_DURATION_ON_STATIC  5//seconds

uint64_t get_time_us(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	uint64_t t = ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL;

	return t;
}

std::string get_local_time_str(void)
{
    std::time_t t = std::time(nullptr);
    //get time: "Wed Sep 15 14:15:13 2021\n"
    //and convert to std::string
    std::string timeStr(std::asctime(std::localtime(&t)));
    //remove the useless chars at the end
    while(timeStr.size() > 0 && !std::isdigit(timeStr.back()))
        timeStr.pop_back();
    return timeStr;
}

std::string local_time_to_name(void)
{
    //replace space with underline: "Wed_Sep_15_14:15:13_2021"
    std::string fileName = std::regex_replace(get_local_time_str(), std::regex(" "), "_");
    //replace colon with dash: "Wed_Sep_15_14-15-13_2021"
    fileName = std::regex_replace(fileName, std::regex(":"), "-");
    return fileName;
}

int main(int argc, char **argv)
{
#ifdef DEBUG
    int count = 0;
#endif
    cv::Mat frame, gray, blur, average32, average8, delta, thresh;
    std::vector<std::vector<cv::Point>> cnts;
    cv::VideoCapture camera(0);
    cv::VideoWriter *video = NULL;
    std::time_t lastMotionTime, t;

    bool bVideoRecording = false;

    if(!camera.isOpened())
        printf("Open camera error\n");
    else
        printf("Open camera succeed\n");

    //set height and width
    camera.set(cv::CAP_PROP_FRAME_WIDTH, IMAGE_WIDTH);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, IMAGE_HEIGHT);

    //initiate the average with first frame
    camera.read(frame);
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, blur, cv::Size(21, 21), 0);

    //see https://stackoverflow.com/questions/7059817/assertion-failed-with-accumulateweighted-in-opencv
    //average32 = Mat::zeros(blur.size(), CV_32FC1);
    //use above zero image would cause motion detected immediately
    blur.convertTo(average32, CV_32FC1);
    cv::accumulateWeighted(blur, average32, 0.05);
    //printf("%d, %d---%d, %d\n", blur.depth(), average32.depth(), blur.channels(), average32.channels());

    printf("start detection at %s\n", get_local_time_str().c_str());

    //used for real time FPS calculation
    uint32_t fps = 15;
    uint64_t lastTime = get_time_us();

    //run
    while(camera.read(frame)) {
        bool bMotionDetected = false;

        //convert to grayscale
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::GaussianBlur(gray, blur, cv::Size(21, 21), 0);

        //accumulate the weighted average between the current frame and previous frame
        cv::accumulateWeighted(blur, average32, 0.05);
        //convert CV_32F to CV_8U
        cv::convertScaleAbs(average32, average8);

        //compute difference and find contours
        cv::absdiff(blur, average8, delta);
        cv::threshold(delta, thresh, 25, 255, cv::THRESH_BINARY);
        cv::dilate(thresh, thresh, cv::Mat(), cv::Point(-1,-1), 2);
        cv::findContours(thresh, cnts, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        for(int i = 0; i< cnts.size(); i++) {
            if(cv::contourArea(cnts[i]) >= CONTOUR_PIXEL_NUMBER_THRESH) {
                //printf("c=%d   i=%d   %f\n", count, i, contourArea(cnts[i]));
                bMotionDetected = true;
                break;
            }
        }

        if(bMotionDetected) {
            if(!bVideoRecording) {
                bVideoRecording = true;
                video = new cv::VideoWriter(local_time_to_name()+".avi",
                    cv::VideoWriter::fourcc('M','J','P','G'), fps, cv::Size(IMAGE_WIDTH, IMAGE_HEIGHT));
                printf("  start recording at %s with %u fps\n", get_local_time_str().c_str(), fps);
            }
            // update time
            lastMotionTime = std::time(nullptr);
        } else {
            if(bVideoRecording) {                
                if(std::difftime(std::time(nullptr), lastMotionTime) > RECORDING_DURATION_ON_STATIC) {
                    video->release();
                    delete video;
                    video = NULL;
                    bVideoRecording = false;
                    printf("  stop recording at %s\n", get_local_time_str().c_str());
                }
            }
        }

        if(bVideoRecording) {
            //put date and time on frame
            cv::putText(frame, get_local_time_str() + " FPS: " + std::to_string(fps),
                cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0));
            video->write(frame);
        }

#ifdef DEBUG
        if(count % 10 == 0) {
            cv::imwrite("image/" + std::to_string(count) + "-1-frame.jpg", frame);
            cv::imwrite("image/" + std::to_string(count) + "-2-gray.jpg", gray);
            cv::imwrite("image/" + std::to_string(count) + "-3-blur.jpg", blur);
            cv::imwrite("image/" + std::to_string(count) + "-4-average8.jpg", average8);
            cv::imwrite("image/" + std::to_string(count) + "-5-delta.jpg", delta);
            cv::imwrite("image/" + std::to_string(count) + "-6-thresh.jpg", thresh);
        }
        count++;
#endif

        //calculate the real FPS
        uint64_t t = get_time_us();
        uint64_t loopTimeUs = t - lastTime;
        uint32_t lastFps = (uint32_t)(1000000/loopTimeUs);
        //simply get the average
        fps = (fps + lastFps) / 2;
        lastTime = t;
    }

    if(bVideoRecording) {
        video->release();
        delete video;
        video = NULL;
        bVideoRecording = false;
        std::time_t t = std::time(nullptr);
        printf("  stop recording at %s\n", get_local_time_str().c_str());
    }

    camera.release();

    return 0;
}
