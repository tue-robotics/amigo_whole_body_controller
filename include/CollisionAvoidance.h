/*!
 * \author Paul Metsemakers
 * \date January, 2013
 * \version 0.1
 */

#ifndef WBC_OBSTACLEAVOIDANCE_H_
#define WBC_OBSTACLEAVOIDANCE_H_

// ROS
#include "ros/ros.h"

// Eigen
#include <Eigen/Core>

// Plugins
#include "MotionObjective.h"
#include "ChainParser.h"
#include "Chain.h"
#include "Tree.h"

// Map
#include <tue_map_3d/Map3D.h>


// Bullet GJK Closest Point calculation
#include <BulletCollision/NarrowPhaseCollision/btGjkPairDetector.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btConeShape.h>
#include <BulletCollision/CollisionShapes/btCylinderShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/NarrowPhaseCollision/btPointCollector.h>
#include <BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.h>
#include <BulletCollision/NarrowPhaseCollision/btMinkowskiPenetrationDepthSolver.h>
#include <Bullet-C-Api.h>

/////

class CollisionAvoidance : public MotionObjective
{

    struct Box
    {
        Box(const Eigen::Vector3d& min, const Eigen::Vector3d& max)
            : min_(min), max_(max)
        {
        }
        Eigen::Vector3d min_;
        Eigen::Vector3d max_;
    };


public:

    /// Define the type of OctoMap as timestamped
    typedef octomap::OcTreeStamped OctreeType;

    struct collisionAvoidanceParameters
    {
        struct Parameters
        {
            double f_max;
            double d_threshold;
            int order;
            double octomap_resolution;
        } ;
        Parameters self_collision;
        Parameters environment_collision;
    } ca_param_;

    //ToDo: make configure, start- and stophook. Components can be started/stopped in an actionlib kind of fashion

    /**
     * Constructor
     */
    CollisionAvoidance(collisionAvoidanceParameters &parameters, const double Ts);

    /**
     * Deconstructor
     */
    virtual ~CollisionAvoidance();

    /**
     * Initialize function
     */
    bool initialize(RobotState &robotstate);

    void apply(RobotState& robotstate);

    void setOctoMap(octomap::OcTree *octree);

protected:

    //! Sampling time
    double Ts_;

    Tree tree_;

    RobotState* robot_state_;

    ros::Publisher pub_model_marker_;
    ros::Publisher pub_forces_marker_;
    ros::Publisher pub_bbx_marker_;
    ros::Publisher pub_rep_force_;
    ros::Publisher pub_d_min_;
    ros::Publisher pub_CA_wrench_;
    ros::Publisher pub_delta_p_;

    std::vector< std::vector<RobotState::CollisionBody> > active_groups_;
    std::vector< std::vector<RobotState::CollisionBody> > collision_groups_;

    struct Voxel {
        KDL::Frame center_point;
        double size_voxel;
    };
    struct Distance {
        std::string frame_id;
        btPointCollector bt_distance;
    } ;

    struct RepulsiveForce {
        std::string frame_id;
        btVector3 pointOnA;
        btVector3 direction;
        btScalar amplitude;
    } ;

    struct Wrench {
        std::string frame_id;
        Eigen::VectorXd wrench;
    } ;

    KDL::Frame no_fix_;

    octomap::OcTree* octomap_;

    // Minimum and maximum point of the BBX
    std::vector<octomath::Vector3> min_;
    std::vector<octomath::Vector3> max_;


    /**
     * @brief Calculate the repulsive forces as a result of self collision avoidance
     * @param Output: Vector with the minimum distances between robot collision bodies, vector with the repulsive forces
     */
    void selfCollision(std::vector<Distance> &min_distances, std::vector<RepulsiveForce> &repulsive_forces);

    /**
     * @brief Calculate the repulsive forces as a result of environment collision avoidance
     * @param Output: Vector with the minimum distances to the environment, vector with the repulsive forces
     */
    void environmentCollision(std::vector<Distance> &min_distances, std::vector<RepulsiveForce> &repulsive_forces);

    /**
     * @brief Construct the collision bodies
     * @param Input: The robot state
     */
    void initializeCollisionModel(RobotState &robotstate);

    /**
     * @brief Calculate the pose of the collision bodies
     */
    void calculateTransform();

    /**
     * @brief Calculate the Bullet shape pose from the corresponding FK pose using a correction from this frame pose
     * @param Input: Pose of the KDL frame and the fix for the collision bodies, Output: Bullet transform
     */
    void setTransform(KDL::Frame &fkPose, KDL::Frame& fixPose, btTransform &transform_out);

    /**
     * @brief Calculate the closest distance between two collision bodies
     * @param Input: The two collision bodies and their poses, output: The closest points with the corresponding distance en normal vector between them
     */
    void distanceCalculation(btConvexShape &shapeA, btConvexShape &shapeB, btTransform& transformA, btTransform& transformB, btPointCollector& distance_out);

    /**
     * @brief Select the minimal closest distance
     * @param Vector with all closest distances from a collision body, Vector with the minimal closest distances
     */
    void pickMinimumDistance(std::vector<Distance> &calculatedDistances, std::vector<Distance> &minimumDistances);

    /**
     * @brief Calculate the amplitude of the repulsive forces
     * @param Vector with the minimal closest distances, Vector with the repulsive forces
     */
    void calculateRepulsiveForce(std::vector<Distance> &minimumDistances, std::vector<RepulsiveForce> &repulsiveForces, collisionAvoidanceParameters::Parameters &param);

    /**
     * @brief Calculate the wrenches as a function of the repulsive forces
     * @param Input: Vector with the repulsive forces, Output: Vector with the wrenches
     */
    void calculateWrenches(std::vector<RepulsiveForce> &repulsive_forces, std::vector<Wrench> &wrenches_out);

    /**
     * @brief Output the wrench to the KDL tree
     * @param Vector with wrenches
     */
    void outputWrenches(std::vector<Wrench> &wrenches);

    /**
     * @brief Visualize the collision avoidance in RVIZ
     * @param Vector with minimal distances
     */
    void visualize(std::vector<Distance> &min_distances) const;

    /**
     * @brief Construct the visualization markers to visualize the collision model in RVIZ
     * @param Collision body information
     */
    void visualizeCollisionModel(RobotState::CollisionBody collisionBody,int id)  const;

    /**
     * @brief Construct the visualization markers to visualize the repulsive forces in RVIZ
     * @param Collision vector
     */
    void visualizeRepulsiveForce(Distance &d_min, int id) const;

    /**
     * @brief Construct the visualization markers to visualize the bounding box in RVIZ
     * @param Minimum and maximum point of the bounding box
     */
    void visualizeBBX(octomath::Vector3 min, octomath::Vector3 max, int id) const;

    /**
     * @brief Find the outer points of the collision models for the bounding box construction in /map frame
     * @param Input: The collision body, Output: The minimum and maximum point of the collision body in /map frame
     */
    void findOuterPoints(RobotState::CollisionBody& collisionBody, btVector3 &min, btVector3 &max);

    /**
     * @brief Test the Bullet closest distance calculation
     */
    void bulletTest();
};

#endif
