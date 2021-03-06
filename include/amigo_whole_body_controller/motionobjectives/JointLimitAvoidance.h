/*!
 * \author Janno Lunenburg
 * \date October, 2012
 * \version 0.1
 */

#ifndef JOINT_LIMIT_AVOIDANCE_
#define JOINT_LIMIT_AVOIDANCE_

#include <Eigen/Core>
#include <vector>
#include <kdl/jntarray.hpp>

class JointLimitAvoidance {

public:
    //! Constructor
    JointLimitAvoidance();

    //! Deconstructor
    virtual ~JointLimitAvoidance();

    //! Initialize();
    void initialize(const KDL::JntArray& q_min, const KDL::JntArray& q_max, const std::vector<double>& gain, const std::vector<double>& workspace);

    //! Update
    void update(const KDL::JntArray& q_in, Eigen::VectorXd& tau_out);

    /**
      * Returns cost, i.e., the absolute value of the torque of every single plugin
      */
    double getCost();

protected:

    //! Vector that hold the multiplication factor
    std::vector<double> K_;

    //! Joint array with average joint value
    // H = ((q-q0)/(qmax-qmin))^2
    // dH/dq = 2(q-q0)/(qmax-qmin)^2
    KDL::JntArray q0_;

    Eigen::VectorXd qmin_threshold_, qmax_threshold_;

    //! Number of joints
    uint num_joints_;

    /**
      * Current cost
      */
    double current_cost_;

};

#endif
