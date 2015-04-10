/*
  This file is part of t8code.
  t8code is a C library to manage a collection (a forest) of multiple
  connected adaptive space-trees of general element classes in parallel.

  Copyright (C) 2010 The University of Texas System
  Written by Carsten Burstedde, Lucas C. Wilcox, and Tobin Isaac

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

/** \file t8_dtri_bits.h
 */

#ifndef T8_DTRI_BITS_H
#define T8_DTRI_BITS_H

#include <t8_element.h>
#include "t8_dtri.h"

T8_EXTERN_C_BEGIN ();

/** Test if two tris have the same coordinates, type and level.
 * \return true if \a t1 describes the same tri as \a t2.
 */
int                 t8_dtri_is_equal (const t8_dtri_t * t1,
                                      const t8_dtri_t * t2);

/** Compute the parent of a tri.
 * \param [in]  elem Input tri.
 * \param [in,out] parent Existing tri whose data will
 *                  be filled with the data of elem's parent.
 * \note \a elem may point to the same tri as \a parent.
 */
void                t8_dtri_parent (const t8_dtri_t * t, t8_dtri_t * parent);

/** Compute the coordinates of a vertex of a triangle.
 * \param [in] t    Input triangle.
 * \param [out] coordinates An array of 2 t8_dtri_coord_t that
 * 		     will be filled with the coordinates of the vertex.
 * \param [in] vertex The number of the vertex.
 */
void                t8_dtri_compute_coords (const t8_dtri_t * t,
                                            t8_dtri_coord_t coordinates[2],
                                            const int vertex);

/** Compute the coordinates of the four vertices of a tri.
 * \param [in] t    Input tri.
 * \param [out] coordinates An array of 4x3 t8_dtri_coord_t that
 * 		     will be filled with the coordinates of t's vertices.
 */
void                t8_dtri_compute_all_coords (const t8_dtri_t * t,
                                                t8_dtri_coord_t
                                                coordinates[3][2]);

/** Compute the childid-th child in Bey order of a tri t.
 * \param [in] t    Input tri.
 * \param [in,out] childid The id of the child, 0..7 in Bey order.
 * \param [out] child  Existing tri whose data will be filled
 * 		    with the date of t's childid-th child.
 */
void                t8_dtri_child (const t8_dtri_t * elem,
                                   int childid, t8_dtri_t * child);

/** Compute a specific sibling of a tri.
 * \param [in]     elem  Input tri.
 * \param [in,out] sibling  Existing tri whose data will be filled
 *                    with the data of sibling no. sibling_id of elem.
 * \param [in]     sibid The id of the sibling computed, 0..7 in Bey order.
 */
void                t8_dtri_sibling (const t8_dtri_t * elem,
                                     int sibid, t8_dtri_t * sibling);

/** Compute the face neighbor of a tri.
 * \param [in]     t      Input tri.
 * \param [in]     face   The face across which to generate the neighbor.
 * \param [in,out] n      Existing tri whose data will be filled.
 * \note \a t may point to the same tri as \a n.
 */
int                 t8_dtri_face_neighbour (const t8_dtri_t * t,
                                            t8_dtri_t * n, int face);

/** Test if two tris are siblings.
 * \param [in] t1 First tri to be tested.
 * \param [in] t2 Second tri to be tested.
 * \return true if \a t1 is unequal to and a sibling of \a t2.
 */
int                 t8_dtri_is_sibling (const t8_dtri_t * t1,
                                        const t8_dtri_t * t2);

/** Test if a tri is the parent of another tri.
 * \param [in] t tri to be tested.
 * \param [in] c Possible child tri.
 * \return true if \a t is the parent of \a c.
 */
int                 t8_dtri_is_parent (const t8_dtri_t * t,
                                       const t8_dtri_t * c);

/** Test if a tri is an ancestor of another tri.
 * \param [in] t tri to be tested.
 * \param [in] c Descendent tri.
 * \return true if \a t is unequal to and an ancestor of \a c.
 */
int                 t8_dtri_is_ancestor (const t8_dtri_t * t,
                                         const t8_dtri_t * c);

T8_EXTERN_C_END ();

#endif /* T8_DTRI_BITS_H */