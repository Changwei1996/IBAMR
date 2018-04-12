// Filename: IBFEMethod.h
// Created on 5 Oct 2011 by Boyce Griffith
//
// Copyright (c) 2002-2017, Boyce Griffith
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of The University of North Carolina nor the names of
//      its contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef included_IBAMR_IBFEMethod
#define included_IBAMR_IBFEMethod

/////////////////////////////// INCLUDES /////////////////////////////////////

#include <set>
#include <stdbool.h>
#include <stddef.h>
#include <string>
#include <vector>

#include "GriddingAlgorithm.h"
#include "IntVector.h"
#include "LoadBalancer.h"
#include "PatchHierarchy.h"
#include "ibamr/StokesSpecifications.h"
#include "ibamr/IBStrategy.h"
#include "ibtk/FEDataManager.h"
#include "ibtk/libmesh_utilities.h"
#include "libmesh/enum_fe_family.h"
#include "libmesh/enum_order.h"
#include "libmesh/enum_quadrature_type.h"
#include "tbox/Pointer.h"

namespace IBTK
{
class RobinPhysBdryPatchStrategy;
} // namespace IBTK
namespace SAMRAI
{
namespace hier
{
template <int DIM>
class BasePatchHierarchy;
template <int DIM>
class BasePatchLevel;
} // namespace hier
namespace tbox
{
class Database;
template <class TYPE>
class Array;
} // namespace tbox
namespace xfer
{
template <int DIM>
class CoarsenSchedule;
template <int DIM>
class RefineSchedule;
} // namespace xfer
} // namespace SAMRAI
namespace libMesh
{
class EquationSystems;
class Mesh;
class Point;
class System;
template <typename T>
class NumericVector;
template <typename T>
class PetscVector;
} // namespace libMesh

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class IBFEMethod is an implementation of the abstract base class
 * IBStrategy that provides functionality required by the IB method with finite
 * element elasticity.
 */
class IBFEMethod : public IBStrategy
{
public:
    static const std::string COORDS_SYSTEM_NAME;
    static const std::string COORDS0_SYSTEM_NAME;
    static const std::string COORD_MAPPING_SYSTEM_NAME;
    static const std::string H_SYSTEM_NAME;
    static const std::string P_J_SYSTEM_NAME;
    static const std::string P_I_SYSTEM_NAME;
    static const std::string P_O_SYSTEM_NAME;
    static const std::string DP_J_SYSTEM_NAME;
    static const std::string DU_J_SYSTEM_NAME;
    static const std::string DV_J_SYSTEM_NAME;
    static const std::string DW_J_SYSTEM_NAME;
    static const std::string D2U_J_SYSTEM_NAME;
    static const std::string D2V_J_SYSTEM_NAME;
    static const std::string D2W_J_SYSTEM_NAME;
    static const std::string FORCE_SYSTEM_NAME;
    static const std::string FORCE_T_SYSTEM_NAME;
    static const std::string FORCE_B_SYSTEM_NAME;
    static const std::string FORCE_N_SYSTEM_NAME;
    static const std::string WSS_I_SYSTEM_NAME;
    static const std::string WSS_O_SYSTEM_NAME;
    static const std::string DV_X_O_SYSTEM_NAME;
    static const std::string DU_Y_O_SYSTEM_NAME;
    static const std::string DV_Z_O_SYSTEM_NAME;
    static const std::string DU_Z_O_SYSTEM_NAME;
    static const std::string DW_X_O_SYSTEM_NAME;
    static const std::string DW_Y_O_SYSTEM_NAME;
    static const std::string TAU_SYSTEM_NAME;
    static const std::string VELOCITY_SYSTEM_NAME;

    SAMRAI::tbox::Pointer<SAMRAI::pdat::SideVariable<NDIM, double> > mask_var;
    int mask_current_idx, mask_new_idx, mask_scratch_idx;

    /*!
     * \brief Constructor.
     */
    IBFEMethod(const std::string& object_name,
               SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db,
               libMesh::Mesh* mesh,
               int max_level_number,
               bool register_for_restart = true,
               const std::string& restart_read_dirname = "",
               unsigned int restart_restore_number = 0);

