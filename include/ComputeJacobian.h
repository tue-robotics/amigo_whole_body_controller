/*!
 * \author Janno Lunenburg
 * \date September, 2012
 * \version 0.1
 */

#ifndef COMPUTE_JACOBIAN_H_
#define COMPUTE_JACOBIAN_H_

// ROS
#include <ros/ros.h>

// Construct Chain
#include "ConstructChain.h"

#include "TreeDescription.h"

// KDL
#include <kdl/chainjnttojacsolver.hpp>

// Vector and map
#include <vector>
#include <map>

#include "Chain.h"
#include "Component.h"

class ComputeJacobian {
public:

    /*
     * Constructor
     */
    ComputeJacobian();

    /*
     * Deconstructor
     */
   virtual ~ComputeJacobian();

    /*
     * Initialize
     */
    bool Initialize(std::map<std::string, Component*>& component_map, std::map<std::string, uint>& joint_name_index_map);

    //void Update(std::map<std::string, std::vector<double> >);
    // Why can't I make the update const &?
    void Update(const std::map<std::string, Component*> component_map, Eigen::MatrixXd& Jacobian);

    //! Map contains a string to describe which component this concerns and a vector with eventually two integers to describe the start and end-index of this component
    std::map<std::string, std::vector<int> > index_map;

    //!Number of joints
    unsigned int num_joints;//, num_manipulator_joints, num_torso_joints;

    //! Joint limits
    ///KDL::JntArray joint_min, joint_max;
    std::vector<double> q_min_, q_max_;


private:

    /*
     * Function reads the number of joints in a certain chain and puts the desired values in the index_map
     *
     */
    bool readJoints(urdf::Model &robot_model, const Chain& chain, std::map<std::string, uint>& joint_name_index_map);

    std::vector<Chain> chains_;

    //! Vector containing interfaces between various components
    //TODO: This isn't quite enough, it would also be desirable to know what components it concerns (for bookkeeping)
    //std::vector<std::vector<std::string> > chain_description_array;

    //! Vector containing the various chains
    //std::vector<KDL::Chain> chain_array;

    //! Vector containing the ChainJntToJacSolvers
    //std::vector<boost::scoped_ptr<KDL::ChainJntToJacSolver> > jnt_to_jac_solver_array;
    //boost::scoped_ptr<KDL::ChainJntToJacSolver> jnt_to_jac_solver_array[2];
    //std::vector<KDL::ChainJntToJacSolver*> jnt_to_jac_solver_array;

    //! Number of chains and active chainsused
    int num_chains, num_active_chains_;

};

#endif
