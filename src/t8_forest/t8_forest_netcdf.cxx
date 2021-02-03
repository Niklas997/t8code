/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element classes in parallel.

  Copyright (C) 2015 the developers

  t8code is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  t8code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with t8code; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

/*
Description:
These functions write a file in the NetCDF-format which represents the given 2D- or 3D-cmesh
*/

#include <t8.h>
#if T8_WITH_NETCDF
#include <netcdf.h>
/* do we need to question wheter this header is accessible? */
#include <netcdf_par.h>
/* Standard netcdf error function */
#define ERRCODE 2
#define ERR(e) {t8_global_productionf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}
#endif
#include <t8_element_cxx.hxx>
#include <t8_forest.h>
#include <t8_forest_netcdf.h>
#include <t8_element_shape.h>
//#include <t8_data/t8_containers.h>

T8_EXTERN_C_BEGIN ();

/* Contains all Variables used in order to work with the NetCDF-File */
typedef struct
{
  const char         *filename;
  const char         *filetitle;
  int                 dim;
  t8_gloidx_t         nMesh_elem;
  t8_gloidx_t         nMesh_node;
  t8_gloidx_t         nMesh_local_node;
  int                 nMaxMesh_elem_nodes;
  /* Declaring NetCDF-dimension ids */
  int                 nMesh_elem_dimid;
  int                 nMaxMesh_elem_nodes_dimid;
  int                 nMesh_node_dimid;
  /* Declaring NetCDF-variables ids */
  int                 ncid;
  int                 var_elem_tree_id;
  int                 var_elem_types_id;
  int                 var_elem_nodes_id;
  int                 var_mesh_id;
  int                 var_node_x_id;
  int                 var_node_y_id;
  int                 var_node_z_id;
  int                 dimids[2];        /* contains two NetCDF-dimensions in order to declare two-dimensional NetCDF-variables */
  /* Variables used for default NetCDF purposes */
  int                 fillvalue;
  int                 start_index;
  const char         *convention;
  /* Stores the old NetCDF-FillMode if it gets changed */
  int                 old_fill_mode;

} t8_forest_netcdf_context_t;

/* Contains the Definitions for the NetCDF-dimensions/-variables/-attributes (vary whether a 2D or 3D Mesh will be outputted) */
typedef struct
{
  const char         *mesh;
  const char         *dim_nMesh_node;
  const char         *dim_nMesh_elem;
  const char         *dim_nMaxMesh_elem_nodes;
  const char         *var_Mesh_node_x;
  const char         *var_Mesh_node_y;
  const char         *var_Mesh_node_z;
  const char         *var_Mesh_elem_types;
  const char         *var_Mesh_elem_tree_id;
  const char         *var_Mesh_elem_node;
  const char         *att_elem_shape_type;
  const char         *att_elem_node_connectivity;
  const char         *att_elem_tree_id;
  const char         *att_elem_node;
} t8_forest_netcdf_ugrid_namespace_t;

static void
t8_forest_init_ugrid_namespace_context (t8_forest_netcdf_ugrid_namespace_t *
                                        namespace_conv, int dim)
{
  if (dim == 2) {
    namespace_conv->mesh = "Mesh2";
    namespace_conv->dim_nMesh_node = "nMesh2_node";
    namespace_conv->dim_nMesh_elem = "nMesh2_face";
    namespace_conv->dim_nMaxMesh_elem_nodes = "nMaxMesh2_face_nodes";
    namespace_conv->var_Mesh_node_x = "Mesh2_node_x";
    namespace_conv->var_Mesh_node_y = "Mesh2_node_y";
    namespace_conv->var_Mesh_node_z = "Mesh2_node_z";
    namespace_conv->var_Mesh_elem_types = "Mesh2_face_types";
    namespace_conv->var_Mesh_elem_tree_id = "Mesh2_face_tree_id";
    namespace_conv->var_Mesh_elem_node = "Mesh2_face_nodes";
    namespace_conv->att_elem_shape_type = "face_shape_type";
    namespace_conv->att_elem_node_connectivity = "face_node_conectivity";
    namespace_conv->att_elem_tree_id = "face_tree_id";
    namespace_conv->att_elem_node = "Mesh2_node_x Mesh2_node_y Mesh2_node_z";

  }
  else if (dim == 3) {
    namespace_conv->mesh = "Mesh3D";
    namespace_conv->dim_nMesh_node = "nMesh3D_node";
    namespace_conv->dim_nMesh_elem = "nMesh3D_vol";
    namespace_conv->dim_nMaxMesh_elem_nodes = "nMaxMesh3D_vol_nodes";
    namespace_conv->var_Mesh_node_x = "Mesh3D_node_x";
    namespace_conv->var_Mesh_node_y = "Mesh3D_node_y";
    namespace_conv->var_Mesh_node_z = "Mesh3D_node_z";
    namespace_conv->var_Mesh_elem_types = "Mesh3D_vol_types";
    namespace_conv->var_Mesh_elem_tree_id = "Mesh3D_vol_tree_id";
    namespace_conv->var_Mesh_elem_node = "Mesh3D_vol_nodes";
    namespace_conv->att_elem_shape_type = "volume_shape_type";
    namespace_conv->att_elem_node_connectivity = "volume_node_connectivity";
    namespace_conv->att_elem_tree_id = "volume_tree_id";
    namespace_conv->att_elem_node =
      "Mesh3D_node_x Mesh3D_node_y Mesh3D_node_z";
  }
}

