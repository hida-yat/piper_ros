#include <memory>
#include "sensor_msgs/msg/joy.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include <functional>
#include <chrono>

using std::placeholders::_1;
using namespace std::chrono_literals;

class PiperController : public rclcpp::Node 
{
public:
  PiperController()
  : Node("piper_controller")
  {
    publisher_ = this->create_publisher<sensor_msgs::msg::JointState>(
      "joint_states", 10
    );
    subscription_ = this->create_subscription<sensor_msgs::msg::Joy>(
      "joy", 100, std::bind(&PiperController::joy_callback, this, _1)
    );
    timer_ = this->create_wall_timer(50ms, std::bind(&PiperController::timer_callback, this));

    joint_state_msg_.name = {"joint1", "joint2", "joint3", "joint4", "joint5", "joint6", "joint7", "joint8"}; // Replace with your actual joint names
    joint_state_msg_.position.resize(joint_state_msg_.name.size(), 0.0);
    joint_state_msg_.velocity.resize(joint_state_msg_.name.size(), 0.0);
    joint_state_msg_.effort.resize(joint_state_msg_.name.size(), 0.0);
    
    for (size_t i = 0; i < joint_state_msg_.position.size(); ++i) {
      joint_state_msg_.position[i] = joint_angle_[i];
    }
  }
  
private:
  void timer_callback(){

    joint_state_msg_.header.stamp = this->now();

    const double DEADZONE = 0.2;
    double speed = 0.04;
    static bool current_button2_state = false;
    static bool previous_button2_state = false;
    static bool is_gripping = false;

    // 左スティックの左右でジョイント1の回転を制御
    if(joy_msg_.axes[0] > DEADZONE){
     joint_angle_[0] = joint_angle_[0] - speed;
      if (joint_angle_[0] < -2.618) joint_angle_[0] = -2.618;
    }
    else if (joy_msg_.axes[0] < -DEADZONE){
      joint_angle_[0] = joint_angle_[0] + speed;
      if (joint_angle_[0] > 2.168) joint_angle_[0] = 2.168;
    }

    // 右スティックの前後でジョイント2の開閉を制御
    if(joy_msg_.axes[4] > DEADZONE){
      joint_angle_[1] = joint_angle_[1] + speed;
      if (joint_angle_[1] > 3.14) joint_angle_[1] = 3.14;
    }
    else if(joy_msg_.axes[4] < -DEADZONE){
      joint_angle_[1] = joint_angle_[1] - speed;
      if (joint_angle_[1] < 0) joint_angle_[1] = 0;
    }

    // 十字スティックの上下でジョイント3の開閉を制御. xボタン（△ボタン）同時押しでジョイント5の開閉を制御
    if(joy_msg_.axes[7] == 1){
      if(joy_msg_.buttons[2]){
        joint_angle_[4] -= speed;
        if(joint_angle_[4] < -1.220) joint_angle_[4] = -1.220;
      }
      else{
        joint_angle_[2] -= speed;
        if(joint_angle_[2] < -2.967) joint_angle_[2] = -2.967;
      }
    }
    else if(joy_msg_.axes[7] == -1){
      if(joy_msg_.buttons[2]){
        joint_angle_[4] += speed;
        if(joint_angle_[4] > 1.220) joint_angle_[4] = 1.220;
      }
      else{
        joint_angle_[2] += speed;
        if(joint_angle_[2] > 0) joint_angle_[2] = 0;
      }
    }

    // 十字スティックの左右でジョイント4の回転を制御．xボタン（△ボタン）同時押しでジョイント6の回転を制御
    if(joy_msg_.axes[6] == 1){
      if(joy_msg_.buttons[2]){
       joint_angle_[5] -= speed;
        if(joint_angle_[5] < -2.094) joint_angle_[5] = -2.094;
      }
      else{
        joint_angle_[3] -= speed;
        if(joint_angle_[3] < -1.745) joint_angle_[3] = -1.745;
      }
    }
    else if(joy_msg_.axes[6] == -1){
      if(joy_msg_.buttons[2]){
        joint_angle_[5] += speed;
        if(joint_angle_[5] > 2.094) joint_angle_[5] = 2.094;
      }
      else{
        joint_angle_[3] += speed;
        if(joint_angle_[3] > 1.745) joint_angle_[3] = 1.745;
      }
    }

    // Aボタン（○ボタン）でハンドを制御する. 押すたびに開閉が切り替わる
    if(joy_msg_.buttons[0]){
      current_button2_state = true;
    }
    else{
      current_button2_state = false;
    }
    // 過去と現在のボタン情報から，ボタンが押されたタイミングを検知
    if(previous_button2_state == false && current_button2_state == true){
      is_gripping = !is_gripping;
    }
    if(is_gripping){
      joint_angle_[6] = 0;
      joint_angle_[7] = 0;
    }
    else{
      joint_angle_[6] = 0.035;
      joint_angle_[7] = -0.035;
    }
    // 現在のボタン２の押下状態をprevious_button2_stateに格納
    previous_button2_state = current_button2_state;

    // ゼロポイントへリセットするコマンド
    if(joy_msg_.buttons[7] == 1){
      for (size_t i = 0; i < joint_state_msg_.position.size(); ++i) {
        joint_angle_[i] = 0;
      }
    }

    // ボタン3で物を掴む体勢に
    if(joy_msg_.buttons[1]) {
      joint_angle_[1] = 0.950;
      joint_angle_[2] = -1.315;
      joint_angle_[4] = 1.220;
    } 

    for (size_t i = 0; i < joint_state_msg_.position.size(); ++i) {
      joint_state_msg_.position[i] = joint_angle_[i];
    }
    publisher_->publish(joint_state_msg_);

    RCLCPP_INFO(this->get_logger(), "Joy callback at: %.3f", this->now().seconds());
  }

  void joy_callback(const sensor_msgs::msg::Joy::SharedPtr msg)
  {
    joy_msg_ = *msg;
  }


  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr publisher_;
  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr subscription_;
  sensor_msgs::msg::JointState joint_state_msg_;
  sensor_msgs::msg::Joy joy_msg_;
  //各jointの現在角度を表す変数joint_angle_を宣言
  double joint_angle_[8] = {0, 0, 0, 0, 0, 0, 0.035, -0.035};

};

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<PiperController>();
    rclcpp::spin(node);
    node.reset(); 
    rclcpp::shutdown();
    return 0;
}
