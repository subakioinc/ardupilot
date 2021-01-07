/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 *  ArduCopter (also known as APM, APM:Copter or just Copter)
 *  Wiki:           copter.ardupilot.org
 *  Creator:        Jason Short
 *  Lead Developer: Randy Mackay
 *  Lead Tester:    Marco Robustini
 *  Based on code and ideas from the Arducopter team: Leonard Hall, Andrew Tridgell, Robert Lefebvre, Pat Hickey, Michael Oborne, Jani Hirvinen,
                                                      Olivier Adler, Kevin Hester, Arthur Benemann, Jonathan Challinger, John Arne Birkeland,
                                                      Jean-Louis Naudin, Mike Smith, and more
 *  Thanks to: Chris Anderson, Jordi Munoz, Jason Short, Doug Weibel, Jose Julio
 *
 *  Special Thanks to contributors (in alphabetical order by first name):
 *
 *  Adam M Rivera       :Auto Compass Declination
 *  Amilcar Lucas       :Camera mount library
 *  Andrew Tridgell     :General development, Mavlink Support
 *  Angel Fernandez     :Alpha testing
 *  AndreasAntonopoulous:GeoFence
 *  Arthur Benemann     :DroidPlanner GCS
 *  Benjamin Pelletier  :Libraries
 *  Bill King           :Single Copter
 *  Christof Schmid     :Alpha testing
 *  Craig Elder         :Release Management, Support
 *  Dani Saez           :V Octo Support
 *  Doug Weibel         :DCM, Libraries, Control law advice
 *  Emile Castelnuovo   :VRBrain port, bug fixes
 *  Gregory Fletcher    :Camera mount orientation math
 *  Guntars             :Arming safety suggestion
 *  HappyKillmore       :Mavlink GCS
 *  Hein Hollander      :Octo Support, Heli Testing
 *  Igor van Airde      :Control Law optimization
 *  Jack Dunkle         :Alpha testing
 *  James Goppert       :Mavlink Support
 *  Jani Hiriven        :Testing feedback
 *  Jean-Louis Naudin   :Auto Landing
 *  John Arne Birkeland :PPM Encoder
 *  Jose Julio          :Stabilization Control laws, MPU6k driver
 *  Julien Dubois       :PosHold flight mode
 *  Julian Oes          :Pixhawk
 *  Jonathan Challinger :Inertial Navigation, CompassMot, Spin-When-Armed
 *  Kevin Hester        :Andropilot GCS
 *  Max Levine          :Tri Support, Graphics
 *  Leonard Hall        :Flight Dynamics, Throttle, Loiter and Navigation Controllers
 *  Marco Robustini     :Lead tester
 *  Michael Oborne      :Mission Planner GCS
 *  Mike Smith          :Pixhawk driver, coding support
 *  Olivier Adler       :PPM Encoder, piezo buzzer
 *  Pat Hickey          :Hardware Abstraction Layer (HAL)
 *  Robert Lefebvre     :Heli Support, Copter LEDs
 *  Roberto Navoni      :Library testing, Porting to VRBrain
 *  Sandro Benigno      :Camera support, MinimOSD
 *  Sandro Tognana      :PosHold flight mode
 *  Sebastian Quilter   :SmartRTL
 *  ..and many more.
 *
 *  Code commit statistics can be found here: https://github.com/ArduPilot/ardupilot/graphs/contributors
 *  Wiki: https://copter.ardupilot.org/
 *
 */

#include "Copter.h"

#define FORCE_VERSION_H_INCLUDE
#include "version.h"
#undef FORCE_VERSION_H_INCLUDE

const AP_HAL::HAL& hal = AP_HAL::get_HAL();

#define SCHED_TASK(func, rate_hz, max_time_micros) SCHED_TASK_CLASS(Copter, &copter, func, rate_hz, max_time_micros)

/*
  스케쥴러 table 배열로 정의. 특별히 빠른 freq로 돌아야하는 것은 fast_loop()에 구성
  구성 : function_pointer, hz, limit duration(이 시간내에 이 task는 끝나야한다!)
  scheduler table for fast CPUs - all regular tasks apart from the fast_loop()
  should be listed here, along with how often they should be called (in hz)
  and the maximum time they are expected to take (in microseconds)
 */
