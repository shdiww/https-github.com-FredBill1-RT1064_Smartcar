extern "C" {
#include "headfile.h"
int main(void);
}

#include <cmath>
#include <limits>

#include "Thread.h"
#include "devices.hpp"
#include "nodes/nodes.hpp"
#include "rosRT/Topic.hpp"

//
#include "apriltag/apriltag.hpp"
#include "apriltag/apriltag_pose.hpp"
#include "apriltag/internal/Quick_decode.hpp"
#include "apriltag/internal/UnionBuffer.hpp"
#include "apriltag/internal/decode_quad.hpp"
#include "apriltag/internal/fit_quad.hpp"
#include "apriltag/internal/reconcile_detections.hpp"
#include "apriltag/internal/segmentation.hpp"
#include "apriltag/internal/threshold.hpp"
#include "apriltag/tag25h9.hpp"
#include "apriltag/visualization.hpp"

void rotCB(const rosRT::msgs::QuaternionStamped& data) {
    wireless.writeV(data.quaternion.x, data.quaternion.y, data.quaternion.z, data.quaternion.w, data.header.stamp);
    wireless.sendTail();
}
auto rot = rosRT::Subscriber::create<rosRT::msgs::QuaternionStamped>("imu/6DOF_orientation", 1, rotCB);

void imgThreadEntry(void*) {
    using namespace imgProc::apriltag;
    AT_SDRAM_NONCACHE_SECTION_ALIGN(static uint8_t p[N][M], 64);
    AT_SDRAM_SECTION_ALIGN(static QuadImg_t binary, 64);
    auto tf = tag25h9_create();
    tf.init(1);
    auto pre = rt_tick_get_millisecond();
    apriltag_detection_info info{nullptr, 10, 652.3019409179688, 653.458251953125, 312.8074714771901, 230.0874313802706};
    apriltag_pose solution;
    gpio_init(D4, GPI, 0, GPIO_PIN_CONFIG);
    gpio_init(D27, GPI, 0, GPIO_PIN_CONFIG);
    for (;;) {
        mt9v03x_csi_image_take(p[0]);
        // show_grayscale(p[0]);
        threshold(p[0], binary);
        show_threshim(binary);
        unionfind_connected(binary);
        // show_unionfind();
        auto clusters = gradient_clusters(binary);
        // show_clusters(*clusters);

        auto quads = fit_quads(*clusters, tf, p[0], true);

        // show_clusters(*clusters);
        // show_quads(*quads);
        // PRINTF("%d\r\n", std::distance(quads->begin(), quads->end()));

        auto& detections = *decode_quads(tf, p[0], *quads);
        reconcile_detections(detections);
        // ips114_displayimage032(p[0], MT9V03X_CSI_W, MT9V03X_CSI_H);  //显示摄像头图像
        // PRINTF("cnt: %d\r\n", std::distance(detections.begin(), detections.end()));

        for (auto det_p : detections) {
            auto& det = *det_p;
            // PRINTF("id: %d\r\nhanmming: %d, decision_margin: %f\r\n", det.id, det.hamming, det.decision_margin);
            uint64_t color = 2333;
            // PRINTF("center: x=%f y=%f\r\n", det.c[0], det.c[1]);
            plot(det.c[1], det.c[0], color & 0xFFFF);
            for (int i = 0; i < 4; i++) {
                color *= int(1e9 + 7);
                // PRINTF("p%d: x=%f y=%f\r\n", i, det.p[i][0], det.p[i][1]);
                plot(det.p[i][1], det.p[i][0], color & 0xFFFF);
            }
            info.det = det_p;
            estimate_pose_for_tag_homography(info, solution);
            auto& R = solution.R;
            // float xr = std::atan2(R[2][1], R[2][2]);
            // float yr = std::atan2(-R[2][0], std::sqrt(R[2][1] * R[2][1] + R[2][2] * R[2][2]));
            // float zr = std::atan2(R[1][0], R[0][0]);
            // wireless.writeV(xr, yr, zr);
            // wireless.sendTail();
            PRINTF("x:%f y:%f z:%f\r\n", solution.t[0], solution.t[1], solution.t[2]);
        }

        PRINTF("\r\n");

        auto cur = rt_tick_get_millisecond();
        // PRINTF("time:%d\r\n\r\n", cur - pre);
        pre = cur;
        // if (staticBuffer.overflow()) PRINTF("overflowed\r\n");
        // else
        //     PRINTF("used: %dB %dKB %dMB\r\n", staticBuffer.usage(), staticBuffer.usage() >> 10, staticBuffer.usage() >>
        //     20);
        // while (gpio_get(C4)) {}
        // while (!gpio_get(C4)) {}
        while (gpio_get(D4) && gpio_get(C4)) {}
        // rt_thread_mdelay(100);
    }
}

int main(void) {
    gpio_init(B9, GPO, 0, GPIO_PIN_CONFIG);
    rt_thread_mdelay(500);

    ips114_init();  //初始化1.14寸IPS屏幕
    ips114_showstr(0, 0, "SEEKFREE MT9V03x");
    ips114_showstr(0, 1, "Initializing... ");

    //如果屏幕没有任何显示，请检查屏幕接线

    mt9v03x_csi_init();  //初始化摄像头 使用CSI接口
    //如果屏幕一直显示初始化信息，请检查摄像头接线
    //如果使用主板，一直卡在while(!uart_receive_flag)，请检查是否电池连接OK?
    //如果图像只采集一次，请检查场信号(VSY)是否连接OK?

    ips114_showstr(0, 1, "      OK...     ");
    systick_delay_ms(500);

    // wireless.init("Wireless", UART8_CONFIG);
    // initDevices();
    // wirelessThread.start();
    EnableGlobalIRQ(0);
    // testtest::pose_kalman_test(NULL);

    rtthread::Thread imgThread(imgThreadEntry, NULL, 4096, 1, 1000, "imgThread");
    imgThread.start();

    for (;;) {
        // wireless.waitHeader();
        // if (!wireless.readF(duty)) { ips.printf("Failed to read\n"); }
        // ips.printf("%d\n", duty);
        gpio_toggle(B9);
        rt_thread_mdelay(500);
    }
}
