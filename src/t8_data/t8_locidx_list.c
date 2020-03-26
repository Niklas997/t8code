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

/** \file t8_locidx_list.c
 *
 * In this file we implement the routines to handle
 * t8_locidx_list_t access.
 */

#include <t8_data/t8_locidx_list.h>

/* Allocate a new t8_locidx_list object and return a poitner to it.
 * \return A pointer to a newly allocated t8_locidx_list_t
 */
t8_locidx_list_t   *
t8_locidx_list_new ()
{
  t8_locidx_list_t   *list;

  /* Allocate memory for our list */
  list = T8_ALLOC (t8_locidx_list_t, list);

  /* initialize the new list */
  t8_locidx_list_init (list);

  /* Assert that this list is initialized */
  T8_ASSERT (t8_locidx_list_is_initialized (list));
  /* return the initialized list */
  return list;
}

/* Initialize an allocated t8_locidx_list
 * \param [in, out] Pointer to an allocated t8_locidx_list_t
 * \note If this list is already filled the all its elements are free'd
 */
void
t8_locidx_list_init (t8_locidx_list_t * list)
{
  T8_ASSERT (list != NULL);

  /* Initialize the data list */
  sc_list_init (&list->list, NULL);

  /* Initialize the mempool to store the t8_locidx_t */
  sc_mempool_init (&list->indices, sizeof (t8_locidx_t));

  /* Assert that this list is initialized */
  T8_ASSERT (t8_locidx_list_is_initialized (list));
}

/* Query whether a t8_locidx_list_t is initialized.
 * \param [in] list A pointer to a list.
 * \return True (non-zero) if and only if the list is properly initialized.
 * \note Calling this with \a list = NULL is valid and will return 0.
 */
int
t8_locidx_list_is_initialized (const t8_locidx_list_t * list)
{
  if (list == NULL) {
    return 0;
  }

  /* A list is initialized if 
   * - its mempool has the proper size associated to it. 
   * - its list stores sc_lint_t objects
   */
  if (list->indices.elem_size != sizeof (t8_locidx_t)
      || list->list.allocator == NULL
      || list->list.allocator->elem_size != sizeof (sc_link_t)) {
    return 0;
  }

  /* Safety check that the number of elements in the list matches
   * the number of elements in the mempool. */
  T8_ASSERT (list->list.elem_count == list->indices.elem_count);
  return 1;
}

/* Remove the last element in a list and return its entry.
 * \param [in,out] list An initliazed list with at least one entry.
 *                      On return its last entry will have been removed.
 * \return  The last entry of \a list.
 * \note It is illegal to call this function if \a list does not have any elements.
 */
t8_locidx_t
t8_locidx_list_pop (t8_locidx_list_t * list)
{
  /* List must be initialized and non-empty */
  T8_ASSERT (t8_locidx_list_is_initialized (list));
  T8_ASSERT (t8_locidx_list_count (list) > 0);
  /* Pop the last item from the internal list */
  t8_locidx_t        *ppopped_index = sc_list_pop (list->list);
  t8_locidx_t         popped_index = *ppopped_index;

  /* Free the memory associated to the popped index */
  sc_mempool_free (list->indices, ppopped_index);

  /* return the index */
  return popped_index;
}

/* Returns the number of entries in a list.
 * \param [in] list An initialized list.
 * \return          The number of entries in \a list.
 */
size_t
t8_locidx_list_count (const t8_locidx_list_t * list)
{
  /* Assert that this list is initialized */
  T8_ASSERT (t8_locidx_list_is_initialized (list));

  return list->list.elem_count;
}

/* Append a new entry to the end of a list.
 * \param [in,out] list An initialized list.
 * \param [in]     entry The new entry.
 */
void
t8_locidx_list_append (t8_locidx_list_t * list, const t8_locidx_t entry)
{
  /* Assert that this list is initialized */
  T8_ASSERT (t8_locidx_list_is_initialized (list));

  /* Allocated an element in the mempool. */
  void               *allocated_entry = sc_mempool_alloc (list->indices);
  /* Append this element to the list */
  sc_list_append (list->list, (void *) allocated_entry);
}

/* Free all elements of a list.
 * \param [in, out] list An initialized list.
 * After calling this function \a list will still be initialized but
 * all its elements will have been free'd and its count will be zero.
 */
void
t8_locidx_list_reset (t8_locidx_list_t * list)
{
  /* Assert that this list is initialized */
  T8_ASSERT (t8_locidx_list_is_initialized (list));

  sc_list_reset (list->list);
  sc_mempool_reset (list->indices);
}