const AP_Scheduler::Task Copter::scheduler_tasks[] = {
    SCHED_TASK(rc_loop,              100,    130), // 조정기 읽기
    SCHED_TASK(throttle_loop,         50,     75), //  throttle 적용
    SCHED_TASK_CLASS(AP_GPS, &copter.gps, update, 50, 200),
#if OPTFLOW == ENABLED
    SCHED_TASK_CLASS(OpticalFlow,          &copter.optflow,             update,         200, 160),
#endif
    SCHED_TASK(update_batt_compass,   10,    120), // 배터리와 compass 업데이트 주기
    SCHED_TASK_CLASS(RC_Channels,          (RC_Channels*)&copter.g2.rc_channels,      read_aux_all,    10,     50),
    SCHED_TASK(arm_motors_check,      10,     50), // 모터 arming 검사
#if TOY_MODE_ENABLED == ENABLED
    SCHED_TASK_CLASS(ToyMode,              &copter.g2.toy_mode,         update,          10,  50),
#endif
    SCHED_TASK(auto_disarm_check,     10,     50), // 자동 disarm 검사
    SCHED_TASK(auto_trim,             10,     75), // auto_trim  : 기능 확인
#if RANGEFINDER_ENABLED == ENABLED
    SCHED_TASK(read_rangefinder,      20,    100), // 거리센서 읽기
#endif
#if PROXIMITY_ENABLED == ENABLED
    SCHED_TASK_CLASS(AP_Proximity,         &copter.g2.proximity,        update,         200,  50),
#endif
#if BEACON_ENABLED == ENABLED
    SCHED_TASK_CLASS(AP_Beacon,            &copter.g2.beacon,           update,         400,  50),
#endif
    SCHED_TASK(update_altitude,       10,    100), // 고도 update
    SCHED_TASK(run_nav_updates,       50,    100), // nav_update (?)
    SCHED_TASK(update_throttle_hover,100,     90), // throttle_hover update (?)
#if MODE_SMARTRTL_ENABLED == ENABLED
    SCHED_TASK_CLASS(ModeSmartRTL, &copter.mode_smartrtl,       save_position,    3, 100), //RTL 기능
#endif
#if SPRAYER_ENABLED == ENABLED
    SCHED_TASK_CLASS(AC_Sprayer,           &copter.sprayer,             update,           3,  90), // sprayer ??
#endif
    SCHED_TASK(three_hz_loop,          3,     75),
    SCHED_TASK_CLASS(AP_ServoRelayEvents,  &copter.ServoRelayEvents,      update_events, 50,     75),
    SCHED_TASK_CLASS(AP_Baro,              &copter.barometer,           accumulate,      50,  90),
#if AC_FENCE == ENABLED
    SCHED_TASK_CLASS(AC_Fence,             &copter.fence,               update,          10, 100),
#endif
#if PRECISION_LANDING == ENABLED
    SCHED_TASK(update_precland,      400,     50), // 정밀 착륙
#endif
#if FRAME_CONFIG == HELI_FRAME
    SCHED_TASK(check_dynamic_flight,  50,     75),
#endif
#if LOGGING_ENABLED == ENABLED
    SCHED_TASK(fourhundred_hz_logging,400,    50),
#endif
    SCHED_TASK_CLASS(AP_Notify,            &copter.notify,              update,          50,  90),
    SCHED_TASK(one_hz_loop,            1,    100),  // 1초에 1번씩 실행되는 loop
    SCHED_TASK(ekf_check,             10,     75),  // EKF 정상인지 체크??
    SCHED_TASK(check_vibration,       10,     50),  // 진동 체크??
    SCHED_TASK(gpsglitch_check,       10,     50),  // gps glitch 검사
    SCHED_TASK(landinggear_update,    10,     75),  // landinggear update
    SCHED_TASK(standby_update,        100,    75),  // standby_update 
    SCHED_TASK(lost_vehicle_check,    10,     50),  // lost_vehicle 검사
    SCHED_TASK_CLASS(GCS,                  (GCS*)&copter._gcs,          update_receive, 400, 180),  // gcs 수신
    SCHED_TASK_CLASS(GCS,                  (GCS*)&copter._gcs,          update_send,    400, 550),  // gcs 송신
#if HAL_MOUNT_ENABLED
    SCHED_TASK_CLASS(AP_Mount,             &copter.camera_mount,        update,          50,  75),
#endif
#if CAMERA == ENABLED
    SCHED_TASK_CLASS(AP_Camera,            &copter.camera,              update,          50,  75),
#endif
#if LOGGING_ENABLED == ENABLED
    SCHED_TASK(ten_hz_logging_loop,   10,    350), // 10hz loop
    SCHED_TASK(twentyfive_hz_logging, 25,    110), // 25hz loop 
    SCHED_TASK_CLASS(AP_Logger,      &copter.logger,           periodic_tasks, 400, 300),
#endif
    SCHED_TASK_CLASS(AP_InertialSensor,    &copter.ins,                 periodic,       400,  50), // 400hz로 ins 센서 읽기?

    SCHED_TASK_CLASS(AP_Scheduler,         &copter.scheduler,           update_logging, 0.1,  75), // log 업데이트??
#if RPM_ENABLED == ENABLED
    SCHED_TASK(rpm_update,            40,    200),  // rpm update
#endif
    SCHED_TASK(compass_cal_update,   100,    100),  // compass 계산 update 
    SCHED_TASK(accel_cal_update,      10,    100),  // 가속도 계산 update
    SCHED_TASK_CLASS(AP_TempCalibration,   &copter.g2.temp_calibration, update,          10, 100), //
#if HAL_ADSB_ENABLED
    SCHED_TASK(avoidance_adsb_update, 10,    100), //adsb 회피 
#endif
#if ADVANCED_FAILSAFE == ENABLED
    SCHED_TASK(afs_fs_check,          10,    100), // 고급 failsafe
#endif
#if AC_TERRAIN == ENABLED
    SCHED_TASK(terrain_update,        10,    100), // terrain 처리
#endif
#if GRIPPER_ENABLED == ENABLED
    SCHED_TASK_CLASS(AP_Gripper,           &copter.g2.gripper,          update,          10,  75),
#endif
#if WINCH_ENABLED == ENABLED
    SCHED_TASK_CLASS(AP_Winch,             &copter.g2.winch,            update,          50,  50),
#endif
#ifdef USERHOOK_FASTLOOP
    SCHED_TASK(userhook_FastLoop,    100,     75),
#endif
#ifdef USERHOOK_50HZLOOP
    SCHED_TASK(userhook_50Hz,         50,     75),
#endif
#ifdef USERHOOK_MEDIUMLOOP
    SCHED_TASK(userhook_MediumLoop,   10,     75),
#endif
#ifdef USERHOOK_SLOWLOOP
    SCHED_TASK(userhook_SlowLoop,     3.3,    75),
#endif
#ifdef USERHOOK_SUPERSLOWLOOP
    SCHED_TASK(userhook_SuperSlowLoop, 1,   75),
#endif
#if BUTTON_ENABLED == ENABLED
    SCHED_TASK_CLASS(AP_Button,            &copter.button,           update,           5, 100),
#endif
#if STATS_ENABLED == ENABLED
    SCHED_TASK_CLASS(AP_Stats,             &copt er.g2.stats,            update,           1, 100),
#endif
};

