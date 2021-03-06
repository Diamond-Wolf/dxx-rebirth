/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <functional>
#include <cstddef>
#include "dxxsconf.h"
#include "cpp-valptridx.h"

/* Unexpected invalid data can be handled in one of three ways.
 *
 * DXX_VALPTRIDX_ERROR_STYLE_TREAT_AS_UB: if invalid input is found, ignore
 * it and continue on; the program will have Undefined Behavior if an
 * invalid input ever occurs.
 *
 * DXX_VALPTRIDX_ERROR_STYLE_TREAT_AS_TRAP: if invalid input is found,
 * execute __builtin_trap(); a compiler-implementation dependent fatal
 * exit occurs.
 *
 * DXX_VALPTRIDX_ERROR_STYLE_TREAT_AS_EXCEPTION: if invalid input is found,
 * throw an exception.
 *
 * UB produces the smallest program, but has Undefined Behavior in
 * the case of errors.  The program may crash immediately, crash at a
 * later stage when some other function becomes confused by the invalid
 * data propagated by the undefined behavior, continue to run but behave
 * incorrectly, or seem to work normally.
 *
 * TRAP produces the next smallest program.  The program will crash on
 * fatal errors, but error reporting is minimal.  Inspection of the core
 * file will be required to identify the cause of the crash, and may be
 * difficult, depending on how the optimizer laid out the program.
 *
 * EXCEPTION produces the largest program, but provides the best error
 * reporting.
 *
 * Choose UB to profile for how much space is consumed by the validation
 * and reporting code.
 * Choose TRAP to profile for how much space is consumed by the
 * reporting code or to reduce size when debugging is not a concern.
 * Otherwise, choose EXCEPTION.
 *
 * This choice intentionally lacks a named configure knob.
 */
#define DXX_VALPTRIDX_ERROR_STYLE_TREAT_AS_UB	0
#define DXX_VALPTRIDX_ERROR_STYLE_TREAT_AS_TRAP	1
#define DXX_VALPTRIDX_ERROR_STYLE_TREAT_AS_EXCEPTION	2

#ifndef DXX_VALPTRIDX_REPORT_ERROR_STYLE
//#define DXX_VALPTRIDX_REPORT_ERROR_STYLE	DXX_VALPTRIDX_ERROR_STYLE_TREAT_AS_UB
//#define DXX_VALPTRIDX_REPORT_ERROR_STYLE	DXX_VALPTRIDX_ERROR_STYLE_TREAT_AS_TRAP
#define DXX_VALPTRIDX_REPORT_ERROR_STYLE	DXX_VALPTRIDX_ERROR_STYLE_TREAT_AS_EXCEPTION
#endif

/* Only EXCEPTION uses the filename/lineno information.  Omit it from
 * other configurations, even when the compiler supports
 * __builtin_FILE().
 */
#if defined(DXX_HAVE_CXX_BUILTIN_FILE_LINE) && DXX_VALPTRIDX_REPORT_ERROR_STYLE == DXX_VALPTRIDX_ERROR_STYLE_TREAT_AS_EXCEPTION
#define DXX_VALPTRIDX_ENABLE_REPORT_FILENAME
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DECL_VARS	const char *filename = __builtin_FILE(), const unsigned lineno = __builtin_LINE()
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS	, DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DECL_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DEFN_VARS	const char *const filename, const unsigned lineno
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS	DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DEFN_VARS,
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_PASS_VARS_	filename, lineno
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_VOID_VARS()	static_cast<void>(filename), static_cast<void>(lineno)
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS	, DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_PASS_VARS_
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS	DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_PASS_VARS_,
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VA(...)	DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_PASS_VARS_, ## __VA_ARGS__
#else
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DECL_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_DECL_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_DEFN_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_N_VOID_VARS()	static_cast<void>(0)
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_L_PASS_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VARS
#define DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_PASS_VA(...)	__VA_ARGS__
#endif