    /*!
     * \brief Constructor.
     */
    IBFEMethod(const std::string& object_name,
               SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db,
               const std::vector<libMesh::Mesh*>& meshes,
               int max_level_number,
               bool register_for_restart = true,
               const std::string& restart_read_dirname = "",
               unsigned int restart_restore_number = 0);

    /*!
     * \brief Destructor.
     */
    ~IBFEMethod();

    /*!
     * Return a pointer to the finite element data manager object for the
     * specified part.
     */
    IBTK::FEDataManager* getFEDataManager(unsigned int part = 0) const;

    /*!
     * Indicate that a part should use stress normalization.
     */
    void registerStressNormalizationPart(unsigned int part = 0);

    /*!
     * Typedef specifying interface for coordinate mapping function.
     */
    typedef void (*CoordinateMappingFcnPtr)(libMesh::Point& x, const libMesh::Point& X, void* ctx);

    /*!
     * Struct encapsulating coordinate mapping function data.
     */
    struct CoordinateMappingFcnData
    {
        CoordinateMappingFcnData(CoordinateMappingFcnPtr fcn = NULL, void* ctx = NULL) : fcn(fcn), ctx(ctx)
        {
        }

        CoordinateMappingFcnPtr fcn;
        void* ctx;
    };

    /*!
     * Register the (optional) function used to initialize the physical
     * coordinates from the Lagrangian coordinates.
     *
     * \note If no function is provided, the initial physical coordinates are
     * taken to be the same as the Lagrangian coordinate system, i.e., the
     * initial coordinate mapping is assumed to be the identity mapping.
     */
    void registerInitialCoordinateMappingFunction(const CoordinateMappingFcnData& data, unsigned int part = 0);

    /*!
     * Typedef specifying interface for Lagrangian body force distribution
     * function.
     */
    typedef IBTK::VectorMeshFcnPtr LagForceFcnPtr;

    /*!
     * Struct encapsulating Lagrangian force distribution data.
     */
    struct LagForceFcnData
    {
        LagForceFcnData(LagForceFcnPtr fcn = NULL,
                        const std::vector<IBTK::SystemData>& system_data = std::vector<IBTK::SystemData>(),
                        void* const ctx = NULL)
            : fcn(fcn), system_data(system_data), ctx(ctx)
        {
        }

        LagForceFcnPtr fcn;
        std::vector<IBTK::SystemData> system_data;
        void* ctx;
    };

    /*!
     * Register the (optional) function to compute body force distributions on
     * the Lagrangian finite element mesh.
     *
     * \note It is \em NOT possible to register multiple body force functions
     * with this class.
     */
    void registerLagForceFunction(const LagForceFcnData& data, unsigned int part = 0);

    /*!
     * Return the number of ghost cells required by the Lagrangian-Eulerian
     * interaction routines.
     */
    const SAMRAI::hier::IntVector<NDIM>& getMinimumGhostCellWidth() const;

    /*!
     * Setup the tag buffer.
     */
    void setupTagBuffer(SAMRAI::tbox::Array<int>& tag_buffer,
                        SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> > gridding_alg) const;

    /*!
     * Method to prepare to advance data from current_time to new_time.
     */
    void preprocessIntegrateData(double current_time, double new_time, int num_cycles);

    /*!
     * Method to clean up data following call(s) to integrateHierarchy().
     */
    void postprocessIntegrateData(double current_time, double new_time, int num_cycles);

    /*!
     * Interpolate the Eulerian velocity to the curvilinear mesh at the
     * specified time within the current time interval.
     */
    void interpolateVelocity(
        int u_data_idx,
        const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::CoarsenSchedule<NDIM> > >& u_synch_scheds,
        const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineSchedule<NDIM> > >& u_ghost_fill_scheds,
        double data_time);

