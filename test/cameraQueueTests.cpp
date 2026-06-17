#include <unity.h>

#include "multi-state-utils/ImageTransfers/OpenMVReceiver.h"
#include "multi-state-utils/ImageTransfers/OpenMVReceiver.cpp"

static bool feedReceiverChunk(OpenMVReceiver& receiver, const String& input)
{
    int inputLength = 0;
    receiver.testInput(input, inputLength);

    TEST_ASSERT_EQUAL(input.length(), inputLength);

    return receiver.runReceiver();
}

static void feedCompleteImage(OpenMVReceiver& receiver, const String& image)
{
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "IMG_BEGIN"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, image));
    TEST_ASSERT_TRUE(feedReceiverChunk(receiver, "IMG_END"));
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_open_mv_receiver_queues_and_returns_images_in_order(void)
{
    OpenMVReceiver receiver;

    const String imageOne = "image_01: nose cone visible";
    const String imageTwo = "image_02: payload bay centered";
    const String imageThree = "image_03: parachute bundle seen";

    feedCompleteImage(receiver, imageOne);
    feedCompleteImage(receiver, imageTwo);
    feedCompleteImage(receiver, imageThree);

    TEST_ASSERT_EQUAL_UINT8(3, receiver.queueSize());

    String receivedImage = "";
    int receivedByteCount = 0;

    TEST_ASSERT_TRUE(receiver.getImage(receivedImage, receivedByteCount));
    TEST_ASSERT_EQUAL_STRING(imageOne.c_str(), receivedImage.c_str());
    TEST_ASSERT_EQUAL(imageOne.length(), receivedByteCount);

    TEST_ASSERT_TRUE(receiver.getImage(receivedImage, receivedByteCount));
    TEST_ASSERT_EQUAL_STRING(imageTwo.c_str(), receivedImage.c_str());
    TEST_ASSERT_EQUAL(imageTwo.length(), receivedByteCount);

    TEST_ASSERT_TRUE(receiver.getImage(receivedImage, receivedByteCount));
    TEST_ASSERT_EQUAL_STRING(imageThree.c_str(), receivedImage.c_str());
    TEST_ASSERT_EQUAL(imageThree.length(), receivedByteCount);

    TEST_ASSERT_EQUAL_UINT8(0, receiver.queueSize());
    TEST_ASSERT_FALSE(receiver.getImage(receivedImage, receivedByteCount));
}

void test_open_mv_receiver_drops_oldest_images_when_queue_is_overloaded(void)
{
    OpenMVReceiver receiver;

    const String images[] = {
        "image_01: launch rail closeup",
        "image_02: avionics sled visible",
        "image_03: payload bay centered",
        "image_04: nose cone visible",
        "image_05: fin can profile",
        "image_06: recovery harness packed",
        "image_07: drogue bundle visible",
        "image_08: main parachute bundle",
        "image_09: camera board closeup",
        "image_10: horizon line visible",
        "image_11: landing zone marker",
        "image_12: final payload view"
    };

    for(int i = 0; i < 12; i++) {
        feedCompleteImage(receiver, images[i]);
    }

    TEST_ASSERT_EQUAL_UINT8(10, receiver.queueSize());

    String receivedImage = "";
    int receivedByteCount = 0;

    for(int i = 2; i < 12; i++) {
        TEST_ASSERT_TRUE(receiver.getImage(receivedImage, receivedByteCount));
        TEST_ASSERT_EQUAL_STRING(images[i].c_str(), receivedImage.c_str());
        TEST_ASSERT_EQUAL(images[i].length(), receivedByteCount);
    }

    TEST_ASSERT_EQUAL_UINT8(0, receiver.queueSize());
    TEST_ASSERT_FALSE(receiver.getImage(receivedImage, receivedByteCount));
}

void test_open_mv_receiver_uses_img_begin_byte_count_metadata(void)
{
    OpenMVReceiver receiver;

    const String base64Image = "QUJDREVGRw==";
    const int decodedByteCount = 7;

    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "IMG_BEGIN 7"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, base64Image));
    TEST_ASSERT_TRUE(feedReceiverChunk(receiver, "IMG_END"));

    String receivedImage = "";
    int receivedByteCount = 0;

    TEST_ASSERT_TRUE(receiver.getImage(receivedImage, receivedByteCount));
    TEST_ASSERT_EQUAL_STRING(base64Image.c_str(), receivedImage.c_str());
    TEST_ASSERT_EQUAL(decodedByteCount, receivedByteCount);

    TEST_ASSERT_EQUAL_UINT8(0, receiver.queueSize());
}