/* Define NetCDF-dimesnions */
static void
t8_forest_write_netcdf_dimensions (t8_forest_netcdf_context_t * context,
                                   t8_forest_netcdf_ugrid_namespace_t *
                                   namespace_context)
{
#if T8_WITH_NETCDF
  /* *Define dimensions in the NetCDF file.* */

  /* Return value in order to check NetCDF commands */
  int                 retval;
  /* Define dimension: number of elements */
  if ((retval =
       nc_def_dim (context->ncid, namespace_context->dim_nMesh_elem,
                   context->nMesh_elem, &context->nMesh_elem_dimid))) {
    ERR (retval);
  }

  /* Define dimension: maximum node number per element */
  if ((retval =
       nc_def_dim (context->ncid, namespace_context->dim_nMaxMesh_elem_nodes,
                   context->nMaxMesh_elem_nodes,
                   &context->nMaxMesh_elem_nodes_dimid))) {
    ERR (retval);
  }

  /* Store the ID of the dimensions. */
  context->dimids[0] = context->nMesh_elem_dimid;
  context->dimids[1] = context->nMaxMesh_elem_nodes_dimid;

  t8_debugf ("First NetCDF-dimensions were defined.\n");
#endif
}

/* Define NetCDF-variables */
static void
t8_forest_write_netcdf_variables (t8_forest_netcdf_context_t * context,
                                  t8_forest_netcdf_ugrid_namespace_t *
                                  namespace_context)
{
#if T8_WITH_NETCDF
  /* *Define variables in the NetCDF file.* */

  /* Return value in order to check NetCDF commands */
  int                 retval;

  /* Define a general describing Mesh-variable */
  if ((retval =
       nc_def_var (context->ncid, namespace_context->mesh, NC_INT, 0, 0,
                   &context->var_mesh_id))) {
    ERR (retval);
  }

  /* Define cf_role attribute */
  const char         *role_mesh = "mesh_topology";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_mesh_id, "cf_role",
                        strlen (role_mesh), role_mesh))) {
    ERR (retval);
  }

  /* Define long_name attribute. */
  const char         *long_mesh =
    "Topology data of unstructured tree-based mesh";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_mesh_id, "long_name",
                        strlen (long_mesh), long_mesh))) {
    ERR (retval);
  }

  /* Define topology_dimension attribute */
  if ((retval =
       nc_put_att_int (context->ncid, context->var_mesh_id,
                       "topology_dimension", NC_INT, 1, &context->dim))) {
    ERR (retval);
  }

  /* Define node_coordinates attribute */
  if ((retval =
       nc_put_att_text (context->ncid, context->var_mesh_id,
                        "node_coordinates",
                        strlen (namespace_context->att_elem_node),
                        namespace_context->att_elem_node))) {
    ERR (retval);
  }
  /* Define elem_shape_type attribute */
  if ((retval =
       nc_put_att_text (context->ncid, context->var_mesh_id,
                        namespace_context->att_elem_shape_type,
                        strlen (namespace_context->var_Mesh_elem_types),
                        namespace_context->var_Mesh_elem_types))) {
    ERR (retval);
  }
  /* Define elem_node_connectivity attribute */
  if ((retval =
       nc_put_att_text (context->ncid, context->var_mesh_id,
                        namespace_context->att_elem_node_connectivity,
                        strlen (namespace_context->var_Mesh_elem_node),
                        namespace_context->var_Mesh_elem_node))) {
    ERR (retval);
  }
  /* Define elem_tree_id attribute */
  if ((retval =
       nc_put_att_text (context->ncid, context->var_mesh_id,
                        namespace_context->att_elem_tree_id,
                        strlen (namespace_context->var_Mesh_elem_tree_id),
                        namespace_context->var_Mesh_elem_tree_id))) {
    ERR (retval);
  }
  /*************************************************************************/

  /* Define the element-type variable in the NetCDF-file. */
  if ((retval =
       nc_def_var (context->ncid, namespace_context->var_Mesh_elem_types,
                   NC_INT, 1, &context->nMesh_elem_dimid,
                   &context->var_elem_types_id))) {
    ERR (retval);
  }
  /* Define cf_role attribute */
  if ((retval =
       nc_put_att_text (context->ncid, context->var_elem_types_id, "cf_role",
                        strlen (namespace_context->att_elem_shape_type),
                        namespace_context->att_elem_shape_type))) {
    ERR (retval);
  }
  /* Define long_name attribute. */
  const char         *long_elem_types = "Specifies the shape of the elements";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_elem_types_id,
                        "long_name", strlen (long_elem_types),
                        long_elem_types))) {
    ERR (retval);
  }
  /* Define _FillValue attribute */
  if ((retval =
       nc_put_att_int (context->ncid, context->var_elem_types_id,
                       "_FillValue", NC_INT, 1, &context->fillvalue))) {
    ERR (retval);
  }
  /* Define start_index attribute. */
  if ((retval =
       nc_put_att_int (context->ncid, context->var_elem_types_id,
                       "start_index", NC_INT, 1, &context->start_index))) {
    ERR (retval);
  }

  /*************************************************************************/

  /* Define the element-tree_id variable. */
  if ((retval =
       nc_def_var (context->ncid, namespace_context->var_Mesh_elem_tree_id,
                   NC_INT, 1, &context->nMesh_elem_dimid,
                   &context->var_elem_tree_id))) {
    ERR (retval);
  }
  /* Define cf_role attribute */
  if ((retval =
       nc_put_att_text (context->ncid, context->var_elem_tree_id, "cf_role",
                        strlen (namespace_context->att_elem_tree_id),
                        namespace_context->att_elem_tree_id))) {
    ERR (retval);
  }
  /* Define long_name attribute. */
  const char         *long_elem_prop = "Lists each elements tree_id";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_elem_tree_id, "long_name",
                        strlen (long_elem_prop), long_elem_prop))) {
    ERR (retval);
  }
  /* Define _FillValue attribute */
  if ((retval =
       nc_put_att_int (context->ncid, context->var_elem_tree_id, "_FillValue",
                       NC_INT, 1, &context->fillvalue))) {
    ERR (retval);
  }
  /* Define start_index attribute. */
  if ((retval =
       nc_put_att_int (context->ncid, context->var_elem_tree_id,
                       "start_index", NC_INT, 1, &context->start_index))) {
    ERR (retval);
  }

  /*************************************************************************/

  /* Define the element-nodes variable. */
  if ((retval =
       nc_def_var (context->ncid, namespace_context->var_Mesh_elem_node,
                   NC_INT, 2, context->dimids,
                   &context->var_elem_nodes_id))) {
    ERR (retval);
  }
  /* Define cf_role attribute */
  if ((retval =
       nc_put_att_text (context->ncid, context->var_elem_nodes_id, "cf_role",
                        strlen
                        (namespace_context->att_elem_node_connectivity),
                        namespace_context->att_elem_node_connectivity))) {
    ERR (retval);
  }
  /* Define long_name attribute. */
  const char         *long_elem_nodes =
    "Lists the corresponding nodes to each element";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_elem_nodes_id,
                        "long_name", strlen (long_elem_nodes),
                        long_elem_nodes))) {
    ERR (retval);
  }
  /* Define _FillValue attribute */
  if ((retval =
       nc_put_att_int (context->ncid, context->var_elem_nodes_id,
                       "_FillValue", NC_INT, 1, &context->fillvalue))) {
    ERR (retval);
  }
  /* Define start_index attribute. */
  if ((retval =
       nc_put_att_int (context->ncid, context->var_elem_nodes_id,
                       "start_index", NC_INT, 1, &context->start_index))) {
    ERR (retval);
  }
