
/*! @HEADER */
/*
************************************************************************

                CTrilinos:  C interface to Trilinos
                Copyright (2009) Sandia Corporation

Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
license for use of this work by or on behalf of the U.S. Government.

This library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA
Questions? Contact M. Nicole Lemaster (mnlemas@sandia.gov)

************************************************************************
*/
/*! @HEADER */


/*! @file CTrilinos_utils_templ.hpp
 * @brief Templated utility functions for CTrilinos. */


#ifndef CTRILINOS_UTILS_TEMPL_HPP
#define CTRILINOS_UTILS_TEMPL_HPP


#include "CTrilinos_config.h"


#include <string>
#include "CTrilinos_enums.h"


namespace CTrilinos {


/* convert struct from specific type to generic CTrilinos_Universal_ID_t
 * but keep the content in tact */
template <typename T>
CTrilinos_Universal_ID_t
abstractType( T id )
{
    CTrilinos_Universal_ID_t newid;

    newid.table = id.table;
    newid.index = id.index;
    newid.is_const = id.is_const;

    return newid;
}

/* convert struct from generic CTrilinos_Universal_ID_t to specific type
 * but keep the content in tact */
template <typename T>
T
concreteType( CTrilinos_Universal_ID_t id )
{
    T newid;

    newid.table = id.table;
    newid.index = id.index;
    newid.is_const = id.is_const;

    return newid;
}


} // namespace CTrilinos


#endif
