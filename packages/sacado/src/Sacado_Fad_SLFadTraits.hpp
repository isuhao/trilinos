// $Id$ 
// $Source$ 
// @HEADER
// ***********************************************************************
// 
//                           Sacado Package
//                 Copyright (2006) Sandia Corporation
// 
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact David M. Gay (dmgay@sandia.gov) or Eric T. Phipps
// (etphipp@sandia.gov).
// 
// ***********************************************************************
// @HEADER

#ifndef SACADO_FAD_SLFADTRAITS_HPP
#define SACADO_FAD_SLFADTRAITS_HPP

#include "Sacado_Traits.hpp"
#include <sstream>

// Forward declarations
namespace Sacado {
  namespace Fad {
    template <typename T, int Num> class SLFad;
  }
}

namespace Sacado {

  //! Specialization of %Promote to SLFad types
  template <typename ValueT, int Num>
  struct Promote< Fad::SLFad<ValueT,Num>, 
		  Fad::SLFad<ValueT,Num> > {
    typedef Fad::SLFad<ValueT,Num> type;
  };

  //! Specialization of %Promote to SLFad types
  template <typename ValueT, int Num, typename R>
  struct Promote< Fad::SLFad<ValueT,Num>, R > {
    typedef typename ValueType< Fad::SLFad<ValueT,Num> >::type value_type_l;
    typedef typename ValueType<R>::type value_type_r;
    typedef typename Promote<value_type_l,value_type_r>::type value_type;

    typedef Fad::SLFad<value_type,Num> type;
  };

  //! Specialization of %Promote to SLFad types
  template <typename L, typename ValueT, int Num>
  struct Promote< L, Fad::SLFad<ValueT, Num> > {
  public:

    typedef typename ValueType<L>::type value_type_l;
    typedef typename ValueType< Fad::SLFad<ValueT,Num> >::type value_type_r;
    typedef typename Promote<value_type_l,value_type_r>::type value_type;

    typedef Fad::SLFad<value_type,Num> type;
  };

  //! Specialization of %ScalarType to SLFad types
  template <typename ValueT, int Num>
  struct ScalarType< Fad::SLFad<ValueT,Num> > {
    typedef typename Fad::SLFad<ValueT,Num>::ScalarT type;
  };

  //! Specialization of %ValueType to SLFad types
  template <typename ValueT, int Num>
  struct ValueType< Fad::SLFad<ValueT,Num> > {
    typedef ValueT type;
  };

  //! Specialization of %IsADType to SLFad types
  template <typename ValueT, int Num>
  struct IsADType< Fad::SLFad<ValueT,Num> > {
    static const bool value = true;
  };

  //! Specialization of %IsADType to SLFad types
  template <typename ValueT, int Num>
  struct IsScalarType< Fad::SLFad<ValueT,Num> > {
    static const bool value = false;
  };

  //! Specialization of %Value to SLFad types
  template <typename ValueT, int Num>
  struct Value< Fad::SLFad<ValueT,Num> > {
    typedef typename ValueType< Fad::SLFad<ValueT,Num> >::type value_type;
    static const value_type& eval(const Fad::SLFad<ValueT,Num>& x) { 
      return x.val(); }
  };

  //! Specialization of %ScalarValue to SLFad types
  template <typename ValueT, int Num>
  struct ScalarValue< Fad::SLFad<ValueT,Num> > {
    typedef typename ValueType< Fad::SLFad<ValueT,Num> >::type value_type;
    typedef typename ScalarType< Fad::SLFad<ValueT,Num> >::type scalar_type;
    static const scalar_type& eval(const Fad::SLFad<ValueT,Num>& x) { 
      return ScalarValue<value_type>::eval(x.val()); }
  };

  //! Specialization of %StringName to SLFad types
  template <typename ValueT, int Num>
  struct StringName< Fad::SLFad<ValueT,Num> > {
    static std::string eval() { 
      std::stringstream ss;
      ss << "Sacado::Fad::SLFad< " 
	 << StringName<ValueT>::eval() << ", " << Num << " >";
      return ss.str(); 
    }
  };

} // namespace Sacado

// Define Teuchos traits classes
#ifdef HAVE_SACADO_TEUCHOS
#include "Teuchos_PromotionTraits.hpp"
#include "Teuchos_ScalarTraits.hpp"
#include "Sacado_Fad_ScalarTraitsImp.hpp"

namespace Teuchos {

  //! Specialization of %Teuchos::PromotionTraits to SLFad types
  template <typename ValueT, int Num>
  struct PromotionTraits< Sacado::Fad::SLFad<ValueT,Num>, 
			  Sacado::Fad::SLFad<ValueT,Num> > {
    typedef typename Sacado::Promote< Sacado::Fad::SLFad<ValueT,Num>,
				      Sacado::Fad::SLFad<ValueT,Num> >::type
    promote;
  };

  //! Specialization of %Teuchos::PromotionTraits to SLFad types
  template <typename ValueT, int Num, typename R>
  struct PromotionTraits< Sacado::Fad::SLFad<ValueT,Num>, R > {
    typedef typename Sacado::Promote< Sacado::Fad::SLFad<ValueT,Num>, R >::type 
    promote;
  };

  //! Specialization of %Teuchos::PromotionTraits to SLFad types
  template <typename L, typename ValueT, int Num>
  struct PromotionTraits< L, Sacado::Fad::SLFad<ValueT,Num> > {
  public:
    typedef typename Sacado::Promote< L, Sacado::Fad::SLFad<ValueT,Num> >::type 
    promote;
  };

  //! Specializtion of Teuchos::ScalarTraits
  template <typename ValueT, int Num>
  struct ScalarTraits< Sacado::Fad::SLFad<ValueT,Num> > :
    public Sacado::Fad::ScalarTraitsImp< Sacado::Fad::SLFad<ValueT,Num> >
  {};
}
#endif // HAVE_SACADO_TEUCHOS

#endif // SACADO_FAD_SFADTRAITS_HPP