#endif
}

static void
t8_forest_write_netcdf_data (t8_forest_t forest,
                             t8_forest_netcdf_context_t * context)
{
#if T8_WITH_NETCDF
  t8_eclass_t         tree_class;
  t8_gloidx_t         gtree_id;
  t8_locidx_t         num_local_trees;
  t8_locidx_t         num_local_elements;
  t8_locidx_t         ltree_id;
  t8_locidx_t         local_elem_id;
  t8_locidx_t         num_local_tree_elem;
  t8_element_shape_t  element_shape;
  t8_element_t       *element;
  t8_locidx_t         local_tree_offset;
  t8_gloidx_t         first_local_elem_id;
  t8_gloidx_t         num_local_nodes;
  t8_gloidx_t         num_nodes;
  int                 retval;

  /* Get the first local element id in a forest (function is collective) */
  first_local_elem_id = t8_forest_get_first_local_element_id (forest);

  /* Get number of local trees. */
  num_local_trees = t8_forest_get_num_local_trees (forest);

  /* Ger number of local elements */
  num_local_elements = t8_forest_get_num_element (forest);
#if 0
  /* Declare variables with their proper dimensions. */
  int                *Mesh_elem_types =
    (int *) T8_ALLOC (int, context->nMesh_elem);
  int                *Mesh_elem_tree_id =
    (int *) T8_ALLOC (int, context->nMesh_elem);
#endif

  /* Declare variables with their proper dimensions. */
  int                *Mesh_elem_types =
    (int *) T8_ALLOC (int, num_local_elements);
  int                *Mesh_elem_tree_id =
    (int *) T8_ALLOC (int, num_local_elements);

  /* Check if pointers are not NULL. */
  T8_ASSERT (Mesh_elem_types != NULL && Mesh_elem_tree_id != NULL);

  /* Counts the number of nodes */
  num_local_nodes = 0;
  /* Iterate over all local trees and their respective elements */
  for (ltree_id = 0; ltree_id < num_local_trees; ltree_id++) {
    num_local_tree_elem = t8_forest_get_tree_num_elements (forest, ltree_id);
    tree_class = t8_forest_get_tree_class (forest, ltree_id);
    /* Computing the local tree offest */
    local_tree_offset = t8_forest_get_tree_element_offset (forest, ltree_id);
    /* Iterate over all local elements in the local tree */
    for (local_elem_id = 0; local_elem_id < num_local_tree_elem;
         local_elem_id++) {
      /* Get the eclass scheme */
      t8_eclass_scheme_c *scheme =
        t8_forest_get_eclass_scheme (forest, tree_class);
      /* Get the local element in the local tree */
      element =
        t8_forest_get_element_in_tree (forest, ltree_id, local_elem_id);
      /* Determine the element shape */
      element_shape = scheme->t8_element_shape (element);
      /* Store the type of the element in its global index position */
      Mesh_elem_types[(local_tree_offset + local_elem_id)] =
        t8_element_shape_vtk_type (element_shape);
      /* Store the elements tree_id in its global index position */
      Mesh_elem_tree_id[(local_tree_offset + local_elem_id)] =
        t8_forest_global_tree_id (forest, ltree_id);
      /* Adding the number of corners of this elements shape to the counter */
      num_local_nodes += t8_element_shape_num_vertices (element_shape);
    }
  }
  /* Write the data in the corresponding NetCDF-variable. */
  /* Fill the 'Mesh_elem_types'-variable. */
#if 0
  if ((retval =
       nc_put_var_int (context->ncid, context->var_elem_types_id,
                       &Mesh_elem_types[0]))) {
    ERR (retval);
  }
  /* Fill the 'Mesh_elem_tree_id'-variable. */
  if ((retval =
       nc_put_var_int (context->ncid, context->var_elem_tree_id,
                       &Mesh_elem_tree_id[0]))) {
    ERR (retval);
  }
#endif
  size_t              start_ptr = (size_t) first_local_elem_id;
  size_t              count_ptr = (size_t) num_local_elements;
  if ((retval =
       nc_put_vara_int (context->ncid, context->var_elem_types_id, &start_ptr,
                        &count_ptr, &Mesh_elem_types[0]))) {
    ERR (retval);
  }
  /* Fill the 'Mesh_elem_tree_id'-variable. */
  if ((retval =
       nc_put_vara_int (context->ncid, context->var_elem_tree_id, &start_ptr,
                        &count_ptr, &Mesh_elem_tree_id[0]))) {
    ERR (retval);
  }
  /* Free the allocated memory */
  T8_FREE (Mesh_elem_types);
  T8_FREE (Mesh_elem_tree_id);

  /* Store the number of local nodes */
  context->nMesh_local_node = num_local_nodes;
  /* Gather the number of all global nodes */
  retval =
    sc_MPI_Allreduce (&num_local_nodes, &num_nodes, 1, sc_MPI_LONG_LONG_INT,
                      sc_MPI_SUM, sc_MPI_COMM_WORLD);
  SC_CHECK_MPI (retval);

  /* After counting the number of nodes, the  NetCDF-dimension 'nMesh_node' can be created => Store the 'nMesh_node' dimension */
  context->nMesh_node = num_nodes;

#endif
}

