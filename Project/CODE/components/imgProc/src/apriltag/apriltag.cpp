#include "apriltag/apriltag.hpp"

#include "apriltag/internal/Quick_decode.hpp"
#include "apriltag/internal/StaticBuffer.hpp"
#include "apriltag/internal/UnionBuffer.hpp"
#include "apriltag/internal/decode_quad.hpp"
#include "apriltag/internal/fit_quad.hpp"
#include "apriltag/internal/reconcile_detections.hpp"
#include "apriltag/internal/segmentation.hpp"
#include "apriltag/internal/threshold.hpp"
#include "apriltag/visualization.hpp"

extern "C" {
#include "common.h"
}

namespace imgProc {
namespace apriltag {

void apriltag_family::init(int maxhamming, bool static_allocate) { quick_decode::init(*this, maxhamming, static_allocate); }

detections_t &apriltag_detect(apriltag_family &tf, uint8_t *img, apriltag_detect_visualize_flag visualize_flag) {
    staticBuffer.reset();

    // show_grayscale(img);
    threshold(img, threshim);
    if (visualize_flag == apriltag_detect_visualize_flag::threshim) show_threshim(threshim);

    unionfind_connected(threshim);
    if (visualize_flag == apriltag_detect_visualize_flag::unionfind) show_unionfind();

    auto &clusters = *gradient_clusters(threshim);
    if (visualize_flag == apriltag_detect_visualize_flag::clusters) show_clustersImg(img, clusters);

    auto &quads = *fit_quads(
        clusters, tf, img,
        visualize_flag == apriltag_detect_visualize_flag::quads || visualize_flag == apriltag_detect_visualize_flag::decode);
    if (visualize_flag == apriltag_detect_visualize_flag::quads || visualize_flag == apriltag_detect_visualize_flag::decode) {
        show_clustersImg(img, clusters);
        show_quadsImg(img, quads);
    }

    auto &detections = *decode_quads(tf, img, quads, visualize_flag == apriltag_detect_visualize_flag::decode);
    reconcile_detections(detections);

    return detections;
}

}  // namespace apriltag
}  // namespace imgProc