#include "amigo_whole_body_controller/motionobjectives/JointLimitAvoidance.h"

#include <ros/console.h>

JointLimitAvoidance::JointLimitAvoidance() {

}

JointLimitAvoidance::~JointLimitAvoidance() {

}

/*void JointLimitAvoidance::initialize(const KDL::JntArray& q_min, const KDL::JntArray& q_max, const std::vector<double>& gain) {

    num_joints_ = q_min.data.rows();
    q0_.resize(num_joints_);
    K_.resize(num_joints_);

    ROS_INFO("Length joint array = %i",num_joints_);
    for (uint i = 0; i < num_joints_; i++) {
        q0_(i) = (q_min(i)+q_max(i))/2;
        ROS_INFO("qmin = %f, qmax = %f, q0 = %f",q_min(i),q_max(i),q0_(i));
        K_[i] = 2*gain[i] / (q_max(i) - q_min(i))*(q_max(i) - q_min(i));
    }
    ROS_INFO("Joint limit avoidance initialized");

}*/
void JointLimitAvoidance::initialize(const KDL::JntArray& q_min, const KDL::JntArray& q_max, const std::vector<double>& gain, const std::vector<double>& workspace) {

    num_joints_ = q_min.rows();
    ///q0_.resize(num_joints_);
    K_.resize(num_joints_);
    qmin_threshold_.resize(num_joints_);
    qmax_threshold_.resize(num_joints_);

    ROS_INFO("Length joint array = %i",num_joints_);
    for (uint i = 0; i < num_joints_; i++) {
        ///q0_(i) = (q_min[i]+q_max[i])/2;
        K_[i] = gain[i] / ((q_max(i) - q_min(i))*(q_max(i) - q_min(i)));
        qmin_threshold_(i) = q_min(i) + (1.0-workspace[i])/2 * (q_max(i) - q_min(i));
        qmax_threshold_(i) = q_max(i) - (1.0-workspace[i])/2 * (q_max(i) - q_min(i));
        //ROS_INFO("qmin = %f, qthresmin = %f, qthresmax = %f, qmax = %f, K = %f",q_min(i),qmin_threshold_(i),qmax_threshold_(i),q_max(i), K_[i]);
    }

    current_cost_ = 0;

}

void JointLimitAvoidance::update(const KDL::JntArray& q_in, Eigen::VectorXd& tau_out) {

    current_cost_ = 0;

    //ToDo: Check joint limit avoidance algorithm
    for (uint i = 0; i < num_joints_; i++) {
		double d_tau = 0.0;
        if (q_in(i) < qmin_threshold_(i)) d_tau = K_[i]*(qmin_threshold_(i) - q_in(i));
        else if (q_in(i) > qmax_threshold_(i)) d_tau = K_[i]*(qmax_threshold_(i) - q_in(i));

		tau_out(i) += d_tau;

        // Add this to the costs
        current_cost_ += fabs(d_tau);
    }

}


double JointLimitAvoidance::getCost() {

    return current_cost_;
}