/* Define  NetCDF-coordinate-dimension */
static void
t8_forest_write_netcdf_coordinate_dimension (t8_forest_netcdf_context_t *
                                             context,
                                             t8_forest_netcdf_ugrid_namespace_t
                                             * namespace_context)
{
#if T8_WITH_NETCDF
  /* Define dimension: number of nodes */
  int                 retval;
  if ((retval =
       nc_def_dim (context->ncid, namespace_context->dim_nMesh_node,
                   context->nMesh_node, &context->nMesh_node_dimid))) {
    ERR (retval);
  }
#endif
}

/* Define NetCDF-coordinate-variables */
static void
t8_forest_write_netcdf_coordinate_variables (t8_forest_netcdf_context_t *
                                             context,
                                             t8_forest_netcdf_ugrid_namespace_t
                                             * namespace_context)
{
#if T8_WITH_NETCDF
  /* Define the Mesh_node_x  variable. */
  int                 retval;
  if ((retval =
       nc_def_var (context->ncid, namespace_context->var_Mesh_node_x,
                   NC_DOUBLE, 1, &context->nMesh_node_dimid,
                   &context->var_node_x_id))) {
    ERR (retval);
  }
  /* Define standard_name attribute. */
  const char         *standard_node_x = "Longitude";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_node_x_id,
                        "standard_name", strlen (standard_node_x),
                        standard_node_x))) {
    ERR (retval);
  }
  /* Define long_name attribute. */
  const char         *long_node_x = "Longitude of mesh nodes";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_node_x_id, "long_name",
                        strlen (long_node_x), long_node_x))) {
    ERR (retval);
  }
  /* Define units attribute. */
  const char         *units_node_x = "degrees_east";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_node_x_id, "units",
                        strlen (units_node_x), units_node_x))) {
    ERR (retval);
  }

        /*********************************************/

  /* Define the Mesh_node_y variable. */
  if ((retval =
       nc_def_var (context->ncid, namespace_context->var_Mesh_node_y,
                   NC_DOUBLE, 1, &context->nMesh_node_dimid,
                   &context->var_node_y_id))) {
    ERR (retval);
  }
  /* Define standard_name attribute. */
  const char         *standard_node_y = "Latitude";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_node_y_id,
                        "standard_name", strlen (standard_node_y),
                        standard_node_y))) {
    ERR (retval);
  }
  /* Define long_name attribute. */
  const char         *long_node_y = "Latitude of mesh nodes";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_node_y_id, "long_name",
                        strlen (long_node_y), long_node_y))) {
    ERR (retval);
  }
  /* Define units attribute. */
  const char         *units_node_y = "degrees_north";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_node_y_id, "units",
                        strlen (units_node_y), units_node_y))) {
    ERR (retval);
  }

        /*********************************************/

  /* Define the Mesh_node_z variable. */
  if ((retval =
       nc_def_var (context->ncid, namespace_context->var_Mesh_node_z,
                   NC_DOUBLE, 1, &context->nMesh_node_dimid,
                   &context->var_node_z_id))) {
    ERR (retval);
  }
  /* Define standard_name attribute. */
  const char         *standard_node_z = "Height";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_node_z_id,
                        "standard_name", strlen (standard_node_z),
                        standard_node_z))) {
    ERR (retval);
  }
  /* Define long_name attribute. */
  const char         *long_node_z = "Elevation of mesh nodes";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_node_z_id, "long_name",
                        strlen (long_node_z), long_node_z))) {
    ERR (retval);
  }
  /* Define units attribute. */
  const char         *units_node_z = "m";
  if ((retval =
       nc_put_att_text (context->ncid, context->var_node_z_id, "units",
                        strlen (units_node_z), units_node_z))) {
    ERR (retval);
  }
