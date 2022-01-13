#ifndef _geometry_msgs_hpp
#define _geometry_msgs_hpp

#include "std_msgs.hpp"

namespace rosRT {
namespace msgs {

struct Quaternion {
    float x;
    float y;
    float z;
    float w;
};

struct QuaternionStamped {
    Header header;
    Quaternion quaternion;
};

struct Vector3 {
    float x;
    float y;
    float z;
};

struct Vector3Stamped {
    Header header;
    Vector3 vector;
};

}  // namespace msgs
}  // namespace rosRT

#endif  // _geometry_msgs_hpp