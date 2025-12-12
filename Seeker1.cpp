void Seeker :: readEOFrames(void)
{
    using namespace std::chrono;

    Mat VideoFrame;
    string timestamp;
    UInt32_t SleepTime = 0;
    bool ReadStatus = false;
    struct _VideoData_ VideoData;

    VideoFrame.create(EO_pCodecCtx->height, EO_pCodecCtx->width, CV_8UC3);

    while (EOVideoReadFlag == true)
    {
        auto timCntStart = high_resolution_clock::now();
        ReadStatus = false;

        // ---- Non-blocking read attempt ----
        int ret = av_read_frame(EO_pFormatCtx, EO_packet);

        if (ret < 0)
        {
            std::cerr << "EO Video reading Failed..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // yield CPU
            continue;
        }

        if (EO_packet->stream_index == EO_videoStream)
        {
            EO_ret = avcodec_send_packet(EO_pCodecCtx, EO_packet);

            if (EO_ret < 0)
            {
                std::cerr << "Decode error" << std::endl;
                av_packet_unref(EO_packet);
                std::this_thread::sleep_for(std::chrono::milliseconds(1)); // yield CPU
                continue;
            }

            EO_ret = avcodec_receive_frame(EO_pCodecCtx, EO_pFrame);

            if (EO_ret == 0)
            {
                // ---- Convert frame ----
                sws_scale(EO_img_convert_ctx,
                          (uint8_t const* const*)EO_pFrame->data,
                          EO_pFrame->linesize,
                          0,
                          EO_pCodecCtx->height,
                          EO_pFrameEO->data,
                          EO_pFrameEO->linesize);

                memcpy(VideoFrame.data,
                       EO_pFrameEO->data[0],
                       EO_pCodecCtx->height * EO_pFrameEO->linesize[0]);

                timestamp = getTimeStamp();
                putText(VideoFrame, timestamp, Point(20, 40),
                        FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);

                if (AI_Type == 2)
                {
                    VideoData.VideoFrame = VideoFrame;
                    VideoData.VideoType = AI_Type;
                    VideoData.FrameTime = steady_clock::now();

                    if (CameraData.FrameFlag == true)
                    {
                        CameraData.FrameFlag = false;
                        cout << "Dropping VideoFrames because inference is slow..." << endl << flush;
                    }

                    CameraData.VideoFrame.release();
                    CameraData = VideoData;
                    CameraData.FrameFlag = true;
                }

                ReadStatus = true;
            }
        }

        av_packet_unref(EO_packet);

        // ---- CPU-friendly sleep/yield ----
        if (ReadStatus == true)
        {
            auto timCntEnd = high_resolution_clock::now();
            auto timDuration = duration_cast<milliseconds>(timCntEnd - timCntStart).count();

            SleepTime = FrameDelay > timDuration ? (FrameDelay - timDuration) : 0;

            // Only sleep if not Webcam/RTSP/HTTP
            if (!(EO_VideoSrc.find("/dev/video") == 0 ||
                  EO_VideoSrc.find("rtsp://") == 0 ||
                  EO_VideoSrc.find("http://") == 0 ||
                  EO_VideoSrc.find("https://") == 0))
            {
                if (SleepTime > 0)
                    std::this_thread::sleep_for(std::chrono::milliseconds(SleepTime));
            }
        }
        else
        {
            // Always yield CPU a bit
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