#endif
}

/* Declare the user-defined elementwise NetCDF-variables which were passed to function. */
static void
t8_forest_write_user_netcdf_vars (t8_forest_netcdf_context_t * context,
                                  t8_forest_netcdf_ugrid_namespace_t *
                                  namespace_context,
                                  int num_extern_netcdf_vars,
                                  t8_netcdf_variable_t * ext_variables[])
{
#if T8_WITH_NETCDF
  int                 retval;
  /* Check wheter user-defined variables should be written */
  if (num_extern_netcdf_vars > 0 && ext_variables != NULL) {
    for (int i = 0; i < num_extern_netcdf_vars; i++) {
      switch (ext_variables[i]->datatype) {
      case 0:
        /* A NetCDF Integer-Variable will be declared */
        if ((retval =
             nc_def_var (context->ncid, ext_variables[i]->variable_name,
                         NC_INT, 1, &context->nMesh_elem_dimid,
                         &(ext_variables[i]->var_user_dimid)))) {
          ERR (retval);
        }
        break;
      case 1:
        /* A NetCDF Double-Variable will be declared */
        if ((retval =
             nc_def_var (context->ncid, ext_variables[i]->variable_name,
                         NC_DOUBLE, 1, &context->nMesh_elem_dimid,
                         &(ext_variables[i]->var_user_dimid)))) {
          ERR (retval);
        }
        break;
      default:
        //this variable is set internally and not by the user
        //t8_errorf("The NetCDF datatype is not corresponding to a variable type\n");
        break;
      }
      /* Attach the user-defined 'long_name' attribute to the variable */
      if ((retval =
           nc_put_att_text (context->ncid, (ext_variables[i]->var_user_dimid),
                            "long_name",
                            strlen (ext_variables[i]->variable_long_name),
                            ext_variables[i]->variable_long_name))) {
        ERR (retval);
      }
      /* Attach the user-defined 'units' attribute to the variable */
      if ((retval =
           nc_put_att_text (context->ncid, (ext_variables[i]->var_user_dimid),
                            "units",
                            strlen (ext_variables[i]->variable_long_name),
                            ext_variables[i]->variable_long_name))) {
        ERR (retval);
      }
    }
  }
#endif
}