    void ComputeVorticityForTraction(int u_data_idx,
                                     //~ const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::CoarsenSchedule<NDIM> >
                                     //>& u_synch_scheds, ~ const
                                     // std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineSchedule<NDIM> > >&
                                     // u_ghost_fill_scheds,
                                     double data_time,
                                     unsigned int part = 0);

    
    void calcHydroF(double data_time, int u_idx, int p_idx);
    
    
    

    void computeFluidTraction(double current_time,
                              int U_data_idx,
                              int p_data_idx,
                              unsigned int part = 0);

    void interpolatePressureForTraction(int p_data_idx, double data_time, unsigned int part = 0);

    //~ void computeVolandCOMOfStructure(libMesh::EquationSystems* solid_equation_systems);
    //~ void computeMOIOfStructure(EquationSystems* solid_equation_systems);


    /*!
     * Advance the positions of the Lagrangian structure using the forward Euler
     * method.
     *
     *
     */
    void forwardEulerStep(double current_time, double new_time);

    /*!
     * Advance the positions of the Lagrangian structure using the (explicit)
     * midpoint rule.
     */
    void midpointStep(double current_time, double new_time);

    /*!
     * Advance the positions of the Lagrangian structure using the (explicit)
     * trapezoidal rule.
     */
    void trapezoidalStep(double current_time, double new_time);

    /*!
     * Compute the Lagrangian force at the specified time within the current
     * time interval.
     */
    void computeLagrangianForce(double data_time);

    /*!
     * Spread the Lagrangian force to the Cartesian grid at the specified time
     * within the current time interval.
     */
    void
    spreadForce(int f_data_idx,
                IBTK::RobinPhysBdryPatchStrategy* f_phys_bdry_op,
                const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineSchedule<NDIM> > >& f_prolongation_scheds,
                double data_time);

    /*!
     * Get the default interpolation spec object used by the class.
     */
    IBTK::FEDataManager::InterpSpec getDefaultInterpSpec() const;

    /*!
     * Get the default spread spec object used by the class.
     */
    IBTK::FEDataManager::SpreadSpec getDefaultSpreadSpec() const;

    /*!
     * Set the interpolation spec object used with a particular mesh part.
     */
    void setInterpSpec(const IBTK::FEDataManager::InterpSpec& interp_spec, unsigned int part = 0);

    /*!
     * Set the spread spec object used with a particular mesh part.
     */
    void setSpreadSpec(const IBTK::FEDataManager::SpreadSpec& spread_spec, unsigned int part = 0);

    /*!
     * Initialize the FE equation systems objects.  This method must be called
     * prior to calling initializeFEData().
*/




    /*!
     * \brief Calculate any body forces for INS solver over here.
     */

    void initializeFEEquationSystems();

    /*!
     * Initialize FE data.  This method must be called prior to calling
     * IBHierarchyIntegrator::initializePatchHierarchy().
     */
    void initializeFEData();

    /*!
     * \brief Register Eulerian variables with the parent IBHierarchyIntegrator.
     */
    void registerEulerianVariables();

    /*!
     * Initialize Lagrangian data corresponding to the given AMR patch hierarchy
     * at the start of a computation.  If the computation is begun from a
     * restart file, data may be read from the restart databases.
     *
     * A patch data descriptor is provided for the Eulerian velocity in case
     * initialization requires interpolating Eulerian data.  Ghost cells for
     * Eulerian data will be filled upon entry to this function.
     */
    void initializePatchHierarchy(
        SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> > gridding_alg,
        int u_data_idx,
        const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::CoarsenSchedule<NDIM> > >& u_synch_scheds,
        const std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineSchedule<NDIM> > >& u_ghost_fill_scheds,
        int integrator_step,
        double init_data_time,
        bool initial_time);

    /*!
     * Register a load balancer and work load patch data index with the IB
     * strategy object.
     */
    void registerLoadBalancer(SAMRAI::tbox::Pointer<SAMRAI::mesh::LoadBalancer<NDIM> > load_balancer,
                              int workload_data_idx);

    /*!
     * Update work load estimates on each level of the patch hierarchy.
     */
    void updateWorkloadEstimates(SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                 int workload_data_idx);