void Copter::get_scheduler_tasks(const AP_Scheduler::Task *&tasks,
                                 uint8_t &task_count,
                                 uint32_t &log_bit)
{
    tasks = &scheduler_tasks[0];
    task_count = ARRAY_SIZE(scheduler_tasks);
    log_bit = MASK_LOG_PM;
}

constexpr int8_t Copter::_failsafe_priorities[7];

// 400hz 도는 드론의 핵심 loop
// Main loop - 400hz
void Copter::fast_loop()
{
    // 최신 gyro 센서 data를 읽어오기
    // update INS immediately to get current gyro data populated
    ins.update();

    // IMU 데이터가 필요한 자세제어의 rate controller
    // run low level rate controllers that only require IMU data
    attitude_control->rate_controller_run();

    // rate control을 모터 출력으로 전송
    // send outputs to the motors library immediately
    motors_output();

    // EKF 상태 추정기 실행 (계산량이 많다!)
    // run EKF state estimator (expensive)
    // --------------------
    read_AHRS();

#if FRAME_CONFIG == HELI_FRAME
    update_heli_control_dynamics();
    #if MODE_AUTOROTATE_ENABLED == ENABLED
        heli_update_autorotation();
    #endif
#endif //HELI_FRAME

    // Inertia 읽기
    // Inertial Nav
    // --------------------
    read_inertia();

    // check if ekf has reset target heading or position
    check_ekf_reset();

    // 자세제어 실행
    // run the attitude controllers
    update_flight_mode();

    // EKF에서 home을 업데이트 (필요한 경우)
    // update home from EKF if necessary
    update_home_from_EKF();

    // 착륙 및 충돌 감지
    // check if we've landed or crashed
    update_land_and_crash_detectors();

#if HAL_MOUNT_ENABLED
    // camera mount's fast update
    camera_mount.update_fast();
#endif

    // log sensor health
    if (should_log(MASK_LOG_ANY)) {
        Log_Sensor_Health();
    }

    AP_Vehicle::fast_loop();
}