static void
t8_forest_write_netcdf_coordinate_data (t8_forest_t forest,
                                        t8_forest_netcdf_context_t * context)
{
#if T8_WITH_NETCDF
  double             *vertex_coords = (double *) T8_ALLOC (double, 3);
  double             *vertices;
  t8_eclass_t         tree_class;
  t8_locidx_t         num_local_trees;
  t8_locidx_t         ltree_id = 0;
  t8_locidx_t         num_local_elements;
  t8_locidx_t         num_local_tree_elem;
  t8_locidx_t         local_elem_id;
  t8_locidx_t         local_tree_offset;
  t8_element_t       *element;
  t8_element_shape_t  element_shape;
  t8_gloidx_t         first_local_elem_id;

  t8_gloidx_t         num_it = 0;
  int                 retval;
  int                 mpisize, mpirank;
  size_t              start_ptr = 0;
  size_t              count_ptr;

  /* Get the first local element id in a forest (function is collective) */
  first_local_elem_id = t8_forest_get_first_local_element_id (forest);

  /* Get the size of the MPI_Comm and the process local rank */
  retval = sc_MPI_Comm_size (sc_MPI_COMM_WORLD, &mpisize);
  SC_CHECK_MPI (retval);
  retval = sc_MPI_Comm_rank (sc_MPI_COMM_WORLD, &mpirank);
  SC_CHECK_MPI (retval);

  /* Get number of local trees. */
  num_local_trees = t8_forest_get_num_local_trees (forest);

  /* Ger number of local elements */
  num_local_elements = t8_forest_get_num_element (forest);

  /* Allocate memory for node offsets */
  t8_gloidx_t        *node_offset[mpisize];
  /* Get the number of all nodes local to each rank */
  retval =
    sc_MPI_Allgather (&context->nMesh_local_node, 1, sc_MPI_LONG_LONG_INT,
                      node_offset, 1, sc_MPI_LONG_LONG_INT,
                      sc_MPI_COMM_WORLD);
  SC_CHECK_MPI (retval);
  /*Calculate the global number of element nodes in the previous trees */
  for (int j = 0; j < mpirank; j++) {
    start_ptr += (size_t) node_offset[j];
  }

#if 0
  /* Allocate the Variable-data that will be put out in the NetCDF variables */
  size_t              num_elements = (size_t) (context->nMesh_elem);
  size_t              num_max_nodes_per_elem =
    (size_t) (context->nMaxMesh_elem_nodes);
  size_t              num_nodes = (size_t) (context->nMesh_node);

  int                *Mesh_elem_nodes =
    (int *) T8_ALLOC (int, num_elements * num_max_nodes_per_elem);
  double             *Mesh_node_x = (double *) T8_ALLOC (double, num_nodes);
  double             *Mesh_node_y = (double *) T8_ALLOC (double, num_nodes);
  double             *Mesh_node_z = (double *) T8_ALLOC (double, num_nodes);
#endif

  /* Allocate the Variable-data that will be put out in the NetCDF variables */
  size_t              num_elements = (size_t) num_local_elements;
  size_t              num_max_nodes_per_elem =
    (size_t) (context->nMaxMesh_elem_nodes);
  size_t              num_nodes = (size_t) (context->nMesh_local_node);

  int                *Mesh_elem_nodes =
    (int *) T8_ALLOC (int, num_elements * num_max_nodes_per_elem);
  double             *Mesh_node_x = (double *) T8_ALLOC (double, num_nodes);
  double             *Mesh_node_y = (double *) T8_ALLOC (double, num_nodes);
  double             *Mesh_node_z = (double *) T8_ALLOC (double, num_nodes);
  /* Check if pointers are not NULL. */
  T8_ASSERT (Mesh_node_x != NULL && Mesh_node_y != NULL
             && Mesh_node_z != NULL && Mesh_elem_nodes != NULL);

  /* Iterate over all local trees. */
  /* Corners should be stored in the same order as in a vtk-file (read that somewehere on a netcdf page). */
  int                 i;
  int                 number_nodes;
  for (ltree_id = 0; ltree_id < num_local_trees; ltree_id++) {
    num_local_tree_elem = t8_forest_get_tree_num_elements (forest, ltree_id);
    tree_class = t8_forest_get_tree_class (forest, ltree_id);
    /* Computing the local tree offest */
    local_tree_offset = t8_forest_get_tree_element_offset (forest, ltree_id);
    /* Get the array of vertex coordiantes of the local tree */
    vertices = t8_forest_get_tree_vertices (forest, ltree_id);
    for (local_elem_id = 0; local_elem_id < num_local_tree_elem;
         local_elem_id++) {
      /* Get the eclass scheme */
      t8_eclass_scheme_c *scheme =
        t8_forest_get_eclass_scheme (forest, tree_class);
      /* Get the local element in the local tree */
      element =
        t8_forest_get_element_in_tree (forest, ltree_id, local_elem_id);
      /* Determine the element shape */
      element_shape = scheme->t8_element_shape (element);
      /* Get the number of nodes for this elements shape */
      number_nodes = t8_element_shape_num_vertices (element_shape);
      i = 0;
      for (; i < number_nodes; i++) {
        t8_forest_element_coordinate (forest, ltree_id, element, vertices,
                                      t8_element_shape_vtk_corner_number ((int) element_shape, i), vertex_coords);
        /* Stores the x-, y- and z- coordinate of the nodes */
        Mesh_node_x[num_it] = vertex_coords[0];
        Mesh_node_y[num_it] = vertex_coords[1];
        Mesh_node_z[num_it] = vertex_coords[2];
        /* Stores the the nodes which correspond to this element. */
        Mesh_elem_nodes[(local_tree_offset +
                         local_elem_id) * (context->nMaxMesh_elem_nodes) +
                        i] = ((t8_gloidx_t) start_ptr) + num_it;
        num_it++;
      }
      for (; i < context->nMaxMesh_elem_nodes; i++) {
        /* Pre-fills the the elements corresponding nodes, if it is an element having less than nMaxMesh_elem_nodes. */
        Mesh_elem_nodes[(local_tree_offset +
                         local_elem_id) * (context->nMaxMesh_elem_nodes) +
                        i] = -1;
      }
    }
  }
  /* Free allocated memory */
  T8_FREE (vertex_coords);

  /* *Write the data into the NetCDF coordinate variables.* */

  /* Define a (2D) NetCDF-Hyperslab for filling the variable */
  const size_t        start_ptr_var[2] = { (size_t) first_local_elem_id, 0 };
  const size_t        count_ptr_var[2] =
    { num_elements, (size_t) context->nMaxMesh_elem_nodes };
  /* Fill the 'Mesh_elem_node'-variable. */
  if ((retval =
       nc_put_vara_int (context->ncid, context->var_elem_nodes_id,
                        start_ptr_var, count_ptr_var, &Mesh_elem_nodes[0]))) {
    ERR (retval);
  }

  /* Fill the space coordinate variables */
  count_ptr = (size_t) context->nMesh_local_node;
  /* Fill the 'Mesh_node_x'-variable. */
  if ((retval =
       nc_put_vara_double (context->ncid, context->var_node_x_id, &start_ptr,
                           &count_ptr, &Mesh_node_x[0]))) {
    ERR (retval);
  }
  /* Fill the 'Mesh_node_y'-variable. */
  if ((retval =
       nc_put_vara_double (context->ncid, context->var_node_y_id, &start_ptr,
                           &count_ptr, &Mesh_node_y[0]))) {
    ERR (retval);
  }
  /* Fill the 'Mesh_node_z'-variable. */
  if ((retval =
       nc_put_vara_double (context->ncid, context->var_node_z_id, &start_ptr,
                           &count_ptr, &Mesh_node_z[0]))) {
    ERR (retval);
  }

#if 0
  if ((retval =
       nc_put_var_int (context->ncid, context->var_elem_nodes_id,
                       &Mesh_elem_nodes[0]))) {
    ERR (retval);
  }
  /* Fill the 'Mesh_node_x'-variable. */
  if ((retval =
       nc_put_var_double (context->ncid, context->var_node_x_id,
                          &Mesh_node_x[0]))) {
    ERR (retval);
  }
  /* Fill the 'Mesh_node_y'-variable. */
  if ((retval =
       nc_put_var_double (context->ncid, context->var_node_y_id,
                          &Mesh_node_y[0]))) {
    ERR (retval);
  }
  /* Fill the 'Mesh_node_z'-variable. */
  if ((retval =
       nc_put_var_double (context->ncid, context->var_node_z_id,
                          &Mesh_node_z[0]))) {
    ERR (retval);
  }
#endif
  /* Free the allocated memory */
  T8_FREE (Mesh_node_x);
  T8_FREE (Mesh_node_y);
  T8_FREE (Mesh_node_z);
  T8_FREE (Mesh_elem_nodes);

#endif
}

