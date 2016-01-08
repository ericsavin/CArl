#include "coupled_system.h"


void create_mesh_map(std::string &filename, std::unordered_map<int,int> &node_map, std::unordered_map<int,int> &element_map)
{
	/*
	 *  libmesh : nodes and elements start at 0, and are continuous
	 *  GMSH    : nodes and elements can be discontinuous, libmesh builds a map
	 *  Abaqus  : not sure, but seems similar to GMSH. libmesh builds a map, but
	 *            differently from the method used for GMSH
	 */

	if(boost::filesystem::path(filename).extension().string().compare(".msh")==0)
	{
		build_mesh_map_Gmsh(filename,node_map,element_map);
	}
	else
	{
		libmesh_error_msg("Error: unknown filetype");
	}
}

void build_mesh_map_Gmsh(std::string &filename, std::unordered_map<int,int> &node_map, std::unordered_map<int,int> &element_map)
{
	if(!(boost::filesystem::path(filename).extension().string().compare(".msh")==0))
	{
		libmesh_error_msg("Error: expected Gmsh filetype");
	}

	// Open file
	std::ifstream dataF(filename);
	if(!dataF.good())
	{
		libmesh_error_msg("Error: bad filestream");
	}

	// Buffer string
	std::string bufferLine;
	std::stringstream	dataBuffer;

	int nbOfNodes = -1;
	int nbOfElements = -1;

	int gmshNodeIndex = -1;
	int gmshElemIndex = -1;

	bool hasNodes = false;
	bool hasElements = false;

	// Read info until the file ends
	while(std::getline(dataF,bufferLine))
	{
		/*
		 * 		As of the current Gmsh version (2.11, filetype v. 2.2), each
		 * 	file has only one $Nodes and one $Elements sections
		 */
		if(bufferLine.compare("$Nodes")==0)
		{
			// Read node indexes
			hasNodes = true;
			dataF >> nbOfNodes;

			node_map.reserve(nbOfNodes);

			// Line structure
			// [index] [X] [Y] [Z]
			std::getline(dataF,bufferLine);
			for(unsigned int iii = 0; iii < nbOfNodes; ++iii)
			{
				std::getline(dataF,bufferLine);
				dataBuffer.str("");
				dataBuffer.clear();
				dataBuffer << bufferLine;

				dataBuffer >> gmshNodeIndex;

				// Add point to map
				node_map[gmshNodeIndex] = iii;
			}
		}

		if(bufferLine.compare("$Elements")==0)
		{
			// Read node indexes
			hasElements = true;
			dataF >> nbOfElements;

			element_map.reserve(nbOfElements);

			// Line structure
			// [index] [X] [Y] [Z]
			std::getline(dataF,bufferLine);
			for(unsigned int iii = 0; iii < nbOfElements; ++iii)
			{
				std::getline(dataF,bufferLine);
				dataBuffer.str("");
				dataBuffer.clear();
				dataBuffer << bufferLine;

				dataBuffer 	>> gmshElemIndex;

				// Add point to map
				element_map[gmshElemIndex] = iii;
			}
		}

		if(hasNodes && hasElements)
		{
			break;
		}
	}
}

