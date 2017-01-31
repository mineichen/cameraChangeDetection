/**
 * g++ $(pkg-config --cflags --libs opencv) -o libTest -lpthread -ldl libTest.cpp build-host-linux-64/harris.a
 */
#include <stdio.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
/*
extern "C" {
    #include "build-host-linux-64/harris_uint8.h"
}


buffer_t createHalideBuffer(cv::Mat image)
{
    buffer_t buffer;
    memset(&buffer, 0, sizeof(buffer));
    buffer.host = image.data;
    buffer.elem_size = image.elemSize1();
    buffer.extent[0] = image.cols;
    buffer.extent[1] = image.rows;
    buffer.extent[2] = image.channels();
    buffer.stride[0] = image.step1(1);
    buffer.stride[1] = image.step1(0);
    buffer.stride[2] = 1;
    return buffer;
}

void applyHarris(cv::Mat gray_image){
    int width = gray_image.cols,
    height = gray_image.rows;
    
    cv::Mat out(height-8, width-8,CV_8UC1);
    printf("%i,%i\n", width, height);
    
    buffer_t input_buf = createHalideBuffer(gray_image);
    buffer_t output_buf = createHalideBuffer(out);
    output_buf.min[0] = output_buf.min[1] = 4;
    
    int error = harris_uint8(&input_buf, &output_buf);
    
    cv::imwrite( "Gray_Image_harris.jpg", out );
}
*/

void drawHotspots(cv::Mat image, const char* filename)
{
    FILE * fp = fopen(filename, "r");
    int x,y;
    cv::Scalar red( 0, 255, 255 );
    cv::Scalar blue( 255, 255, 0);
    const int filterOffset = 4;
    
    while(!feof(fp)) {
        fscanf(fp, "%d", &x);
        fscanf(fp, "%d", &y);
        
        cv::circle(image, cv::Point(x+filterOffset,y+filterOffset), 10, (((x/100)+(y/100))%2) ? blue : red);
    }
    
    for(int i = filterOffset; i < image.rows; i+=100) {
        cv::line(image, cv::Point(0, i), cv::Point(image.cols - 1, i), red);
    }
    
    for(int i = filterOffset; i < image.cols; i+=100) {
        cv::line(image, cv::Point(i, 0), cv::Point(i, image.rows - 1), red);
    }
    
    fclose(fp);
    cv::imwrite( "hotspots.jpg", image );
}

int main( int argc, char** argv )
{
    char* imageName = argv[1];
    
    cv::Mat image;
    image = cv::imread( imageName, 1 );
    
    if( argc != 2 || !image.data )
    {
        printf( " No image data \n " );
        return -1;
    }
    
    //cv::Mat gray_image;
    //cv::cvtColor( image, gray_image, CV_BGR2GRAY );
    //cv::imwrite( "output.jpg", gray_image );
    
    //applyHarris(gray_image);
    drawHotspots(image, "hotspots.data");
    
    return 0;
}