/* Funcation that writes user-defined data to user-defined variables, if some were passed */
/* It is only possible to write exactly one value per element per variable */
static void
t8_forest_write_user_netcdf_data (t8_forest_t forest,
                                  t8_forest_netcdf_context_t * context,
                                  int num_extern_netcdf_vars,
                                  t8_netcdf_variable_t * ext_variables[])
{
#if T8_WITH_NETCDF
  if (num_extern_netcdf_vars > 0 && ext_variables != NULL) {
    t8_gloidx_t         gtree_id;
    t8_locidx_t         num_local_trees;
    t8_locidx_t         ltree_id;
    t8_locidx_t         local_tree_offset;
    int                 retval;
    size_t              start_ptr;
    size_t              count_ptr;

    /* Counters which imply the position in the NetCDF-variable where the data will be written, */
    start_ptr = (size_t) t8_forest_get_first_local_element_id (forest);
    count_ptr = (size_t) t8_forest_get_num_element (forest);

    /* Iterate over the amount of user-defined variables */
    for (int i = 0; i < num_extern_netcdf_vars; i++) {
      /* Check if exactly one value per element is given , a number of entries variable or something is needed */
      //T8_ASSERT(); 

      /* Fill the NetCDF-variable with the data */
      if (ext_variables[i]->datatype == 0) {
        /* If it is a Integer NetCDF-variable */
        if ((retval =
             nc_put_vara_int (context->ncid, ext_variables[i]->var_user_dimid,
                              &start_ptr, &count_ptr,
                              &((ext_variables[i]->netcdf_data_int)[0])))) {
          ERR (retval);
        }
      }
      else if (ext_variables[i]->datatype == 1) {
        /* If it is a Double NetCDF-variable */
        if ((retval =
             nc_put_vara_double (context->ncid,
                                 ext_variables[i]->var_user_dimid, &start_ptr,
                                 &count_ptr,
                                 &((ext_variables[i]->netcdf_data_double)
                                   [0])))) {
          ERR (retval);
        }
      }
    }
  }
#endif
}

