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

int main(int argc, char** argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_open_mv_receiver_queues_and_returns_images_in_order);
    RUN_TEST(test_open_mv_receiver_drops_oldest_images_when_queue_is_overloaded);
    RUN_TEST(test_open_mv_receiver_uses_img_begin_byte_count_metadata);
    return UNITY_END();
}