// start takeoff to given altitude (for use by scripting)
bool Copter::start_takeoff(float alt)
{
    // exit if vehicle is not in Guided mode or Auto-Guided mode
    if (!flightmode->in_guided_mode()) {
        return false;
    }

    if (mode_guided.do_user_takeoff_start(alt * 100.0f)) {
        copter.set_auto_armed(true);
        return true;
    }
    return false;
}

// set target location (for use by scripting)
bool Copter::set_target_location(const Location& target_loc)
{
    // exit if vehicle is not in Guided mode or Auto-Guided mode
    if (!flightmode->in_guided_mode()) {
        return false;
    }

    return mode_guided.set_destination(target_loc);
}

bool Copter::set_target_velocity_NED(const Vector3f& vel_ned)
{
    // exit if vehicle is not in Guided mode or Auto-Guided mode
    if (!flightmode->in_guided_mode()) {
        return false;
    }

    // convert vector to neu in cm
    const Vector3f vel_neu_cms(vel_ned.x * 100.0f, vel_ned.y * 100.0f, -vel_ned.z * 100.0f);
    mode_guided.set_velocity(vel_neu_cms);
    return true;
}

bool Copter::set_target_angle_and_climbrate(float roll_deg, float pitch_deg, float yaw_deg, float climb_rate_ms, bool use_yaw_rate, float yaw_rate_degs)
{
    // exit if vehicle is not in Guided mode or Auto-Guided mode
    if (!flightmode->in_guided_mode()) {
        return false;
    }

    Quaternion q;
    q.from_euler(radians(roll_deg),radians(pitch_deg),radians(yaw_deg));

    mode_guided.set_angle(q, climb_rate_ms*100, use_yaw_rate, radians(yaw_rate_degs), false);
    return true;
}


// RC 조정기에서 사용자 입력 수신
// rc_loops - reads user input from transmitter/receiver
// called at 100hz
void Copter::rc_loop()
{
    // RC조정기 읽기와 mode 스위치 읽기
    // Read radio and 3-position switch on radio
    // -----------------------------------------
    read_radio();
    rc().read_mode_switch();
}

// throttle_loop - should be run at 50 hz
// ---------------------------
void Copter::throttle_loop()
{
    // throttle mix 업데이트 ( throttle와 attitude 제어의 제어 우선순위)
    // update throttle_low_comp value (controls priority of throttle vs attitude control)
    update_throttle_mix();

    // auto_armed 상태 검사
    // check auto_armed status
    update_auto_armed();

#if FRAME_CONFIG == HELI_FRAME
    // update rotor speed
    heli_update_rotor_speed_targets();

    // update trad heli swash plate movement
    heli_update_landing_swash();
#endif

    // ground effect 보상
    // compensate for ground effect (if enabled)
    update_ground_effect_detector();
    update_ekf_terrain_height_stable();
}

