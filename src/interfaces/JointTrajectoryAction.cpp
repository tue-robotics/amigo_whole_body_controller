#include <amigo_whole_body_controller/interfaces/JointTrajectoryAction.h>

JointTrajectoryAction::JointTrajectoryAction(WholeBodyController *wbc)
{
    wbc_ = wbc;
    initialize();
}

JointTrajectoryAction::~JointTrajectoryAction() {
    delete server_;
    server_ = NULL;
    delete server_left_;
    server_left_ = NULL;
    delete server_right_;
    server_right_ = NULL;
}

bool JointTrajectoryAction::initialize() {

    ROS_INFO_NAMED("JTA", "JTA: Initializing");
    trajectory_index_ = 0;

    ros::NodeHandle n;

    /// Action server
    server_ = new action_server(n, "joint_trajectory_action", false);
    server_->registerGoalCallback(boost::bind(&JointTrajectoryAction::goalCB, this));
    server_->start();

    server_left_ = new action_server(n, "left_arm/joint_trajectory_action", false);
    server_left_->registerGoalCallback(boost::bind(&JointTrajectoryAction::goalCBLeft, this));
    server_left_->start();

    server_right_ = new action_server(n, "right_arm/joint_trajectory_action", false);
    server_right_->registerGoalCallback(boost::bind(&JointTrajectoryAction::goalCBRight, this));
    server_right_->start();

    /// Get joint names and constraints from the parameter server

    // ToDo: why not parse this from the URDF or (SRDF?) model?
    ros::NodeHandle nh("~");
    using namespace XmlRpc;

    /// Gets all of the joints
    XmlRpc::XmlRpcValue joint_names;
    if (!nh.getParam("joint_names", joint_names))
    {
        ROS_FATAL_NAMED("JTA", "JTA: No joints given. (namespace: %s)", nh.getNamespace().c_str());
        exit(1);
    }
    if (joint_names.getType() != XmlRpc::XmlRpcValue::TypeArray)
    {
        ROS_FATAL_NAMED("JTA", "JTA: Malformed joint specification.  (namespace: %s)", nh.getNamespace().c_str());
        exit(1);
    }
    for (int i = 0; i < joint_names.size(); ++i)
    {
        XmlRpcValue &name_value = joint_names[i];
        if (name_value.getType() != XmlRpcValue::TypeString)
        {
            ROS_FATAL_NAMED("JTA", "JTA: Array of joint names should contain all strings.  (namespace: %s)",
                      nh.getNamespace().c_str());
            exit(1);
        }

        joint_index_[(std::string)name_value] = joint_names_.size();
        joint_names_.push_back((std::string)name_value);

    }

    nh.param("constraints/goal_time", goal_time_constraint_, 0.0);

    /// Gets the constraints for each joint.
    for (size_t i = 0; i < joint_names_.size(); ++i)
    {
        std::string ns = std::string("constraints/") + joint_names_[i];
        double ig, fg,t;
        nh.param(ns + "/intermediate_goal", ig, -1.0);
        nh.param(ns + "/final_goal", fg, -1.0);
        nh.param(ns + "/trajectory", t, -1.0);
        intermediate_goal_constraints_[joint_names_[i]] = ig;
        final_goal_constraints_[joint_names_[i]] = fg;
        trajectory_constraints_[joint_names_[i]] = t;
        ROS_DEBUG_NAMED("JTA", "JTA: %s\t [%f,\t%f,\t%f]",joint_names_[i].c_str(),ig,fg,t);
    }
/*
    // Check if it works
    for (unsigned int i = 0; i < joint_names_.size(); ++i)
    {
        std::string joint_name = joint_names_[i];
        std::map<std::string, unsigned int>::const_iterator index_iter = joint_index_.find(joint_name);
        if (index_iter != joint_index_.end())
        {
            //unsigned int index = index_iter->second;
            std::map<std::string, double>::const_iterator diter;
            diter = intermediate_goal_constraints_.find(joint_name);
            double ig = diter->second;
            diter = final_goal_constraints_.find(joint_name);
            double fg = diter->second;
            diter  = trajectory_constraints_.find(joint_name);
            double t = diter->second;
            ROS_DEBUG_NAMED("JTA", "JTA: %s\t [%f,\t%f,\t%f]",joint_name.c_str(),ig,fg,t);
        } else {
            ROS_ERROR_NAMED("JTA", "JTA: Something went terribly wrong");
        }
    }
*/
    return true;
}