    /*!
     * Begin redistributing Lagrangian data prior to regridding the patch
     * hierarchy.
     */
    void beginDataRedistribution(SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                                 SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> > gridding_alg);

    /*!
     * Complete redistributing Lagrangian data following regridding the patch
     * hierarchy.
     */
    void endDataRedistribution(SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
                               SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> > gridding_alg);

    /*!
     * Initialize data on a new level after it is inserted into an AMR patch
     * hierarchy by the gridding algorithm.
     *
     * \see SAMRAI::mesh::StandardTagAndInitStrategy::initializeLevelData
     */
    void initializeLevelData(SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
                             int level_number,
                             double init_data_time,
                             bool can_be_refined,
                             bool initial_time,
                             SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchLevel<NDIM> > old_level,
                             bool allocate_data);

    /*!
     * Reset cached hierarchy dependent data.
     *
     * \see SAMRAI::mesh::StandardTagAndInitStrategy::resetHierarchyConfiguration
     */
    void resetHierarchyConfiguration(SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
                                     int coarsest_level,
                                     int finest_level);

    /*!
     * Set integer tags to "one" in cells where refinement of the given level
     * should occur according to user-supplied feature detection criteria.
     *
     * \see SAMRAI::mesh::StandardTagAndInitStrategy::applyGradientDetector
     */
    void applyGradientDetector(SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
                               int level_number,
                               double error_data_time,
                               int tag_index,
                               bool initial_time,
                               bool uses_richardson_extrapolation_too);

    /*!
     * Write out object state to the given database.
     */
    void putToDatabase(SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> db);

    /*!
     * Write the equation_systems data to a restart file in the specified directory.
     */
    void writeFEDataToRestartFile(const std::string& restart_dump_dirname, unsigned int time_step_number);

protected:
    /*
     * \brief Compute the interior elastic density, possibly splitting off the
     * normal component of the transmission force along the physical boundary of
     * the Lagrangian structure.
     */
    void computeInteriorForceDensity(libMesh::PetscVector<double>& F_vec,
                                     libMesh::PetscVector<double>& F_n_vec,
                                     libMesh::PetscVector<double>& F_t_vec,
#if (NDIM == 3)
                                     libMesh::PetscVector<double>& F_b_vec,
#endif
                                     libMesh::PetscVector<double>& H_vec,
                                     libMesh::PetscVector<double>& X_vec,
                                     libMesh::PetscVector<double>& P_j_vec,
                                     libMesh::PetscVector<double>& dP_j_vec,
                                     libMesh::PetscVector<double>& du_j_vec,
                                     libMesh::PetscVector<double>& dv_j_vec,
#if (NDIM == 3)
                                     libMesh::PetscVector<double>& dw_j_vec,
#endif
                                     libMesh::PetscVector<double>& d2u_j_vec,
                                     libMesh::PetscVector<double>& d2v_j_vec,
#if (NDIM == 3)
                                     libMesh::PetscVector<double>& d2w_j_vec,
#endif
                                     double data_time,
                                     unsigned int part);

    /*!
     * \brief Impose jump conditions determined from the interior and
     * transmission force densities along the physical boundary of the
     * Lagrangian structure.
     */
    void imposeJumpConditionsWeak(int f_data_idx,
                              libMesh::PetscVector<double>& F_ghost_vec,
                              libMesh::PetscVector<double>& X_ghost_vec,
                              libMesh::PetscVector<double>& P_j_ghost_vec,
							 libMesh::PetscVector<double>& dP_j_ghost_vec,
                              libMesh::PetscVector<double>& du_j_ghost_vec,
                              libMesh::PetscVector<double>& dv_j_ghost_vec,
#if (NDIM == 3)
                              libMesh::PetscVector<double>& dw_j_ghost_vec,
#endif
                              libMesh::PetscVector<double>& d2u_j_ghost_vec,
                              libMesh::PetscVector<double>& d2v_j_ghost_vec,
#if (NDIM == 3)
                              libMesh::PetscVector<double>& d2w_j_ghost_vec,
#endif
                              double data_time,
                              unsigned int part);
                              