template <typename managed_type>
class valptridx :
	protected valptridx_specialized_types<managed_type>::type
{
	using specialized_types = typename valptridx_specialized_types<managed_type>::type;
	using specialized_types::array_size;
	class partial_policy
	{
	public:
		class require_valid;
		class allow_invalid;
		template <template <typename> class policy>
			class apply_cv_policy;
	};
	class vc;	/* require_valid + const_policy */
	class ic;	/* allow_invalid + const_policy */
	class vm;	/* require_valid + mutable_policy */
	class im;	/* allow_invalid + mutable_policy */
	template <typename>
		class guarded;
	class array_base_count_type;
	using array_base_storage_type = std::array<managed_type, array_size>;
public:
	class array_managed_type;

protected:
	using const_pointer_type = const managed_type *;
	using const_reference_type = const managed_type &;
	using mutable_pointer_type = managed_type *;
	/* integral_type must be a primitive integer type capable of holding
	 * all legal values used with managed_type.  Legal values are valid
	 * indexes in array_managed_type and any magic out-of-range values.
	 */
	using typename specialized_types::integral_type;
	using index_type = integral_type;	// deprecated; should be dedicated UDT

	/* basic_ptridx<policy> publicly inherits from basic_idx<policy> and
	 * basic_ptr<policy>, but should not be implicitly sliced to one of
	 * the base types.  To prevent slicing, basic_idx and basic_ptr take
	 * a dummy parameter that is set to 0 for freestanding use and 1
	 * when used as a base class.
	 */
	template <typename policy, unsigned>
		class basic_idx;
	template <typename policy, unsigned>
		class basic_ptr;
	template <typename policy>
		class basic_ptridx;
	template <typename Pc, typename Pm>
		class basic_ival_member_factory;
	template <typename Pc, typename Pm>
		class basic_vval_member_factory;
	class allow_end_construction;
	class assume_nothrow_index;

	static inline void check_index_match(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_reference_type, index_type, const array_managed_type &);
	template <template <typename> class Compare = std::less>
	static inline index_type check_index_range(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS index_type, const array_managed_type *);
	static inline void check_explicit_index_range_ref(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_reference_type, std::size_t, const array_managed_type &);
	static inline void check_implicit_index_range_ref(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_reference_type, const array_managed_type &);
	static inline void check_null_pointer_conversion(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_pointer_type);
	static inline void check_null_pointer(DXX_VALPTRIDX_REPORT_STANDARD_LEADER_COMMA_R_DEFN_VARS const_pointer_type, const array_managed_type &);

#define DXX_VALPTRIDX_FOR_EACH_VC_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, IVPREFIX)	\
	VERB(MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, IVPREFIX, m);	\
	VERB(MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, IVPREFIX, c)

#define DXX_VALPTRIDX_FOR_EACH_IV_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX)	\
	DXX_VALPTRIDX_FOR_EACH_VC_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, i);	\
	DXX_VALPTRIDX_FOR_EACH_VC_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, v)

#define DXX_VALPTRIDX_FOR_EACH_IDX_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)	\
	DXX_VALPTRIDX_FOR_EACH_IV_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, idx)

#define DXX_VALPTRIDX_FOR_EACH_PTR_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)	\
	DXX_VALPTRIDX_FOR_EACH_IV_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, ptr)

#define DXX_VALPTRIDX_FOR_EACH_PTRIDX_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)	\
	DXX_VALPTRIDX_FOR_EACH_IV_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, ptridx)

#define DXX_VALPTRIDX_FOR_EACH_PPI_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)	\
	DXX_VALPTRIDX_FOR_EACH_PTR_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT);	\
	DXX_VALPTRIDX_FOR_EACH_PTRIDX_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)

#define DXX_VALPTRIDX_FOR_EACH_IPPI_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)	\
	DXX_VALPTRIDX_FOR_EACH_IDX_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT);	\
	DXX_VALPTRIDX_FOR_EACH_PPI_TYPE(VERB, MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT)

public:
	/* This is a special placeholder that allows segiter to bypass the
	 * normal rules.  The calling code is responsible for providing the
	 * safety that is bypassed.  Use this bypass only if you understand
	 * exactly what you are skipping and why your use is safe despite
	 * the lack of checking.
	 */
	class allow_none_construction;
	typedef basic_ptridx<ic>	icptridx;
	typedef basic_ptridx<im>	imptridx;
	typedef basic_ptridx<vc>	vcptridx;
	typedef basic_ptridx<vm>	vmptridx;
	typedef basic_idx<ic, 0>	icidx;
	typedef basic_idx<im, 0>	imidx;
	typedef basic_idx<vc, 0>	vcidx;
	typedef basic_idx<vm, 0>	vmidx;
	typedef basic_ptr<ic, 0>	icptr;
	typedef basic_ptr<im, 0>	imptr;
	typedef basic_ptr<vc, 0>	vcptr;
	typedef basic_ptr<vm, 0>	vmptr;
#if DXX_VALPTRIDX_REPORT_ERROR_STYLE == DXX_VALPTRIDX_ERROR_STYLE_TREAT_AS_EXCEPTION
	/* These exceptions can only be thrown when style is EXCEPTION.
	 * Other reporting styles never generate them, so they are left
	 * undeclared to trap any code which attempts to catch an exception
	 * that is never thrown.
	 */
	class index_mismatch_exception;
	class index_range_exception;
	class null_pointer_exception;
#endif

	template <integral_type constant>
		class magic_constant
		{
		public:
			constexpr operator integral_type() const { return constant; }	// integral_type conversion deprecated
		};
};

#define DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEF(MANAGED_TYPE, DERIVED_TYPE_PREFIX, CONTEXT, PISUFFIX, IVPREFIX, MCPREFIX)	\
	using IVPREFIX ## MCPREFIX ## DERIVED_TYPE_PREFIX ## PISUFFIX ## _t = valptridx<MANAGED_TYPE>::IVPREFIX ## MCPREFIX ## PISUFFIX

#define DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEFS(MANAGED_TYPE, DERIVED_TYPE_PREFIX)	\
	DXX_VALPTRIDX_FOR_EACH_IPPI_TYPE(DXX_VALPTRIDX_DEFINE_SUBTYPE_TYPEDEF, MANAGED_TYPE, DERIVED_TYPE_PREFIX,)