void test_open_mv_receiver_parses_detection_json_frame(void)
{
    OpenMVReceiver receiver;

    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "{\"type\":\"det_begin\",\"count\":2}"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver,
        "{\"type\":\"blob\",\"index\":0,\"x\":80,\"y\":60,\"w\":20,\"h\":15,\"a\":10,\"b\":8,\"r\":45,\"c\":820}"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver,
        "{\"type\":\"blob\",\"index\":1,\"x\":40,\"y\":30,\"w\":12,\"h\":10,\"a\":6,\"b\":5,\"r\":-20,\"c\":500}"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver,
        "{\"type\":\"horizon\",\"x1\":0,\"y1\":60,\"x2\":159,\"y2\":62}"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "{\"type\":\"det_end\"}"));

    TEST_ASSERT_TRUE(receiver.hasDetections());

    VisionDetections detections;
    TEST_ASSERT_TRUE(receiver.getDetections(detections));
    TEST_ASSERT_EQUAL_UINT8(2, detections.blobCount);

    TEST_ASSERT_EQUAL_UINT8(0, detections.blobs[0].index);
    TEST_ASSERT_EQUAL_UINT8(80, detections.blobs[0].x);
    TEST_ASSERT_EQUAL_UINT8(60, detections.blobs[0].y);
    TEST_ASSERT_EQUAL_UINT8(20, detections.blobs[0].width);
    TEST_ASSERT_EQUAL_UINT8(15, detections.blobs[0].height);
    TEST_ASSERT_EQUAL_UINT8(10, detections.blobs[0].ellipseA);
    TEST_ASSERT_EQUAL_UINT8(8, detections.blobs[0].ellipseB);
    TEST_ASSERT_EQUAL_INT16(45, detections.blobs[0].rotation);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.820f, detections.blobs[0].confidence);

    TEST_ASSERT_EQUAL_UINT8(1, detections.blobs[1].index);
    TEST_ASSERT_EQUAL_UINT8(40, detections.blobs[1].x);
    TEST_ASSERT_EQUAL_INT16(-20, detections.blobs[1].rotation);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.500f, detections.blobs[1].confidence);

    TEST_ASSERT_TRUE(detections.horizon.valid);
    TEST_ASSERT_EQUAL_UINT8(0, detections.horizon.x1);
    TEST_ASSERT_EQUAL_UINT8(60, detections.horizon.y1);
    TEST_ASSERT_EQUAL_UINT8(159, detections.horizon.x2);
    TEST_ASSERT_EQUAL_UINT8(62, detections.horizon.y2);

    // frame consumed: no second read, and image queue is untouched
    TEST_ASSERT_FALSE(receiver.getDetections(detections));
    TEST_ASSERT_EQUAL_UINT8(0, receiver.queueSize());
}

void test_open_mv_receiver_handles_detection_frame_without_horizon(void)
{
    OpenMVReceiver receiver;

    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "{\"type\":\"det_begin\",\"count\":1}"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver,
        "{\"type\":\"blob\",\"index\":0,\"x\":5,\"y\":6,\"w\":7,\"h\":8,\"a\":3,\"b\":4,\"r\":0,\"c\":1000}"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "{\"type\":\"det_end\"}"));

    VisionDetections detections;
    TEST_ASSERT_TRUE(receiver.getDetections(detections));
    TEST_ASSERT_EQUAL_UINT8(1, detections.blobCount);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, detections.blobs[0].confidence);
    TEST_ASSERT_FALSE(detections.horizon.valid);
}

void test_open_mv_receiver_keeps_images_and_detections_separate(void)
{
    OpenMVReceiver receiver;

    feedCompleteImage(receiver, "image_01: payload bay centered");

    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "{\"type\":\"det_begin\",\"count\":1}"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver,
        "{\"type\":\"blob\",\"index\":0,\"x\":1,\"y\":2,\"w\":3,\"h\":4,\"a\":1,\"b\":2,\"r\":10,\"c\":250}"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "{\"type\":\"det_end\"}"));

    // the image queued before the detection frame is still intact
    TEST_ASSERT_EQUAL_UINT8(1, receiver.queueSize());

    String receivedImage = "";
    int receivedByteCount = 0;
    TEST_ASSERT_TRUE(receiver.getImage(receivedImage, receivedByteCount));
    TEST_ASSERT_EQUAL_STRING("image_01: payload bay centered", receivedImage.c_str());

    VisionDetections detections;
    TEST_ASSERT_TRUE(receiver.getDetections(detections));
    TEST_ASSERT_EQUAL_UINT8(1, detections.blobCount);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.250f, detections.blobs[0].confidence);
}

int main(int argc, char** argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_open_mv_receiver_queues_and_returns_images_in_order);
    RUN_TEST(test_open_mv_receiver_drops_oldest_images_when_queue_is_overloaded);
    RUN_TEST(test_open_mv_receiver_uses_img_begin_byte_count_metadata);
    RUN_TEST(test_open_mv_receiver_parses_detection_json_frame);
    RUN_TEST(test_open_mv_receiver_handles_detection_frame_without_horizon);
    RUN_TEST(test_open_mv_receiver_keeps_images_and_detections_separate);
    return UNITY_END();
}