void assemble_coupling_matrix(	libMesh::EquationSystems& BIG_eq_system,
								libMesh::EquationSystems& micro_eq_system,
								libMesh::EquationSystems& inter_eq_system,
								std::vector<std::pair<int,int> >& intersection_table_BIGmicro,
								std::unordered_multimap<int,int>& intersection_table_inter,
								libMesh::SparseMatrix<libMesh::Number>& couplingMatrix,
								bool bSameElemsType)
{
	// Pointers to the meshes
	const libMesh::MeshBase& mesh_BIG = BIG_eq_system.get_mesh();
	const libMesh::MeshBase& mesh_micro = micro_eq_system.get_mesh();
	const libMesh::MeshBase& mesh_inter = inter_eq_system.get_mesh();

	const unsigned int dim_BIG = mesh_BIG.mesh_dimension();
	const unsigned int dim_micro = mesh_micro.mesh_dimension();
	const unsigned int dim_inter = mesh_inter.mesh_dimension();

	// Systems and vars
	libMesh::LinearImplicitSystem& volume_BIG_system
		= BIG_eq_system.get_system<libMesh::LinearImplicitSystem>("VolTest");
	libMesh::LinearImplicitSystem& volume_micro_system
		= micro_eq_system.get_system<libMesh::LinearImplicitSystem>("VolTest");
	libMesh::LinearImplicitSystem& volume_inter_system
		= inter_eq_system.get_system<libMesh::LinearImplicitSystem>("VolTest");

	const unsigned int n_components = 1;
	const unsigned int vol_test_BIG = volume_BIG_system.variable_number("SillyVar");
	const unsigned int vol_test_micro = volume_micro_system.variable_number ("SillyVar");
	const unsigned int vol_test_inter = volume_inter_system.variable_number ("SillyVar");

	// DoF maps
	const libMesh::DofMap& dof_map_BIG = volume_BIG_system.get_dof_map();
	const libMesh::DofMap& dof_map_micro = volume_micro_system.get_dof_map();
	const libMesh::DofMap& dof_map_inter = volume_inter_system.get_dof_map();

	libMesh::FEType fe_type_BIG = dof_map_BIG.variable_type(vol_test_BIG);
	libMesh::FEType fe_type_micro = dof_map_micro.variable_type(vol_test_micro);
	libMesh::FEType fe_type_inter = dof_map_inter.variable_type(vol_test_inter);

	// Set up FE bases and quadratures
	// TODO : verify if the micro's quadrature is really needed
	libMesh::UniquePtr<libMesh::FEBase> fe_BIG (libMesh::FEBase::build(dim_BIG, fe_type_BIG));
	libMesh::UniquePtr<libMesh::FEBase> fe_micro (libMesh::FEBase::build(dim_micro, fe_type_micro));
	libMesh::UniquePtr<libMesh::FEBase> fe_inter (libMesh::FEBase::build(dim_inter, fe_type_inter));

	libMesh::QGauss qrule_BIG(dim_BIG, fe_type_BIG.default_quadrature_order());
	fe_BIG->attach_quadrature_rule (&qrule_BIG);

	libMesh::QGauss qrule_inter(dim_inter, fe_type_inter.default_quadrature_order());
	fe_inter->attach_quadrature_rule (&qrule_inter);

//	QGauss qrule_micro(dim_micro, fe_type_micro.default_quadrature_order());
//	fe_micro->attach_quadrature_rule (&qrule_micro);

	// Jacobian -> only the BIG one is needed
	const std::vector<libMesh::Real>& JxW = fe_inter->get_JxW();

	// Shape functions
	const std::vector<std::vector<libMesh::Real> >& phi_BIG = fe_BIG->get_phi();
	const std::vector<std::vector<libMesh::Real> >& phi_micro = fe_micro->get_phi();

	std::vector<libMesh::dof_id_type> dof_indices_BIG;
	std::vector<libMesh::dof_id_type> dof_indices_micro;

	// Local matrix
	libMesh::DenseMatrix<libMesh::Number> Me;
	unsigned int n_dofs_BIG;
	unsigned int n_dofs_micro;

	if(bSameElemsType)
	{
		const libMesh::Elem* elem_BIG = mesh_BIG.elem(0);
		dof_map_BIG.dof_indices(elem_BIG, dof_indices_BIG);
		n_dofs_BIG   = dof_indices_BIG.size();

		const libMesh::Elem* elem_micro = mesh_micro.elem(0);
		dof_map_micro.dof_indices(elem_micro, dof_indices_micro);
		n_dofs_micro   = dof_indices_micro.size();

		Me.resize (n_dofs_BIG, n_dofs_micro);
	}

	// Vector that will keep the quadrature points
	const std::vector<libMesh::Point>& quad_points_inter = fe_inter->get_xyz();

	// Initialize global matrix
	const unsigned int BIG_M = dof_map_BIG.n_dofs();
	const unsigned int micro_N = dof_map_micro.n_dofs();

	const unsigned int BIG_M_local = dof_map_BIG.n_local_dofs();
	const unsigned int micro_N_local = dof_map_micro.n_local_dofs();

	couplingMatrix.init(BIG_M,micro_N,BIG_M_local,micro_N_local);

	// Intersection indexes and iterators
	int nb_of_intersections = intersection_table_BIGmicro.size();
	int elem_BIG_idx = -1;
	int elem_micro_idx = -1;
	int elem_inter_idx = -1;
	std::unordered_multimap<int,int>::iterator it_inter_idx;

	// For each intersection
	for(int iii = 0; iii < nb_of_intersections; ++iii)
	{
		// Get the big and micro element pointers
		elem_BIG_idx = intersection_table_BIGmicro[iii].first;
		elem_micro_idx = intersection_table_BIGmicro[iii].second;

		const libMesh::Elem* elem_BIG = mesh_BIG.elem(elem_BIG_idx);
		const libMesh::Elem* elem_micro = mesh_micro.elem(elem_micro_idx);

		// And their dof maps
		dof_map_BIG.dof_indices(elem_BIG, dof_indices_BIG);
		dof_map_micro.dof_indices(elem_micro, dof_indices_micro);

		// Resize dense matrix, if needed
		if(!bSameElemsType)
		{
			// Then must resize the matrix
			n_dofs_BIG   = dof_indices_BIG.size();
			n_dofs_micro   = dof_indices_micro.size();
			Me.resize (n_dofs_BIG, n_dofs_micro);
		}

		Me.zero();

		// Now iterate over the intersections
		auto inter_idx_range = intersection_table_inter.equal_range(iii);

		for(	it_inter_idx = inter_idx_range.first;
				it_inter_idx != inter_idx_range.second;
				++it_inter_idx)
		{
			// Get the intersection mesh pointer
			elem_inter_idx = it_inter_idx->second;

			const libMesh::Elem* elem_inter = mesh_inter.elem(elem_inter_idx);

			// Restart the elements
			fe_inter->reinit(elem_inter);

			fe_BIG->reinit(elem_BIG,&quad_points_inter);
			fe_micro->reinit(elem_micro,&quad_points_inter);

			// For each quadrature point determinate the sub-matrices elements
			for (unsigned int qp=0; qp<qrule_inter.n_points(); qp++)
			{
				// Internal tension
				L2_Coupling(Me,qp,phi_BIG,phi_micro,
							n_dofs_BIG,n_dofs_micro,JxW);
			}
		}

		couplingMatrix.add_matrix(Me,dof_indices_BIG,dof_indices_micro);
	}
};

