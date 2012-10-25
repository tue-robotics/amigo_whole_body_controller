#include "ObstacleAvoidance.h"
#include <tf/transform_datatypes.h>

#include "visualization_msgs/MarkerArray.h"

using namespace std;

ObstacleAvoidance::ObstacleAvoidance() {

}

ObstacleAvoidance::~ObstacleAvoidance() {
    for(vector<Box*>::iterator it = boxes_.begin(); it != boxes_.end(); ++it) {
        delete *it;
    }
}

bool ObstacleAvoidance::initialize(const std::string& end_effector_frame, uint F_start_index) {

    ROS_INFO("Initializing Obstacle Avoidance");

    // Get node handle
    ros::NodeHandle n("~");
    std::string ns = ros::this_node::getName();    

    end_effector_frame_ = end_effector_frame;
    F_start_index_ = F_start_index;

    bool wait_for_transform = listener_.waitForTransform(end_effector_frame_,"/map", ros::Time::now(), ros::Duration(1.0));
    if (!wait_for_transform) ROS_WARN("Transform between %s and /map is not available",end_effector_frame.c_str());

    pub_marker_ = n.advertise<visualization_msgs::MarkerArray>("/whole_body_controller/environment_markers", 10);

    // initialize environment
    boxes_.push_back(new Box(Eigen::Vector3d(-0.127, 0.730, 0), Eigen::Vector3d(1.151, 1.139, 0.7)));
    boxes_.push_back(new Box(Eigen::Vector3d( 1.032, -0.942, 0), Eigen::Vector3d(1.590, 0.140, 0.5)));

    ROS_INFO("Initialized Obstacle Avoidance");

    return true;
}

void ObstacleAvoidance::update(Eigen::VectorXd& F_task, uint& force_vector_index) {

    for(unsigned int i = 0; i < 3; ++i) {
        double sign = 1;
        if (F_task[i] < 0) sign = -1;
        F_task[i] = min(abs(F_task[i]), 1.0) * sign;
    }

    tf::Stamped<tf::Pose> end_effector_pose;
    end_effector_pose.setIdentity();
    end_effector_pose.frame_id_ = end_effector_frame_;
    end_effector_pose.stamp_ = ros::Time();

    tf::Stamped<tf::Pose> end_effector_pose_MAP;

    try {
        listener_.transformPose("/map", end_effector_pose, end_effector_pose_MAP);
    } catch (tf::TransformException& e) {
        ROS_ERROR("CartesianImpedance: %s", e.what());
        return;
    }

    //ROS_INFO("End effector pose: %f, %f, %f", end_effector_pose_MAP.getOrigin().getX(), end_effector_pose_MAP.getOrigin().getY(), end_effector_pose_MAP.getOrigin().getZ());

    Eigen::Vector3d end_effector_pos(end_effector_pose_MAP.getOrigin().getX(), end_effector_pose_MAP.getOrigin().getY(), end_effector_pose_MAP.getOrigin().getZ());

    Eigen::Vector3d total_force;
    total_force.setZero();

    for(vector<Box*>::iterator it = boxes_.begin(); it != boxes_.end(); ++it) {
        const Box& box = **it;

        bool inside_obstacle = true;
        Eigen::Vector3d diff;
        for(unsigned int i = 0; i < 3; ++i) {
            if (end_effector_pos[i] < box.min_[i]) {
                diff[i] = end_effector_pos[i] - box.min_[i];
                inside_obstacle = false;
            } else if (end_effector_pos[i] > box.max_[i]) {
                diff[i] = end_effector_pos[i] - box.max_[i];
                inside_obstacle = false;
            } else {
                diff[i] = 0;
            }
        }

        if (inside_obstacle) {
            ROS_ERROR("Inside obstacle!");
        } else {
            double distance = diff.norm();
            Eigen::Vector3d diff_normalized = diff / distance;

            double weight = 0;
            /*
            if (distance < 0.2) {
                double x = distance;
                weight = 10 / (x * x);
            } else if (distance < 0.3) {
                weight = 0;//0.1 - (distance - 0.2) / 10;
            }
            */
            if (distance < 0.1) {
                weight = (1 - (distance / 0.1)) * 10;
            }

            total_force += diff_normalized * weight;
        }
    }

    visualize(end_effector_pos + (total_force / 10));

    F_task.segment(0, 3) += total_force;

    // cout << F_task << endl;
}

void ObstacleAvoidance::visualize(const Eigen::Vector3d& dir_position) const {
    visualization_msgs::MarkerArray marker_array;

    int id = 0;
    for(vector<Box*>::const_iterator it = boxes_.begin(); it != boxes_.end(); ++it) {
        const Box& box = **it;

        visualization_msgs::Marker marker;
        marker.type = visualization_msgs::Marker::CUBE;
        marker.header.frame_id = "/map";
        marker.header.stamp = ros::Time::now();
        marker.id = id++;

        marker.scale.x = (box.max_[0] - box.min_[0]);
        marker.scale.y = (box.max_[1] - box.min_[1]);
        marker.scale.z = (box.max_[2] - box.min_[2]);

        marker.pose.position.x = box.min_[0] + marker.scale.x / 2;
        marker.pose.position.y = box.min_[1] + marker.scale.y / 2;
        marker.pose.position.z = box.min_[2] + marker.scale.z / 2;
        marker.pose.orientation.x = 0;
        marker.pose.orientation.y = 0;
        marker.pose.orientation.z = 0;
        marker.pose.orientation.w = 1;

        marker.color.a = 1;
        marker.color.r = 1;
        marker.color.g = 1;
        marker.color.b = 1;

        marker_array.markers.push_back(marker);
    }

    visualization_msgs::Marker dir;
    dir.type = visualization_msgs::Marker::SPHERE;
    dir.header.frame_id = "/map";
    dir.header.stamp = ros::Time::now();
    dir.id = id++;

    dir.scale.x = 0.1;
    dir.scale.y = 0.1;
    dir.scale.z = 0.1;

    dir.pose.position.x = dir_position[0];
    dir.pose.position.y = dir_position[1];
    dir.pose.position.z = dir_position[2];
    dir.pose.orientation.x = 0;
    dir.pose.orientation.y = 0;
    dir.pose.orientation.z = 0;
    dir.pose.orientation.w = 1;

    dir.color.a = 1;
    dir.color.r = 1;
    dir.color.g = 0;
    dir.color.b = 0;

    marker_array.markers.push_back(dir);

    pub_marker_.publish(marker_array);
}
