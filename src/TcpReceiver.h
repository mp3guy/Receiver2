/*
 * TcpReceiver.h
 *
 *  Created on: 27 Oct 2014
 *      Author: thomas
 */

#ifndef TCPRECEIVER_H_
#define TCPRECEIVER_H_

#include <string>
#include <sys/time.h>
#include <sstream>
#include <opencv2/opencv.hpp>
#include <zlib.h>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <unistd.h>
#include <cassert>
#include <stdio.h>

class TcpReceiver
{
    public:
        TcpReceiver(std::string addr,
                    std::string port,
                    int width,
                    int height)
         : addr(addr),
           port(port),
           width(width),
           height(height)
        {
            decompressionBuffer = new Bytef[width * height * 2];
            image = 0;
        }

        virtual ~TcpReceiver()
        {
            delete [] decompressionBuffer;

            if(image != 0)
            {
                cvReleaseImage(&image);
            }
        }

        bool receive()
        {
            boost::asio::io_service io_service;

            boost::asio::ip::tcp::resolver resolver(io_service);
            boost::asio::ip::tcp::resolver::query query(addr, port);
            boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
            boost::asio::ip::tcp::resolver::iterator end;

            boost::asio::ip::tcp::socket socket(io_service);
            boost::system::error_code error = boost::asio::error::host_not_found;

            while(error && endpoint_iterator != end)
            {
                socket.close();
                socket.connect(*endpoint_iterator++, error);
            }

            if(error)
            {
                static int counter = 0;

                if(counter++ % 3 == 0)
                {
                    std::cout << "\rWaiting... " << addr; std::cout.flush();
                }
                else if(counter++ % 2 == 0)
                {
                    std::cout << "\rWaiting..  " << addr; std::cout.flush();
                }
                else if(counter++ % 1 == 0)
                {
                    std::cout << "\rWaiting.   " << addr; std::cout.flush();
                }

                usleep(500000);

                return false;
            }

            size_t totalLength = 0;
            std::string datastr;
            timeval timestamp;

            while(true)
            {
                boost::asio::streambuf temp_buffer;
                boost::asio::streambuf::mutable_buffers_type temp_bufs = temp_buffer.prepare(65536);
                boost::system::error_code error;

                size_t len = socket.read_some(temp_bufs, error);

                temp_buffer.commit(len);

                totalLength += len;

                boost::asio::streambuf::const_buffers_type bufs = temp_buffer.data();
                std::string str(boost::asio::buffers_begin(bufs), boost::asio::buffers_begin(bufs) + len);

                datastr.append(str);

                if(error == boost::asio::error::eof)
                {
                    break; // Connection closed cleanly by peer.
                }
                else if(error)
                {
                    return false;
                }
            }

            if(totalLength)
            {
                gettimeofday(&timestamp, 0);
                unsigned long long int currentTimestamp = timestamp.tv_sec * 1000000 + timestamp.tv_usec;

                const unsigned char * data = (const unsigned char *)datastr.c_str();

                int rgbSize = *((int *)&data[0]);

                if(image != 0)
                {
                    cvReleaseImage(&image);
                }

                //Uncompressed
                if(rgbSize == width * height * 3)
                {
                    image = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);

                    memcpy(image->imageData, &data[sizeof(int)], width * height * 3);
                }
                else
                {
                    CvMat tempMat = cvMat(1, rgbSize, CV_8UC1, (void *)&data[sizeof(int)]);

                    image = cvDecodeImage(&tempMat);
                }

                int depthSize = datastr.length() - rgbSize - sizeof(int);

                unsigned long decompLength = width * height * 2;

                //Uncompressed
                if(depthSize == width * height * 2)
                {
                    memcpy(&decompressionBuffer[0], &data[sizeof(int) + rgbSize], depthSize);
                }
                else
                {
                    uncompress(&decompressionBuffer[0], (unsigned long *)&decompLength, (const Bytef *)&data[sizeof(int) + rgbSize], depthSize);
                }

                rgb = (unsigned char *)image->imageData;
                depth = (unsigned short*)&decompressionBuffer[0];
                this->timestamp = currentTimestamp;

                return true;
            }

            return false;
        }

        const std::string addr;
        const std::string port;
        unsigned char * rgb;
        unsigned short * depth;
        unsigned long long int timestamp;

    private:
        IplImage * image;
        Bytef * decompressionBuffer;
        const int width;
        const int height;
};


#endif /* TCPRECEIVER_H_ */