void assemble_coupling_matrix(	libMesh::EquationSystems& BIG_eq_system,
								libMesh::EquationSystems& micro_eq_system,
								libMesh::EquationSystems& inter_eq_system,
								libMesh::SparseMatrix<libMesh::Number>& couplingMatrix,
								bool bSameElemsType)
{
	// Pointers to the meshes
	const libMesh::MeshBase& mesh_BIG = BIG_eq_system.get_mesh();
	const libMesh::MeshBase& mesh_micro = micro_eq_system.get_mesh();
	const libMesh::MeshBase& mesh_inter = inter_eq_system.get_mesh();

	const unsigned int dim_BIG = mesh_BIG.mesh_dimension();
	const unsigned int dim_micro = mesh_micro.mesh_dimension();
	const unsigned int dim_inter = mesh_inter.mesh_dimension();

	// Systems and vars
	libMesh::LinearImplicitSystem& volume_BIG_system
		= BIG_eq_system.get_system<libMesh::LinearImplicitSystem>("VolTest");
	libMesh::LinearImplicitSystem& volume_micro_system
		= micro_eq_system.get_system<libMesh::LinearImplicitSystem>("VolTest");
	libMesh::LinearImplicitSystem& volume_inter_system
		= inter_eq_system.get_system<libMesh::LinearImplicitSystem>("VolTest");

	const unsigned int n_components = 1;
	const unsigned int vol_test_BIG = volume_BIG_system.variable_number("SillyVar");
	const unsigned int vol_test_micro = volume_micro_system.variable_number ("SillyVar");
	const unsigned int vol_test_inter = volume_inter_system.variable_number ("SillyVar");

	// DoF maps
	const libMesh::DofMap& dof_map_BIG = volume_BIG_system.get_dof_map();
	const libMesh::DofMap& dof_map_micro = volume_micro_system.get_dof_map();
	const libMesh::DofMap& dof_map_inter = volume_inter_system.get_dof_map();

	libMesh::FEType fe_type_BIG = dof_map_BIG.variable_type(vol_test_BIG);
	libMesh::FEType fe_type_micro = dof_map_micro.variable_type(vol_test_micro);
	libMesh::FEType fe_type_inter = dof_map_inter.variable_type(vol_test_inter);

	// Set up FE bases and quadratures
	// TODO : verify if the micro's quadrature is really needed
	libMesh::UniquePtr<libMesh::FEBase> fe_BIG (libMesh::FEBase::build(dim_BIG, fe_type_BIG));
	libMesh::UniquePtr<libMesh::FEBase> fe_micro (libMesh::FEBase::build(dim_micro, fe_type_micro));
	libMesh::UniquePtr<libMesh::FEBase> fe_inter (libMesh::FEBase::build(dim_inter, fe_type_inter));

	libMesh::QGauss qrule_BIG(dim_BIG, fe_type_BIG.default_quadrature_order());
	fe_BIG->attach_quadrature_rule (&qrule_BIG);

	libMesh::QGauss qrule_inter(dim_inter, fe_type_inter.default_quadrature_order());
	fe_inter->attach_quadrature_rule (&qrule_inter);

//	QGauss qrule_micro(dim_micro, fe_type_micro.default_quadrature_order());
//	fe_micro->attach_quadrature_rule (&qrule_micro);

	// Jacobian -> only the BIG one is needed
	const std::vector<libMesh::Real>& JxW = fe_inter->get_JxW();

	// Shape functions
	const std::vector<std::vector<libMesh::Real> >& phi_BIG = fe_BIG->get_phi();
	const std::vector<std::vector<libMesh::Real> >& phi_micro = fe_micro->get_phi();

	std::vector<libMesh::dof_id_type> dof_indices_BIG;
	std::vector<libMesh::dof_id_type> dof_indices_micro;

	libMesh::MeshBase::const_element_iterator       el_BIG     = mesh_BIG.active_local_elements_begin();
	const libMesh::MeshBase::const_element_iterator end_el_BIG = mesh_BIG.active_local_elements_end();

	libMesh::MeshBase::const_element_iterator       el_micro     = mesh_micro.active_local_elements_begin();
	const libMesh::MeshBase::const_element_iterator end_el_micro = mesh_micro.active_local_elements_end();

	libMesh::MeshBase::const_element_iterator       el_inter     = mesh_inter.active_local_elements_begin();
	const libMesh::MeshBase::const_element_iterator end_el_inter = mesh_inter.active_local_elements_end();

	// Local matrix
	libMesh::DenseMatrix<libMesh::Number> Me;
	unsigned int n_dofs_BIG;
	unsigned int n_dofs_micro;

	if(bSameElemsType)
	{
		const libMesh::Elem* elem_BIG = *el_BIG;
		dof_map_BIG.dof_indices(elem_BIG, dof_indices_BIG);
		n_dofs_BIG   = dof_indices_BIG.size();

		const libMesh::Elem* elem_micro = *el_micro;
		dof_map_micro.dof_indices(elem_micro, dof_indices_micro);
		n_dofs_micro   = dof_indices_micro.size();

		Me.resize (n_dofs_BIG, n_dofs_micro);
	}

	// Vector that will keep the quadrature points
	const std::vector<libMesh::Point>& quad_points_inter = fe_inter->get_xyz();

	// Initialize global matrix
	const unsigned int BIG_M = dof_map_BIG.n_dofs();
	const unsigned int micro_N = dof_map_micro.n_dofs();

	const unsigned int BIG_M_local = dof_map_BIG.n_local_dofs();
	const unsigned int micro_N_local = dof_map_micro.n_local_dofs();

	couplingMatrix.init(BIG_M,micro_N,BIG_M_local,micro_N_local);

	// For each element in the big mesh
	for ( ; el_BIG != end_el_BIG; ++el_BIG)
	{
		// Get its pointer
		const libMesh::Elem* elem_BIG = *el_BIG;

		// And its dof map
		dof_map_BIG.dof_indices(elem_BIG, dof_indices_BIG);

		// For each element in the micro mesh
		for( ; el_micro != end_el_micro; ++el_micro)
		{
			// Get its pointer
			const libMesh::Elem* elem_micro = *el_micro;

			// And its dof map
			dof_map_micro.dof_indices(elem_micro, dof_indices_micro);

			// Resize dense matrix, if needed
			if(!bSameElemsType)
			{
				// Then must resize the matrix
				n_dofs_BIG   = dof_indices_BIG.size();
				n_dofs_micro   = dof_indices_micro.size();
				Me.resize (n_dofs_BIG, n_dofs_micro);
			}

			// TODO : build intersection

			// For each element in the intersection mesh
			for( ; el_inter != end_el_inter; ++el_inter)
			{
				// Restart the elements
				const libMesh::Elem* elem_inter = *el_inter;

				fe_inter->reinit(elem_inter);

				fe_BIG->reinit(elem_BIG,&quad_points_inter);
				fe_micro->reinit(elem_micro,&quad_points_inter);

				// For each quadrature point determinate the sub-matrices elements
				for (unsigned int qp=0; qp<qrule_inter.n_points(); qp++)
				{
					// Internal tension
					L2_Coupling(Me,qp,phi_BIG,phi_micro,
								n_dofs_BIG,n_dofs_micro,JxW);
				}
			}

			couplingMatrix.add_matrix(Me,dof_indices_BIG,dof_indices_micro);
		}
	}
};

libMesh::Real kronecker_delta(unsigned int i,
				   unsigned int j)
{
	return i == j ? 1. : 0.;
}