#ifndef _BaseDriver_hpp
#define _BaseDriver_hpp

#include <cstdint>

#include "utils/FakeAtomic.hpp"

class BaseDriver {
 public:
    struct WheelSpeed {
        double L1, L2, R1, R2;
        void setZero() { L1 = L2 = R1 = R2 = 0; }
    };
    struct BaseSpeed {
        double x, y, yaw;
    };

    struct CmdVelAcc {
        uint64_t timestamp_us;
        bool valid;
        BaseSpeed vel, acc;
    };

    class ControlState {
        int flag;

     public:
        ControlState() : flag(0) {}
        ControlState(int state) { set(state); }
        ControlState(bool L1, bool L2, bool R1, bool R2) { set(L1, L2, R1, R2); }
        inline void set(int state) { flag = state; }
        inline void set(bool L1, bool L2, bool R1, bool R2) {
            flag = ((int)L1) | ((int)L2 << 1) | ((int)R1 << 2) | ((int)R2 << 3);
        }
        ControlState& operator^=(const ControlState& other) {
            flag ^= other.flag;
            return *this;
        }
        ControlState operator^(const ControlState& other) { return ControlState(flag ^ other.flag); }
        inline bool L1() const { return flag & 1; }
        inline bool L2() const { return flag & 2; }
        inline bool R1() const { return flag & 4; }
        inline bool R2() const { return flag & 8; }
    };

 protected:
    FakeAtomicLoader<WheelSpeed> _wheelSpeedLoader;
    FakeAtomicLoader<ControlState> _controlStateLoader;
    FakeAtomicLoader<CmdVelAcc> _cmdVelAccLoader;
    WheelSpeed _wheelSpeed;
    BaseSpeed _baseSpeed;
    ControlState _controlState;
    CmdVelAcc _cmdVelAcc;

 public:
    // 车底盘中心到轮子中心的距离的分量, 单位是m
    static constexpr double r_x = 0.198 / 2, r_y = (0.21 - 0.0325) / 2, r_k = r_x + r_y + 0.001;

    static inline WheelSpeed calc_vel(double x, double y, double yaw) {
        WheelSpeed res;
        res.L1 = x - y - yaw * r_k;
        res.L2 = x + y - yaw * r_k;
        res.R1 = x + y + yaw * r_k;
        res.R2 = x - y + yaw * r_k;
        return res;
    }

    static inline BaseSpeed calc_vel(double L1, double L2, double R1, double R2) {
        BaseSpeed res;
        res.x = (L1 + L2 + R1 + R2) / 4;
        res.y = (-L1 + L2 + R1 - R2) / 4;
        res.yaw = (-L1 - L2 + R1 + R2) / (4 * r_k);
        return res;
    }

    inline void cmd_vel(double x, double y, double yaw) { _wheelSpeedLoader.store(calc_vel(x, y, yaw)); }
    inline void cmd_vel(const BaseSpeed& base) { cmd_vel(base.x, base.y, base.yaw); }
    inline void cmd_vel(double L1, double L2, double R1, double R2) { _wheelSpeedLoader.store({L1, L2, R1, R2}); }
    inline void cmd_vel(const WheelSpeed& wheel) { _wheelSpeedLoader.store(wheel); }

    inline void cmd_vel_acc(double vX, double vY, double vYaw, double aX, double aY, double aYaw, uint64_t timestamp_us) {
        _cmdVelAccLoader.store({timestamp_us, true, {vX, vY, vYaw}, {aX, aY, aYaw}});
    }

    inline void get_vel(double L1, double L2, double R1, double R2) { _baseSpeed = calc_vel(L1, L2, R1, R2); }
    inline void get_vel(WheelSpeed& wheel) { get_vel(wheel.L1, wheel.L2, wheel.R1, wheel.R2); }

    inline void setControlState(bool L1, bool L2, bool R1, bool R2) { _controlStateLoader.emplace(L1, L2, R1, R2); }
    inline void setControlState(int state) { _controlStateLoader.emplace(state); }
    inline void setControlState(const ControlState& state) { _controlStateLoader.store(state); }

    inline bool loadWheelSpeed() { return _wheelSpeedLoader.load(_wheelSpeed); }
    inline bool loadControlState() { return _controlStateLoader.load(_controlState); }
    inline bool loadCmdVelAcc() { return _cmdVelAccLoader.load(_cmdVelAcc); }

    inline const WheelSpeed& getWheelSpeed() const { return _wheelSpeed; }
    inline const BaseSpeed& getBaseSpeed() const { return _baseSpeed; }
    inline const ControlState& getControlState() const { return _controlState; }
    inline CmdVelAcc& getCmdVelAcc() { return _cmdVelAcc; }

    BaseDriver() {
        _cmdVelAcc.valid = false;
        cmd_vel(0, 0, 0);
        get_vel(0, 0, 0, 0);
        setControlState(0);
    }
};

#endif  // _BaseDriver_hpp