    void imposeJumpConditionsPointWise(int f_data_idx,
                              libMesh::PetscVector<double>& F_ghost_vec,
                              libMesh::PetscVector<double>& X_ghost_vec,
                              libMesh::PetscVector<double>& P_j_ghost_vec,
							 libMesh::PetscVector<double>& dP_j_ghost_vec,
                              libMesh::PetscVector<double>& du_j_ghost_vec,
                              libMesh::PetscVector<double>& dv_j_ghost_vec,
#if (NDIM == 3)
                              libMesh::PetscVector<double>& dw_j_ghost_vec,
#endif
                              libMesh::PetscVector<double>& d2u_j_ghost_vec,
                              libMesh::PetscVector<double>& d2v_j_ghost_vec,
#if (NDIM == 3)
                              libMesh::PetscVector<double>& d2w_j_ghost_vec,
#endif
                              double data_time,
                              unsigned int part);


    /*!
     * \brief Initialize the physical coordinates using the supplied coordinate
     * mapping function.  If no function is provided, the initial coordinates
     * are taken to be the Lagrangian coordinates.
     */
    void initializeCoordinates(unsigned int part);

    /*!
     * \brief Compute dX = x - X, useful mainly for visualization purposes.
     */
    void updateCoordinateMapping(unsigned int part);

    /*
     * Indicates whether the integrator should output logging messages.
     */
    bool d_do_log;

    /*
     * Pointers to the patch hierarchy and gridding algorithm objects associated
     * with this object.
     */
    SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > d_hierarchy;
    SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> > d_gridding_alg;
    bool d_is_initialized;

    /*
     * The current time step interval.
     */
    double d_current_time, d_new_time, d_half_time;

    /*
     * FE data associated with this object.
     */
    std::vector<libMesh::Mesh*> d_meshes;
    int d_max_level_number;
    std::vector<libMesh::EquationSystems*> d_equation_systems;

    const unsigned int d_num_parts;
    std::vector<IBTK::FEDataManager*> d_fe_data_managers;
    SAMRAI::hier::IntVector<NDIM> d_ghosts;
    std::vector<libMesh::System*> d_X_systems, d_X0_systems, d_U_systems, d_WSS_i_systems, d_WSS_o_systems,
        d_du_y_o_systems, d_dv_x_o_systems, d_dw_x_o_systems, d_dw_y_o_systems, d_P_o_systems, d_P_i_systems, d_TAU_systems,
        d_du_j_systems, d_dv_j_systems, d_dw_j_systems, d_F_systems, d_P_j_systems, d_dP_j_systems, d_F_n_systems,
        d_F_t_systems, d_F_b_systems, d_H_systems, d_dv_z_o_systems, d_du_z_o_systems;
    std::vector<libMesh::System *> d_d2u_j_systems, d_d2v_j_systems, d_d2w_j_systems;
    std::vector<libMesh::PetscVector<double> *> d_X_current_vecs, d_X_new_vecs, d_X_half_vecs, d_X_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double>*> d_X0_vecs, d_X0_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_U_current_vecs, d_U_new_vecs, d_U_half_vecs;
    std::vector<libMesh::PetscVector<double> *> d_F_half_vecs, d_F_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_H_half_vecs, d_H_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_F_t_half_vecs, d_F_t_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_F_b_half_vecs, d_F_b_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_F_n_half_vecs, d_F_n_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_dP_j_half_vecs, d_dP_j_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_P_j_half_vecs, d_P_j_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double>*> d_P_i_half_vecs, d_P_i_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double>*> d_P_o_half_vecs, d_P_o_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_du_j_half_vecs, d_du_j_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_dv_j_half_vecs, d_dv_j_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_dw_j_half_vecs, d_dw_j_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_d2u_j_half_vecs, d_d2u_j_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_d2v_j_half_vecs, d_d2v_j_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_d2w_j_half_vecs, d_d2w_j_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_WSS_i_half_vecs, d_WSS_i_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double> *> d_WSS_o_half_vecs, d_WSS_o_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double>*> d_du_y_o_half_vecs, d_du_y_o_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double>*> d_dv_x_o_half_vecs, d_dv_x_o_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double>*> d_dw_x_o_half_vecs, d_dw_x_o_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double>*> d_dw_y_o_half_vecs, d_dw_y_o_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double>*> d_du_z_o_half_vecs, d_du_z_o_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double>*> d_dv_z_o_half_vecs, d_dv_z_o_IB_ghost_vecs;
    std::vector<libMesh::PetscVector<double>*> d_TAU_half_vecs, d_TAU_IB_ghost_vecs;
    bool d_fe_equation_systems_initialized, d_fe_data_initialized;