/* Function that creates the NetCDF-File and fills it  */
static void
t8_forest_write_netcdf_file (t8_forest_t forest,
                             t8_forest_netcdf_context_t * context,
                             t8_forest_netcdf_ugrid_namespace_t *
                             namespace_context, int num_extern_netcdf_vars,
                             t8_netcdf_variable_t * ext_variables[])
{
#if T8_WITH_NETCDF
  t8_gloidx_t         num_glo_elem;
  int                 retval;

  /* Check if the forest was committed. */
  T8_ASSERT (t8_forest_is_committed (forest));

  /* Get the number of global elements in the forest. */
  num_glo_elem = t8_forest_get_global_num_elements (forest);

  /* Assign global number of elements. */
  context->nMesh_elem = num_glo_elem;
#if 0
  /* Create the NetCDF file, the NC_CLOBBER parameter tells netCDF to overwrite this file, if it already exists. Leaves the file in 'define-mode'. */
  if ((retval = nc_create (context->filename, NC_CLOBBER, &context->ncid))) {
    ERR (retval);
  }
#endif
  /* Create a parallel NetCDF-File */
  if ((retval =
       nc_create_par (context->filename, NC_CLOBBER | NC_NETCDF4
                      | NC_MPIIO, sc_MPI_COMM_WORLD, sc_MPI_INFO_NULL,
                      &context->ncid))) {
    ERR (retval);
  }
  t8_debugf ("NetCDf-file has been created.\n");

  /* Define the first NetCDF-dimensions (nMesh_node is not known yet) */
  t8_forest_write_netcdf_dimensions (context, namespace_context);

  /* Define NetCDF-variables */
  t8_forest_write_netcdf_variables (context, namespace_context);

  /* Disable the default fill-value-mode. */
  if ((retval =
       nc_set_fill (context->ncid, NC_NOFILL, &context->old_fill_mode))) {
    ERR (retval);
  }

  /* *Define global attributes* */

  /* Define title attribute */
  if ((retval =
       nc_put_att_text (context->ncid, NC_GLOBAL, "title",
                        strlen (context->filetitle), context->filetitle))) {
    ERR (retval);
  }
  /* Define convention attribute */
  if ((retval =
       nc_put_att_text (context->ncid, NC_GLOBAL, "convention",
                        strlen (context->convention), context->convention))) {
    ERR (retval);
  }

  /* MPI-Barrier */
  retval = sc_MPI_Barrier (sc_MPI_COMM_WORLD);

  /* End define-mode. NetCDF-file enters data-mode. */
  if ((retval = nc_enddef (context->ncid))) {
    ERR (retval);
  }

  /* Fill the already defined NetCDF-variables and calculate the 'nMesh_node' (global number of nodes) -dimension */
  t8_forest_write_netcdf_data (forest, context);        /* loop ueber localtree_id und weiteren loop ueber num_local_elements und dann mit ltree_id undLoc_elem_id daten holen */

  /* Leave the NetCDF-data-mode and re-enter the define-mode. */
  if ((retval = nc_redef (context->ncid))) {
    ERR (retval);
  }

  /* Define the NetCDF-dimension 'nMesh_node' */
  t8_forest_write_netcdf_coordinate_dimension (context, namespace_context);

  /* Define the NetCDF-coordinate variables */
  t8_forest_write_netcdf_coordinate_variables (context, namespace_context);

  /* Eventuallay declare user-defined elementwise NetCDF-variables, if some were passed */
  t8_forest_write_user_netcdf_vars (context, namespace_context,
                                    num_extern_netcdf_vars, ext_variables);

#if 0
  /* Write the NetCDF coordinate data into the variables */
  //t8_forest_write_netcdf_coordinate_data(forest, context, namespace_context);
#endif
  /* Disable the default fill-value-mode. */
  if ((retval =
       nc_set_fill (context->ncid, NC_NOFILL, &context->old_fill_mode))) {
    ERR (retval);
  }

  /* MPI-Barrier */
  retval = sc_MPI_Barrier (sc_MPI_COMM_WORLD);
  SC_CHECK_MPI (retval);

  /* End define-mode. NetCDF-file enters data-mode. */
  if ((retval = nc_enddef (context->ncid))) {
    ERR (retval);
  }

  /* Write the NetCDF-coordinate variable data */
  t8_forest_write_netcdf_coordinate_data (forest, context);

  /* Eventually write user-defined variable data */
  t8_forest_write_user_netcdf_data (forest, context, num_extern_netcdf_vars,
                                    ext_variables);

  /* All data has been written to the NetCDF-file, therefore, close the file. */
  if ((retval = nc_close (context->ncid))) {
    ERR (retval);
  }

  t8_global_productionf ("The NetCDF-File has been written and closed.\n");
#endif
}

/* Function that gets called if a cmesh schould be written in NetCDF-Format */
void
t8_forest_write_netcdf (t8_forest_t forest, const char *file_prefix,
                        const char *file_title, int dim,
                        int num_extern_netcdf_vars,
                        t8_netcdf_variable_t * ext_variables[])
{
#if T8_WITH_NETCDF
  t8_forest_netcdf_context_t context;
  /* Check whether pointers are not NULL */
  T8_ASSERT (file_title != NULL);
  T8_ASSERT (file_prefix != NULL);
  char                file_name[BUFSIZ];
  /* Create the NetCDF-Filname */
  snprintf (file_name, BUFSIZ, "%s.nc", file_prefix);
  /* Initialize first variables for NetCDF purposes. */
  context.filename = file_name;
  context.filetitle = file_title;
  context.dim = dim;
  context.nMaxMesh_elem_nodes = t8_element_shape_max_num_corner[dim];
  context.fillvalue = -1;
  context.start_index = 0;
  context.convention = "UGRID v1.0";
  t8_forest_netcdf_ugrid_namespace_t namespace_context;
  t8_forest_init_ugrid_namespace_context (&namespace_context, dim);
  /* Check which dimension of cmesh should be written. */
  switch (dim) {
  case 2:
    t8_debugf ("Writing 2D forest to NetCDF.\n");
    t8_forest_write_netcdf_file (forest, &context, &namespace_context,
                                 num_extern_netcdf_vars, ext_variables);
    break;
  case 3:
    t8_debugf ("Writing 3D forest to NetCDF.\n");
    t8_forest_write_netcdf_file (forest, &context, &namespace_context,
                                 num_extern_netcdf_vars, ext_variables);
    break;
  default:
    t8_global_errorf
      ("Only writing 2D and 3D NetCDF forest data is supported.\n");
    break;
  }
#else
  t8_global_errorf
    ("This version of t8code is not compiled with netcdf support.\n");
#endif
}

T8_EXTERN_C_END ();