// 배터리와 컴파스 센서 정보 읽기
// update_batt_compass - read battery and compass
// should be called at 10hz
void Copter::update_batt_compass(void)
{
    // 배터리부터 읽고 compass 읽기. 모터 간섭 보상에 사용할 수도 있으니.
    // read battery before compass because it may be used for motor interference compensation
    battery.read();

    if(AP::compass().enabled()) {
        // update compass with throttle value - used for compassmot
        compass.set_throttle(motors->get_throttle());
        compass.set_voltage(battery.voltage());
        compass.read();
    }
}

// 400hz로 로깅되는 내용. attitude, rate, pid
// Full rate logging of attitude, rate and pid loops
// should be run at 400hz
void Copter::fourhundred_hz_logging()
{
    if (should_log(MASK_LOG_ATTITUDE_FAST) && !copter.flightmode->logs_attitude()) {
        Log_Write_Attitude();
    }
}

// 10hz로 로깅되는 내용
// ten_hz_logging_loop
// should be run at 10hz
void Copter::ten_hz_logging_loop()
{
    // 고속 rate logging 설정하지 않은 경우 attitude data를 로깅
    // log attitude data if we're not already logging at the higher rate
    if (should_log(MASK_LOG_ATTITUDE_MED) && !should_log(MASK_LOG_ATTITUDE_FAST) && !copter.flightmode->logs_attitude()) {
        Log_Write_Attitude();
    }
    // EKF_POS 로깅하기
    // log EKF attitude data 
    if (should_log(MASK_LOG_ATTITUDE_MED) || should_log(MASK_LOG_ATTITUDE_FAST)) {
        Log_Write_EKF_POS();
    }
    if (should_log(MASK_LOG_MOTBATT)) { // 모터와 배터리 로깅
        Log_Write_MotBatt();
    }
    if (should_log(MASK_LOG_RCIN)) { // RC조정기 입력 로깅
        logger.Write_RCIN();
        if (rssi.enabled()) {
            logger.Write_RSSI();
        }
    }
    if (should_log(MASK_LOG_RCOUT)) { // RC출력 로깅
        logger.Write_RCOUT();
    }
    if (should_log(MASK_LOG_NTUN) && (flightmode->requires_GPS() || landing_with_GPS())) { // position 제어 로깅
        pos_control->write_log();
    }
    if (should_log(MASK_LOG_IMU) || should_log(MASK_LOG_IMU_FAST) || should_log(MASK_LOG_IMU_RAW)) {  // 진동 로깅
        logger.Write_Vibration();
    }
    if (should_log(MASK_LOG_CTUN)) { // 자세제어 로깅
        attitude_control->control_monitor_log();
#if PROXIMITY_ENABLED == ENABLED
        logger.Write_Proximity(g2.proximity);  // Write proximity sensor distances
#endif
#if BEACON_ENABLED == ENABLED
        logger.Write_Beacon(g2.beacon);
#endif
    }
#if FRAME_CONFIG == HELI_FRAME
    Log_Write_Heli();
#endif
#if WINCH_ENABLED == ENABLED
    if (should_log(MASK_LOG_ANY)) {
        g2.winch.write_log();
    }
#endif
}

// twentyfive_hz_logging - should be run at 25hz
void Copter::twentyfive_hz_logging()
{
#if HIL_MODE != HIL_MODE_DISABLED
    // HIL for a copter needs very fast update of the servo values
    gcs().send_message(MSG_SERVO_OUTPUT_RAW);
#endif

#if HIL_MODE == HIL_MODE_DISABLED
    if (should_log(MASK_LOG_ATTITUDE_FAST)) {
        Log_Write_EKF_POS();
    }

    if (should_log(MASK_LOG_IMU)) {
        logger.Write_IMU();
    }
#endif

#if PRECISION_LANDING == ENABLED
    // log output
    Log_Write_Precland();
#endif

#if MODE_AUTOROTATE_ENABLED == ENABLED
    if (should_log(MASK_LOG_ATTITUDE_MED) || should_log(MASK_LOG_ATTITUDE_FAST)) {
        //update autorotation log
        g2.arot.Log_Write_Autorotation();
    }
#endif
}