    /*
     * Method paramters.
     */
    IBTK::FEDataManager::InterpSpec d_default_interp_spec;
    IBTK::FEDataManager::SpreadSpec d_default_spread_spec;
    std::vector<IBTK::FEDataManager::InterpSpec> d_interp_spec;
    std::vector<IBTK::FEDataManager::SpreadSpec> d_spread_spec;
    bool d_split_normal_force, d_split_tangential_force;
    bool d_use_jump_conditions;
    bool d_add_vorticity_term;
    bool d_use_higher_order_jump;
    bool d_modify_vel_interp_jumps;
    double d_vel_interp_width;
    double d_mu;
    std::vector<libMesh::FEFamily> d_fe_family;
    std::vector<libMesh::Order> d_fe_order;
    std::vector<libMesh::QuadratureType> d_default_quad_type;
    std::vector<libMesh::Order> d_default_quad_order;
    bool d_use_consistent_mass_matrix;
    //~ std::vector<IBTK::Vector3d> d_center_of_mass_solid_current, d_center_of_mass_solid_new;
    //~ std::vector<double> d_vol_solid_new;
    //~ std::vector<double> d_vol_solid_current;

    /*
     * Functions used to compute the initial coordinates of the Lagrangian mesh.
     */
    std::vector<CoordinateMappingFcnData> d_coordinate_mapping_fcn_data;

    /*
     * Functions used to compute forces on the Lagrangian mesh.
     */
    std::vector<LagForceFcnData> d_lag_force_fcn_data;

    /*
     * Nonuniform load balancing data structures.
     */
    SAMRAI::tbox::Pointer<SAMRAI::mesh::LoadBalancer<NDIM> > d_load_balancer;
    int d_workload_idx;

    /*
     * The object name is used as a handle to databases stored in restart files
     * and for error reporting purposes.
     */
    std::string d_object_name;

    /*
     * A boolean value indicating whether the class is registered with the
     * restart database.
     */
    bool d_registered_for_restart;

    /*
     * Directory and time step number to use when restarting.
     */
    std::string d_libmesh_restart_read_dir;
    int d_libmesh_restart_restore_number;

    /*
     * Restart file type for libMesh equation systems (e.g. xda or xdr).
     */
    std::string d_libmesh_restart_file_extension;

private:
    /*!
     * \brief Default constructor.
     *
     * \note This constructor is not implemented and should not be used.
     */
    IBFEMethod();

    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    IBFEMethod(const IBFEMethod& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    IBFEMethod& operator=(const IBFEMethod& that);

    /*!
     * Implementation of class constructor.
     */
    void commonConstructor(const std::string& object_name,
                           SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db,
                           const std::vector<libMesh::Mesh*>& meshes,
                           int max_level_number,
                           bool register_for_restart,
                           const std::string& restart_read_dirname,
                           unsigned int restart_restore_number);

    /*!
     * Read input values from a given database.
     */
    void getFromInput(SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> db, bool is_from_restart);

    /*!
     * Read object state from the restart file and initialize class data
     * members.
     */
    void getFromRestart();
    
   std::vector<void (*)(const double, const double, const int, void*)> d_prefluidsolve_callback_fns;
    std::vector<void*> d_prefluidsolve_callback_fns_ctx;
    
    
    
};
} // namespace IBAMR

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBAMR_IBFEMethod