void JointTrajectoryAction::update() {

    /// Only do stuff when required
    if (is_active_)
    {
        /// Check time constraint
        // We never do this?

        /// Loop over joints
        /// Check trajectory, intermediate, final goal constraints, convergence
        unsigned int converged_joints = 0;
        for (unsigned int i = 0; i < active_goal_.trajectory.joint_names.size(); i++)
        {
            std::string joint_name = active_goal_.trajectory.joint_names[i];
            double ref = active_goal_.trajectory.points[trajectory_index_].positions[i];
            double pos = wbc_->getJointPosition(joint_name);
            double abs_error = ref-pos;

            /// Check trajectory constraints
            if(abs_error > trajectory_constraints_[joint_name]) {
                ROS_WARN_NAMED("JTA", "JTA: Aborting because the trajectory constraint was violated");
                //for (unsigned int j = 0; j < number_of_goal_joints_; j++) {
                //    if ( fabs(ref_pos_[j] - cur_pos_[j]) > intermediate_goal_constraints_[joint_names_[j]]) {
                //        ROS_WARN_NAMED("JTA", "JTA: Error joint %s = %f exceeds intermediate joint constraint (%f)",joint_names_[j].c_str(),ref_pos_[j] - cur_pos_[j],intermediate_goal_constraints_[joint_names_[j]]);
                //    }
                //    else if ( fabs(ref_pos_[j] - cur_pos_[j]) > final_goal_constraints_[joint_names_[j]]) {
                //        ROS_WARN_NAMED("JTA", "JTA: Error joint %s = %f exceeds final joint contraint (%f)",joint_names_[j].c_str(),ref_pos_[j] - cur_pos_[j],final_goal_constraints_[joint_names_[j]]);
                //    }
            }

            /// Check if this joint has converged
            if(trajectory_index_ < ((int)active_goal_.trajectory.points.size()-1))
            {
//                ROS_DEBUG_NAMED("JTA", "JTA: intermediate constraints check for %s %f < %f",
//                                joint_name.c_str(),
//                                abs_error,
//                                intermediate_goal_constraints_[joint_name]);
                if(abs_error < intermediate_goal_constraints_[joint_name])
                {
                    converged_joints += 1;
                }
            }
            else
            {
//                ROS_DEBUG_NAMED("JTA", "JTA: final constraints check for %s %f < %f",
//                                joint_name.c_str(),
//                                abs_error,
//                                final_goal_constraints_[joint_name]);
                if(abs_error < final_goal_constraints_[joint_name])
                {
                    converged_joints += 1;
                }
            }
        }

        /// Check whether all joints of this point have converged
        if (converged_joints == active_goal_.trajectory.joint_names.size() && converged_joints != 0)
        {
            ROS_INFO_NAMED("JTA", "JTA: all joints converged for trajectory point %u", trajectory_index_);
            trajectory_index_ += 1;

            // if we have more points to do, give them to the posture controller
            if (trajectory_index_ != active_goal_.trajectory.points.size()) {
                setJointPositions();
            }
        }

        /// Check whether the final goal is achieved
        if (trajectory_index_ == active_goal_.trajectory.points.size() && trajectory_index_ != 0)
        {
            ROS_INFO_NAMED("JTA", "JTA: trajectory fully converged");
            setSucceeded();
            is_active_ = false;
        }
    }
}

void JointTrajectoryAction::goalCB() {

    ROS_INFO_NAMED("JTA", "JTA: Received new joint goal");
    trajectory_index_ = 0;
    goal_reception_time_ = ros::Time::now();

    active_goal_ = *server_->acceptNewGoal();
    is_active_ = true;
    recent_server_ = "";

    if (!setJointPositions())
    {
        ROS_ERROR_NAMED("JTA", "JTA: Cannot set desired joint positions");
        server_->setAborted();
    }

}

void JointTrajectoryAction::goalCBLeft() {

    ROS_INFO_NAMED("JTA", "JTA: Received new joint goal");
    trajectory_index_ = 0;
    goal_reception_time_ = ros::Time::now();

    active_goal_ = *server_left_->acceptNewGoal();
    is_active_ = true;
    recent_server_ = "left";

    /// Remove motion objective because this will typically be conflicting
    /// Although not really neat, this is how our executives are set up
    std::vector<MotionObjective*> imps_to_remove = wbc_->getCartesianImpedances("grippoint_left", "");
    ROS_DEBUG_NAMED("JTA", "JTA: Removing %i impedances",(int)imps_to_remove.size());
    for (unsigned int i = 0; i < imps_to_remove.size(); i++) {
        wbc_->removeMotionObjective(imps_to_remove[i]);
    }

    if (!setJointPositions())
    {
        ROS_ERROR_NAMED("JTA", "JTA: Cannot set desired joint positions");
        server_left_->setAborted();
    }

}

void JointTrajectoryAction::goalCBRight() {

    ROS_INFO_NAMED("JTA", "JTA: Received new joint goal");
    trajectory_index_ = 0;
    goal_reception_time_ = ros::Time::now();

    active_goal_ = *server_right_->acceptNewGoal();
    is_active_ = true;
    recent_server_ = "right";

    /// Remove motion objective because this will typically be conflicting
    /// Although not really neat, this is how our executives are set up
    std::vector<MotionObjective*> imps_to_remove = wbc_->getCartesianImpedances("grippoint_right", "");
    ROS_DEBUG_NAMED("JTA", "JTA: Removing %i impedances",(int)imps_to_remove.size());
    for (unsigned int i = 0; i < imps_to_remove.size(); i++) {
        wbc_->removeMotionObjective(imps_to_remove[i]);
    }

    if (!setJointPositions())
    {
        ROS_ERROR_NAMED("JTA", "JTA: Cannot set desired joint positions");
        server_right_->setAborted();
    }

}

void JointTrajectoryAction::cancelCB() {
    is_active_ = false;
}

void JointTrajectoryAction::setSucceeded()
{
    if (recent_server_ == "left") {
        server_left_->setSucceeded();
    } else if (recent_server_ == "right") {
        server_right_->setSucceeded();
    } else {
        server_->setSucceeded();
    }
}

void JointTrajectoryAction::setAborted()
{
    if (recent_server_ == "left") {
        server_left_->setAborted();
    } else if (recent_server_ == "right") {
        server_right_->setAborted();
    } else {
        server_->setAborted();
    }
}

bool JointTrajectoryAction::setJointPositions()
{
    ROS_DEBUG_NAMED("JTA", "JTA: Setting joint positions");
    /// Loop over joints
    for (unsigned i = 0; i < active_goal_.trajectory.joint_names.size(); i++)
    {
        wbc_->setDesiredJointPosition(active_goal_.trajectory.joint_names[i], active_goal_.trajectory.points[trajectory_index_].positions[i]);
    }
    return true;
}