// 3.3hz로 도는 것들
// three_hz_loop - 3.3hz loop
void Copter::three_hz_loop()
{
    // GCS와 연결이 끊어졌는지 검사
    // check if we've lost contact with the ground station
    failsafe_gcs_check();

    // terrain 데이터를 더 이상 받지 못하는지 체크
    // check if we've lost terrain data
    failsafe_terrain_check();

#if AC_FENCE == ENABLED
    // fence를 넘어갔는지 검사
    // check if we have breached a fence
    fence_check();
#endif // AC_FENCE_ENABLED

    // 비행 튜닝?
    // update ch6 in flight tuning
    tuning();
}

// 1초에 1회 
// one_hz_loop - runs at 1Hz
void Copter::one_hz_loop()
{
    if (should_log(MASK_LOG_ANY)) {
        Log_Write_Data(LogDataID::AP_STATE, ap.value);
    }

    // arming 상태 확인
    arming.update();

    if (!motors->armed()) {
        // 초기 설정시에 실시간으로 ahrs 방향 변경이 가능하도록
        // make it possible to change ahrs orientation at runtime during initial config
        ahrs.update_orientation();

        update_using_interlock();

        // 사용자가 frame class와 type를 업데이트 하지 않았는지 검사
        // check the user hasn't updated the frame class or type
        motors->set_frame_class_and_type((AP_Motors::motor_frame_class)g2.frame_class.get(), (AP_Motors::motor_frame_type)g.frame_type.get());

#if FRAME_CONFIG != HELI_FRAME
        // 모든 throttle 채널 설정들을 설정
        // set all throttle channel settings
        motors->set_throttle_range(channel_throttle->get_radio_min(), channel_throttle->get_radio_max());
#endif
    }

    // 할당된 기능을 업데이트 및 추가 servo 활성화
    // update assigned functions and enable auxiliary servos
    SRV_Channels::enable_aux_servos();

    // terrain 데이터 로깅
    // log terrain data
    terrain_logging();

#if HAL_ADSB_ENABLED
    adsb.set_is_flying(!ap.land_complete);
#endif

    AP_Notify::flags.flying = !ap.land_complete;
}

void Copter::init_simple_bearing()
{
    // 현재 cos_yaw와 sin_yaw 값을 캡쳐
    // capture current cos_yaw and sin_yaw values
    simple_cos_yaw = ahrs.cos_yaw();
    simple_sin_yaw = ahrs.sin_yaw();

    // simple mode heading으로부터 180까지 super simple heading을 초기화
    // initialise super simple heading (i.e. heading towards home) to be 180 deg from simple mode heading
    super_simple_last_bearing = wrap_360_cd(ahrs.yaw_sensor+18000);
    super_simple_cos_yaw = simple_cos_yaw;
    super_simple_sin_yaw = simple_sin_yaw;

    // simple bearing을 로깅
    // log the simple bearing
    if (should_log(MASK_LOG_ANY)) {
        Log_Write_Data(LogDataID::INIT_SIMPLE_BEARING, ahrs.yaw_sensor);
    }
}
// simple 및 super simple mode : https://ardupilot.org/copter/docs/simpleandsuper-simple-modes.html
// 조정자의 관점에서 비행체의 heading이 결정! - 초보자에게 적합
// simple_mode를 업데이트 - simple mode에 있는 경우 조종사 입력을 로테이션
// update_simple_mode - rotates pilot input if we are in simple mode
void Copter::update_simple_mode(void)
{
    float rollx, pitchx;

    // 새로운 radio frame이 없거나 simple mode가 아니면 즉시 빠져나감
    // exit immediately if no new radio frame or not in simple mode
    if (simple_mode == SimpleMode::NONE || !ap.new_radio_frame) {
        return;
    }

    // mark radio frame as consumed
    ap.new_radio_frame = false;

    if (simple_mode == SimpleMode::SIMPLE) {
        // rotate roll, pitch input by -initial simple heading (i.e. north facing)
        rollx = channel_roll->get_control_in()*simple_cos_yaw - channel_pitch->get_control_in()*simple_sin_yaw;
        pitchx = channel_roll->get_control_in()*simple_sin_yaw + channel_pitch->get_control_in()*simple_cos_yaw;
    }else{
        // rotate roll, pitch input by -super simple heading (reverse of heading to home)
        rollx = channel_roll->get_control_in()*super_simple_cos_yaw - channel_pitch->get_control_in()*super_simple_sin_yaw;
        pitchx = channel_roll->get_control_in()*super_simple_sin_yaw + channel_pitch->get_control_in()*super_simple_cos_yaw;
    }

    // rotate roll, pitch input from north facing to vehicle's perspective
    channel_roll->set_control_in(rollx*ahrs.cos_yaw() + pitchx*ahrs.sin_yaw());
    channel_pitch->set_control_in(-rollx*ahrs.sin_yaw() + pitchx*ahrs.cos_yaw());
}

