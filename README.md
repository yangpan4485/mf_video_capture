# mf_video_capture
Windows 下使用 Media Foundation 进行屏幕采集，一种是使用 IMFSourceReader（win7 和 win10），另外一种使用 IMFCaptureEngine（win8 以上）

## 编译方法

+ git clone https://github.com/yangpan4485/mf_video_capture.git
+ cd mf_video_capture
+ md build & cd build
+ cmake ..
+ 打开 sln 编译运行