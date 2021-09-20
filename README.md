# MotionDetect
Simple OpenCV C++ motion detect on Raspberry Pi

## How to build
- Install OpenCV on Raspberry Pi. See OpenCV offical page.
    <https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html>

- Build the project.
    ```
    cd MotionDetect
    mkdir build && cd build
    cmake ..
    make
    ```

## Usage
- Set camera rotation if needed.
    ```
    sudo v4l2-ctl --set-ctrl=rotate=180
    ```
- Run motion detect. It records .avi files.
    ```
    sudo ./motion_detect
    ```
