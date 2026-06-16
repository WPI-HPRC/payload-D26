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

static void feedCompleteML(OpenMVReceiver& receiver)
{
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_BEGIN 42 2"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_HORIZON 1 77 0 77 159 79"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_BLOB 0 20 90 123 20 90 8 5 0"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_BLOB 1 100 95 250 100 95 12 7 3"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_END"));
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

void test_open_mv_receiver_parses_ml_result(void)
{
    OpenMVReceiver receiver;
    OpenMVMLData mlData = {};

    feedCompleteML(receiver);

    TEST_ASSERT_TRUE(receiver.hasMLResult());
    TEST_ASSERT_TRUE(receiver.getMLResult(mlData));
    TEST_ASSERT_FALSE(receiver.hasMLResult());

    TEST_ASSERT_EQUAL_UINT32(42, mlData.frameId);
    TEST_ASSERT_TRUE(mlData.horizonValid);
    TEST_ASSERT_EQUAL_INT16(77, mlData.horizonYPx);
    TEST_ASSERT_EQUAL_INT16(0, mlData.horizonX1);
    TEST_ASSERT_EQUAL_INT16(77, mlData.horizonY1);
    TEST_ASSERT_EQUAL_INT16(159, mlData.horizonX2);
    TEST_ASSERT_EQUAL_INT16(79, mlData.horizonY2);
    TEST_ASSERT_EQUAL_UINT8(2, mlData.expectedBlobCount);
    TEST_ASSERT_EQUAL_UINT8(2, mlData.blobCount);
    TEST_ASSERT_FALSE(mlData.droppedBlobs);

    TEST_ASSERT_EQUAL_INT16(20, mlData.blobs[0].cx);
    TEST_ASSERT_EQUAL_INT16(90, mlData.blobs[0].cy);
    TEST_ASSERT_EQUAL_UINT16(123, mlData.blobs[0].pixels);
    TEST_ASSERT_TRUE(mlData.blobs[0].hasEllipse);
    TEST_ASSERT_EQUAL_INT16(8, mlData.blobs[0].ellipseRx);

    TEST_ASSERT_EQUAL_INT16(100, mlData.blobs[1].cx);
    TEST_ASSERT_EQUAL_UINT16(250, mlData.blobs[1].pixels);
}

void test_open_mv_receiver_handles_ml_then_image_independently(void)
{
    OpenMVReceiver receiver;
    OpenMVMLData mlData = {};
    String receivedImage = "";
    int receivedByteCount = 0;

    feedCompleteML(receiver);
    feedCompleteImage(receiver, "image_after_ml");

    TEST_ASSERT_TRUE(receiver.getMLResult(mlData));
    TEST_ASSERT_EQUAL_UINT32(42, mlData.frameId);

    TEST_ASSERT_EQUAL_UINT8(1, receiver.queueSize());
    TEST_ASSERT_TRUE(receiver.getImage(receivedImage, receivedByteCount));
    TEST_ASSERT_EQUAL_STRING("image_after_ml", receivedImage.c_str());
    TEST_ASSERT_EQUAL(14, receivedByteCount);
}

void test_open_mv_receiver_ignores_cfg_and_dbg_outside_image_framing(void)
{
    OpenMVReceiver receiver;
    String receivedImage = "";
    int receivedByteCount = 0;

    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "CFG IMG=1 ML=1 DBG=0"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "DBG_OPENMV_RATE fps=1.00"));
    feedCompleteImage(receiver, "clean_image");

    TEST_ASSERT_EQUAL_UINT8(1, receiver.queueSize());
    TEST_ASSERT_TRUE(receiver.getImage(receivedImage, receivedByteCount));
    TEST_ASSERT_EQUAL_STRING("clean_image", receivedImage.c_str());
    TEST_ASSERT_EQUAL(11, receivedByteCount);
}

void test_open_mv_receiver_drops_malformed_ml_without_corrupting_images(void)
{
    OpenMVReceiver receiver;
    String receivedImage = "";
    int receivedByteCount = 0;

    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_BEGIN 50 1"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_HORIZON 1 60 0 60 159 61"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_BLOB bad"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_END"));
    TEST_ASSERT_FALSE(receiver.hasMLResult());

    feedCompleteImage(receiver, "image_after_bad_ml");

    TEST_ASSERT_TRUE(receiver.getImage(receivedImage, receivedByteCount));
    TEST_ASSERT_EQUAL_STRING("image_after_bad_ml", receivedImage.c_str());
    TEST_ASSERT_EQUAL(18, receivedByteCount);
}

void test_open_mv_receiver_flags_extra_ml_blobs(void)
{
    OpenMVReceiver receiver;
    OpenMVMLData mlData = {};

    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_BEGIN 99 18"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_HORIZON 0 80 0 80 159 80"));

    for (int i = 0; i < 18; i++) {
        String line = "ML_BLOB ";
        line += String(i);
        line += " 10 20 30";
        TEST_ASSERT_FALSE(feedReceiverChunk(receiver, line));
    }

    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_END"));
    TEST_ASSERT_TRUE(receiver.getMLResult(mlData));
    TEST_ASSERT_EQUAL_UINT32(99, mlData.frameId);
    TEST_ASSERT_EQUAL_UINT8(16, mlData.blobCount);
    TEST_ASSERT_TRUE(mlData.droppedBlobs);
}

void test_open_mv_receiver_does_not_publish_ml_without_end(void)
{
    OpenMVReceiver receiver;

    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_BEGIN 101 1"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_HORIZON 1 55 0 55 159 55"));
    TEST_ASSERT_FALSE(feedReceiverChunk(receiver, "ML_BLOB 0 30 40 50"));
    TEST_ASSERT_FALSE(receiver.hasMLResult());
}

int main(int argc, char** argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_open_mv_receiver_queues_and_returns_images_in_order);
    RUN_TEST(test_open_mv_receiver_drops_oldest_images_when_queue_is_overloaded);
    RUN_TEST(test_open_mv_receiver_uses_img_begin_byte_count_metadata);
    RUN_TEST(test_open_mv_receiver_parses_ml_result);
    RUN_TEST(test_open_mv_receiver_handles_ml_then_image_independently);
    RUN_TEST(test_open_mv_receiver_ignores_cfg_and_dbg_outside_image_framing);
    RUN_TEST(test_open_mv_receiver_drops_malformed_ml_without_corrupting_images);
    RUN_TEST(test_open_mv_receiver_flags_extra_ml_blobs);
    RUN_TEST(test_open_mv_receiver_does_not_publish_ml_without_end);
    return UNITY_END();
}
