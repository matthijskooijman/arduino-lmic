/*

Module:  lmic_env.h

Function:
	Sets up macros etc. to make things a little easier for portabilty

Copyright notice and license info:
	See LICENSE file accompanying this project.
 
Author:
	Terry Moore, MCCI Corporation	November 2018

Description:
	This file is an adaptation of MCCI's standard IOCTL framework.
	We duplicate a bit of functionality that we might get from other
	libraries, so that the LMIC library can continue to stand alone.

*/

#ifndef _lmic_env_h_	/* prevent multiple includes */
#define _lmic_env_h_

/*

Macro:	LMIC_C_ASSERT()

Function:
	Declaration-like macro that will cause a compile error if arg is FALSE.

Definition:
	LMIC_C_ASSERT(
		BOOL fErrorIfFalse
		);

Description:
	This macro, if used where an external reference declarataion is
	permitted, will either compile cleanly, or will cause a compilation
	error. The results of using this macro where a declaration is not
	permitted are unspecified.

	This is different from #if !(fErrorIfFalse) / #error in that the 
	expression is evaluated by the compiler rather than by the pre-
	processor. Therefore things like sizeof() can be used.

Returns:
	No explicit result -- either compiles cleanly or causes a compile
	error.

*/

#ifndef LMIC_C_ASSERT
# define LMIC_C_ASSERT(e)	\
 void  LMIC_C_ASSERT__(int LMIC_C_ASSERT_x[(e) ? 1: -1])
#endif

/****************************************************************************\
|
|	Define the begin/end declaration tags for C++ co-existance
|
\****************************************************************************/

#ifdef __cplusplus
# define LMIC_BEGIN_DECLS	extern "C" {
# define LMIC_END_DECLS	}
#else
# define LMIC_BEGIN_DECLS	/* nothing */
# define LMIC_END_DECLS	/* nothing */
#endif

//----------------------------------------------------------------------------
// Annotations to avoid various "unused" warnings. These must appear as a
// statement in the function body; the macro annotates the variable to quiet
// compiler warnings.  The way this is done is compiler-specific, and so these
// definitions are fall-backs, which might be overridden.
//
// Although these are all similar, we don't want extra macro expansions,
// so we define each one explicitly rather than relying on a common macro.
//----------------------------------------------------------------------------

// signal that a parameter is intentionally unused.
#ifndef LMIC_UNREFERENCED_PARAMETER
# define LMIC_UNREFERENCED_PARAMETER(v)      do { (void) (v); } while (0)
#endif

// an API parameter is a parameter that is required by an API definition, but
// happens to be unreferenced in this implementation. This is a stronger
// assertion than LMIC_UNREFERENCED_PARAMETER(): this parameter is here
// becuase of an API contract, but we have no use for it in this function.
#ifndef LMIC_API_PARAMETER
# define LMIC_API_PARAMETER(v)               do { (void) (v); } while (0)
#endif

// an intentionally-unreferenced variable.
#ifndef LMIC_UNREFERENCED_VARIABLE
# define LMIC_UNREFERENCED_VARIABLE(v)       do { (void) (v); } while (0)
#endif

// we have three (!) debug levels (LMIC_DEBUG_LEVEL > 0, LMIC_DEBUG_LEVEL > 1,
// and LMIC_X_DEBUG_LEVEL > 0. In each case we might have parameters or
// or varables that are only refereneced at the target debug level.

// Parameter referenced only if debugging at level > 0.
#ifndef LMIC_DEBUG1_PARAMETER
# if LMIC_DEBUG_LEVEL > 0
#  define LMIC_DEBUG1_PARAMETER(v)           do { ; } while (0)
# else
#  define LMIC_DEBUG1_PARAMETER(v)           do { (void) (v); } while (0)
# endif
#endif

// variable referenced only if debugging at level > 0
#ifndef LMIC_DEBUG1_VARIABLE
# if LMIC_DEBUG_LEVEL > 0
#  define LMIC_DEBUG1_VARIABLE(v)            do { ; } while (0)
# else
#  define LMIC_DEBUG1_VARIABLE(v)            do { (void) (v); } while (0)
# endif
#endif

// parameter referenced only if debugging at level > 1
#ifndef LMIC_DEBUG2_PARAMETER
# if LMIC_DEBUG_LEVEL > 1
#  define LMIC_DEBUG2_PARAMETER(v)           do { ; } while (0)
# else
#  define LMIC_DEBUG2_PARAMETER(v)           do { (void) (v); } while (0)
# endif
#endif

// variable referenced only if debugging at level > 1
#ifndef LMIC_DEBUG2_VARIABLE
# if LMIC_DEBUG_LEVEL > 1
#  define LMIC_DEBUG2_VARIABLE(v)            do { ; } while (0)
# else
#  define LMIC_DEBUG2_VARIABLE(v)            do { (void) (v); } while (0)
# endif
#endif

// parameter referenced only if LMIC_X_DEBUG_LEVEL > 0
#ifndef LMIC_X_DEBUG_PARAMETER
# if LMIC_X_DEBUG_LEVEL > 0
#  define LMIC_X_DEBUG_PARAMETER(v)           do { ; } while (0)
# else
#  define LMIC_X_DEBUG_PARAMETER(v)           do { (void) (v); } while (0)
# endif
#endif

// variable referenced only if LMIC_X_DEBUG_LEVEL > 0
#ifndef LMIC_X_DEBUG_VARIABLE
# if LMIC_X_DEBUG_LEVEL > 0
#  define LMIC_X_DEBUG_VARIABLE(v)            do { ; } while (0)
# else
#  define LMIC_X_DEBUG_VARIABLE(v)            do { (void) (v); } while (0)
# endif
#endif

// parameter referenced only if EV() macro is enabled (which it never is)
// TODO(tmm@mcci.com) take out the EV() framework as it reuqires C++, and
// this code is really C-99 to its bones.
#ifndef LMIC_EV_PARAMETER
# define LMIC_EV_PARAMETER(v)                 do { (void) (v); } while (0)
#endif

// variable referenced only if EV() macro is defined.
#ifndef LMIC_EV_VARIABLE
# define LMIC_EV_VARIABLE(v)                  do { (void) (v); } while (0)
#endif

/*

Macro:	LMIC_ABI_STD

Index:	Macro:	LMIC_ABI_VARARGS

Function:
	Annotation macros to force a particular binary calling sequence.

Definition:
	#define LMIC_ABI_STD		compiler-specific
	#define	LMIC_ABI_VARARGS 	compiler-specific

Description:
	These macros are used when declaring a function type, and indicate
	that a particular calling sequence is to be used. They are normally
	used between the type portion of the function declaration and the
	name of the function.  For example:

	typedef void LMIC_ABI_STD myCallBack_t(void);

	It's important to use this in libraries on platforms with multiple
	calling sequences, because different components can be compiled with
	different defaults.

Returns:
	Not applicable.

*/

/* ABI marker for normal (fixed parameter count) functions -- used for function types */
#ifndef LMIC_ABI_STD
# ifdef _MSC_VER
#  define LMIC_ABI_STD	__stdcall
# else
#  define LMIC_ABI_STD	/* nothing */
# endif
#endif

/* ABI marker for VARARG functions -- used for function types */
#ifndef LMIC_ABI_VARARGS
# ifdef _MSC_VER
#  define LMIC_ABI_VARARGS	__cdecl
# else
#  define LMIC_ABI_VARARGS	/* nothing */
# endif
#endif

#endif /* _lmic_env_h_ */