// update_super_simple_bearing - adjusts simple bearing based on location
// should be called after home_bearing has been updated
void Copter::update_super_simple_bearing(bool force_update)
{
    if (!force_update) {
        if (simple_mode != SimpleMode::SUPERSIMPLE) {
            return;
        }
        if (home_distance() < SUPER_SIMPLE_RADIUS) {
            return;
        }
    }

    const int32_t bearing = home_bearing();

    // check the bearing to home has changed by at least 5 degrees
    if (labs(super_simple_last_bearing - bearing) < 500) {
        return;
    }

    super_simple_last_bearing = bearing;
    const float angle_rad = radians((super_simple_last_bearing+18000)/100);
    super_simple_cos_yaw = cosf(angle_rad);
    super_simple_sin_yaw = sinf(angle_rad);
}

void Copter::read_AHRS(void)
{
    // AHRS (Attitude Heading Reference System) : 비행체의 XYZ와 heading 방향. IMU 기반으로 비행체 xyz와 heading 추정.
    // Perform IMU calculations and get attitude info
    //-----------------------------------------------
#if HIL_MODE != HIL_MODE_DISABLED
    // update hil before ahrs update
    gcs().update();
#endif

    // fast_loop()에서 이미 INS update를 수행했으면 AHRS한테 skip하라고 하는 설정. 인자로 true를 넘김
    // we tell AHRS to skip INS update as we have already done it in fast_loop()
    ahrs.update(true);
}

// 기압센서 읽기 control tuning을 로깅하기 
// read baro and log control tuning
void Copter::update_altitude()
{
    // read in baro altitude
    read_barometer();

    if (should_log(MASK_LOG_CTUN)) {
        Log_Write_Control_Tuning();
#if HAL_GYROFFT_ENABLED
        gyro_fft.write_log_messages();
#else
        write_notch_log_messages();
#endif
    }
}

// waypoint 관련 helper 
// vehicle specific waypoint info helpers
bool Copter::get_wp_distance_m(float &distance) const
{
    // see GCS_MAVLINK_Copter::send_nav_controller_output()
    distance = flightmode->wp_distance() * 0.01;
    return true;
}

// waypoint 관련 helper 
// vehicle specific waypoint info helpers
bool Copter::get_wp_bearing_deg(float &bearing) const
{
    // see GCS_MAVLINK_Copter::send_nav_controller_output()
    bearing = flightmode->wp_bearing() * 0.01;
    return true;
}

// waypoint 관련 helper 
// vehicle specific waypoint info helpers
bool Copter::get_wp_crosstrack_error_m(float &xtrack_error) const
{
    // see GCS_MAVLINK_Copter::send_nav_controller_output()
    xtrack_error = flightmode->crosstrack_error() * 0.01;
    return true;
}

/*
  constructor for main Copter class
 */
Copter::Copter(void)
    : logger(g.log_bitmask),
    flight_modes(&g.flight_mode1),
    control_mode(Mode::Number::STABILIZE),
    simple_cos_yaw(1.0f),
    super_simple_cos_yaw(1.0),
    land_accel_ef_filter(LAND_DETECTOR_ACCEL_LPF_CUTOFF),
    rc_throttle_control_in_filter(1.0f),
    inertial_nav(ahrs),
    param_loader(var_info),
    flightmode(&mode_stabilize)
{
    // init sensor error logging flags
    sensor_health.baro = true;
    sensor_health.compass = true;
}

Copter copter;
AP_Vehicle& vehicle = copter;

AP_HAL_MAIN_CALLBACKS(&copter);
