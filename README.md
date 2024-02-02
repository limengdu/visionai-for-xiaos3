# visionai-for-xiaos3

Idea reference: thatproject, original code source: https://github.com/0015/ThatProject/tree/master

Procedure:

1. Install the gcloud CLI. (https://cloud.google.com/sdk/docs/install)

2. Initialize the gcloud CLI. (https://cloud.google.com/sdk/docs/initializing)

3. `npm install --save @google-cloud/vision`

4. node test.js

5. upload the code **VisionAI_for_XIAO_ESP32S3_Sense.ino**

6. node server.js

The camera's frame recognition and shooting is done continuously at 5-second intervals, so keep an eye on your Google account balance!

Please modify the ino file with the network name and password and IP address information.

```cpp
#define ILI9341_DRIVER       // Generic driver for common displays
#define TFT_MISO D9
#define TFT_MOSI D10
#define TFT_SCLK D8
#define TFT_CS   D0  // Chip select control pin
#define TFT_DC   D3  // Data Command control pin
#define TFT_RST  D1  // Reset pin (could connect to RST pin)
```

LED 5V
VCC 3.3V

https://www.youtube.com/watch?v=bpCCqerQ56o
