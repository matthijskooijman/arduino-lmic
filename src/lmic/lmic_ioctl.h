/*

Module:  lmic_ioctl.h

Function:
	Definition of LMIC IOCTL facility

Copyright notice and license info:
	See LICENSE file accompanying this project.
 
Author:
	Terry Moore, MCCI Corporation	November 2018

Description:
	This file is an adaptation of MCCI's standard IOCTL framework.
	We duplicate a bit of functionality that we might get from other
	libraries, so that the LMIC library can continue to stand alone.

*/

#ifndef _LMIC_IOCTL_H_	/* prevent multiple includes */
#define _LMIC_IOCTL_H_

#ifndef _LMIC_ENV_H_
# include "lmic_env.h"
#endif

#ifndef _LMIC_TYPES_H_
# include "lmic_types.h"
#endif

/*

Type:  LMIC_IOCTL_CODE

Function:
	Abstract C scalar type, carries an IOCTL code.

Description:
	IOCTL codes are defined similarly to BSD IOCTLs.

	Our IOCTL codes (unique to the LMIC) all have the 
	following layout:

	 3 3 2 2          1 1 1
	 1 0 9 8          6 5 4         0
	+-+-+-+------------+-+-----------+
	| | | |            | |           |
	+-+-+-+------------+-+-----------+
	 | | |       |      |      |
	 | | |       |      |      |
	 | | |       |      |      +-->	Function code. Conventionally
	 | | |       |      |		the upper 7 bits are a group
	 | | |       |      |		code, and the lower 8 bits are
	 | | |       |      |		a function code within the group.
	 | | |       |      |
	 | | |       |      +---------> Set to 1 
	 | | |       |
	 | | |       +---------------->	Length of parameters (in bytes).
	 | | |  
	 +-+-+------------------------> Parameter type:
					001 ->  no parameters (IN and
						OUT parameters are not
						pointers, and should be
						coded as NULL).
					010 ->	OUT parameter points to a
						buffer of given size for
						results from this function (IN
						paramter is not a pointer, 
						and should be coded as NULL).
					100 ->	IN parameter points to a
						buffer of given size, 
						containing parameters for 
						this IOCTL operation.
						(OUT parameter is not a
						pointer, and should be coded
						as NULL)
					110 ->	IN paramter points to input
						paramters, OUT pointer 
						points to a buffer that gets
						the results.  Both buffers
						must be of size indicated
						by the 'length' field.

	Frequently (architecturally) the codes are unexamined by the IOCTL
	dispatch functions; parameter blocks are fixed size, etc.  However,
	this design allows for the usual transparent filtering, and also
	allows for the use of variable length parameter blocks where needed.

See also:
	LMIC_IOCTL_RESULT
	LMIC_IOC(), LMIC_IOCTL_R(), LMIC_IOCTL_W(), 
	LMIC_IOCTL_RW()
	LMIC_IOC_ASYNC(), LMIC_IOCTL_R_ASYNC(), LMIC_IOCTL_W_ASYNC(), 
	LMIC_IOCTL_RW_ASYNC()

*/

typedef uint32_t	LMIC_IOCTL_CODE;

/*

Type:	LMIC_IOCTL_RESULT

Function:
	Abstract type for the functional result of any IOCTL operation.

Description:
	All IOCTL operations return a result of this kind.  There are 
	three categories of results:

	1) |LMIC_IOCTL_RESULT_NOT_CLAIMED| -- this means that the IOCTL
	   was not recognized by this IOCTL handler.  A single numeric code
	   is defined for this case.  This can be recognized by the macro
	   |LMIC_IOCTL_NOT_CLAIMED(code)|.

	2) |LMIC_IOCTL_RESULT_SUCCESS| -- this means that the IOCTL was
	   recognized, and it completed successfully.  A single numeric code
	   is defined for this case.  This can be recognized by the macro
	   |LMIC_IOCTL_SUCCESS(code)|.

	3) |LMIC_IOCTL_RESULT_FAILED| -- this means that the IOCTL was
	   recognized, but that an error occurred in processing.  A whole
	   family of error codes are defined to allow the type of failure
	   to be communicated to the caller.  These can be identified by the
	   boolean macro:  |LMIC_IOCTL_FAILED(code)|, which returns
	   TRUE for a failure, FALSE for either success or not-claimed.

	   Specific failure codes are constructed using the macro
	   |USBUPUMP_IOCTL_RESULT_FAILURE(int)|, which takes a positive 
	   integer, the index code of the failure.

*/

typedef int32_t	LMIC_IOCTL_RESULT;

#define	LMIC_IOCTL_RESULT_NOT_CLAIMED		((LMIC_IOCTL_RESULT) -1)
#define	LMIC_IOCTL_RESULT_SUCCESS		((LMIC_IOCTL_RESULT) 0)
#define	LMIC_IOCTL_RESULT_FAILED		((LMIC_IOCTL_RESULT) 1)

/* define a failure code */
#define	LMIC_IOCTL_RESULT_FAILURE(x)		((LMIC_IOCTL_RESULT) (x))

/*
|| Macros for checking status
*/
#define LMIC_IOCTL_SUCCESS(result)		((result) == 0)

#define	LMIC_IOCTL_NOT_CLAIMED(result)		((result) < 0)

#define	LMIC_IOCTL_FAILED(result)		((result) > 0)

/*

Name:	LMIC_IOCTL_DECLARE_IOCTL_FNTYPE()

Function:
	C preprocessor macro, defines a specific function type for IOCTL 
	dispatching.

Definition:
	LMIC_IOCTL_DECLARE_IOCTL_FNTYPE(
		FunctionTypeName,
		ContextTypeExpression
		);

Description:
	This macro declares |FunctionTypeName| as a standard LMIC IOCTL
	dispatch function, with a context parameter of type 
	|ContextTypeExpression|.

Returns:
	Not applicable -- is a type-definition macro.

Notes:
	Any use of this macro should normally be followed by a ';'.

	We use this routine because frequently the IOCTL dispatch helper
	functions actually have context pointer types that are known at
	compile time (despite the formidable abstractness of this API).
	Using this mechanism allows for type-safey in those situations.

*/

#define LMIC_DECLARE_IOCTL_FNTYPE(					\
	FunctionTypeName,						\
	ContextTypeExpression						\
	)								\
typedef									\
	LMIC_SAL_Function_class(FunctionTypeName)			\
	LMIC_SAL_Check_return						\
	LMIC_SAL_Success(return == 0)					\
LMIC_IOCTL_RESULT							\
(LMIC_ABI_STD FunctionTypeName)(					\
	LMIC_SAL_In 		ContextTypeExpression	pObject,	\
	LMIC_SAL_In L		MIC_IOCTL_CODE		IoctlCode,	\
	LMIC_SAL_In_opt 	const void *		pInParam,	\
	LMIC_SAL_Out_opt 	void *			pOutParam	\
	)

/*

Name:	LMIC_IOC()

Index:	Name:	LMIC_IOCTL_VOID()
	Name:	LMIC_IOCTL_R()
	Name:	LMIC_IOCTL_W()
	Name:	LMIC_IOCTL_RW()
	Name:	LMIC_IOCTL_IOC_EX()
	Name:	LMIC_IOCPARM_LEN()

Function:
	Compute IOCTL codes in a standard way.

Definition:
	LMIC_IOCTL_CODE LMIC_IOC(
				uint32_t DirMask,
				uint8_t  RawGroup,
				uint8_t  Num,
				LMIC_UINT   ArgSize
				);

	LMIC_IOCTL_CODE LMIC_IOCTL_VOID(
				uint8_t Group,
				uint8_t Num
				);
	LMIC_IOCTL_CODE LMIC_IOCTL_R(
				uint8_t Group,
				uint8_t Num,
				ArgumentTypeExpression
				);
	LMIC_IOCTL_CODE LMIC_IOCTL_W(
				uint8_t Group,
				uint8_t Num,
				ArgumentTypeExpression
				);
	LMIC_IOCTL_CODE LMIC_IOCTL_RW(
				uint8_t Group,
				uint8_t Num,
				ArgumentTypeExpression
				);

	size_t LMIC_IOCPARM_LEN(
			LMIC_IOCTL_CODE IoctlCode
			);

Description:
	|LMIC_IOCTL_*| macros compute the appropriate IOCTL code based
	on |Group|, |Num| and |ArgumentTypeExpression|, setting the
	appropriate direction code.  |Group| is always or-ed with 0x80,
	as group codes with bit 0x80 clear are reserved for future use.

	|LMIC_IOC()| computes a fully general IOCTL code, based on
	|DirMask|, |RawGroup|, |RawNum|, and |ArgumentSize|.  Note that
	the |RawGroup| is completely unmodified, and that a numeric
	size is used rather than a type.

	|LMIC_IOCPARM_LEN()| extracts the parameter buffer length from
	an IoctlCode.  This macro is used when processing IOCTLs that
	pass variable-length buffers.

*/

#define	LMIC_IOCPARM_MASK	0x1fffu	/* parameter length, at most 13 bits */
#define	LMIC_IOCPARM_SHIFT	16u	/* how far to shift -- defined for
					|| convenience of external users.
					|| If you change this, nothing good
					|| will result....
					*/
#define	LMIC_IOCPARM_LEN(x)	(((x) >> 16) & LMIC_IOCPARM_MASK)
#define	LMIC_IOCBASECMD(x)	((x) & ~(LMIC_IOCPARM_MASK << 16u))
#define	LMIC_IOCGROUP(x)	(((x) >> 8u) & 0xffu)

/* max size of ioctl args */
#define	LMIC_IOCPARM_MAX	((LMIC_IOCPARM_MASK + 1u) >> 1u)

/* no parameters */
#define	LMIC_IOC_VOID		((LMIC_IOCTL_CODE)0x20000000)

/* copy parameters out */
#define	LMIC_IOC_OUT		((LMIC_IOCTL_CODE)0x40000000)

/* copy parameters in */
#define	LMIC_IOC_IN		((LMIC_IOCTL_CODE)0x80000000)

/* copy paramters in and out */
#define	LMIC_IOC_INOUT		(LMIC_IOC_IN|LMIC_IOC_OUT)

/* mask for IN/OUT/VOID */
#define	LMIC_IOC_DIRMASK	((LMIC_IOCTL_CODE)0xe0000000)

/* the macro to greate a general code */
#define	LMIC_IOC(inout, group, num, len)				\
	((LMIC_IOCTL_CODE)(((inout) & LMIC_IOC_DIRMASK) |		\
			      (((len) & LMIC_IOCPARM_MASK) << 16u) |	\
			      ((group) << 8u) | (num)))

#define	LMIC_IOCTL_VOID(g, n)	\
	LMIC_IOC(LMIC_IOC_VOID,  (g) | 0x80u, (n), 0)
#define	LMIC_IOCTL_R(g, n, t)	\
	LMIC_IOC(LMIC_IOC_OUT,   (g) | 0x80u, (n), sizeof(t))
#define	LMIC_IOCTL_W(g, n, t)	\
	LMIC_IOC(LMIC_IOC_IN,    (g) | 0x80u, (n), sizeof(t))
#define	LMIC_IOCTL_RW(g, n, t)	\
	LMIC_IOC(LMIC_IOC_INOUT, (g) | 0x80u, (n), sizeof(t))

/*

Name:	LMIC_IOCMATCH()

Function:
	Compare IOCTL codes for IOCTLs with variable-size parameters.

Definition:
	bool LMIC_IOCMATCH(
		LMIC_IOCTL_CODE InputCode,
		LMIC_IOCTL_CODE ReferenceCode
		);

Description:
	Compare an input IOCTL code against a reference code, and 
	return TRUE if |InputCode| is derived from |ReferenceCode|,
	and if the parameter length field in |InputCode| is at least
	as large as the minimum value specified in |ReferenceCode|.

Returns:
	|TRUE| if |InputCode| code matches, |FALSE| otherwise.

*/


#define	LMIC_IOCMATCH(in, ref)					\
	(((((LMIC_IOCTL_CODE)(in) ^ (LMIC_IOCTL_CODE)(ref)) &	\
	   ~(LMIC_IOCPARM_MASK << LMIC_IOCPARM_SHIFT)) == 0) &&	\
	 (LMIC_IOCPARM_LEN(in) >= LMIC_IOCPARM_LEN(ref)))

/*

Name:	LMIC_IOCADJUST()

Function:
	Adjust an IOCTL code to reflect actual number of bytes passed.

Definition:
	LMIC_IOCTL_CODE LMIC_IOCADJUST(
		LMIC_IOCTL_CODE Code,
		unsigned	ActualSize
		);

Description:
	The Length field of the IOCTL code |Code| is replace by
	|ActualSize|.

	For most IOCTLs, this macro is not used, because most IOCTLs
	have fixed-size parameter structures.  We provide this
	for completeness.

Returns:
	The adjusted IOCTL code.

Notes:
	This is a macro, and parameters may be evaluated multiple times.
	If ActualSize > LMIC_IOCPARM_MASK, it will be reduced modulo
	(LMIC_IOCPARM_MASK+1).

*/

#define LMIC_IOCADJUST(Code, ActualSize)				\
	((LMIC_IOCTL_CODE)						\
	 ((LMIC_IOCTL_CODE)(Code) & ~(LMIC_IOCPARM_MASK << 16u) |	\
	  (((LMIC_IOCTL_CODE)(ActualSize) & LMIC_IOCPARM_MASK) << 16u)))

/****************************************************************************\
|
|	Some common failure codes.
|
\****************************************************************************/

/* 1 is used for LMIC_IOCTL_RESULT_FAILED */

/* 2:  output buffer too small */
#define	LMIC_IOCTL_RESULT_BUFFER_TOO_SMALL		\
	LMIC_IOCTL_RESULT_FAILURE(2)
/* 3:  some implementation limit was exceeded */
#define LMIC_IOCTL_RESULT_IMPLEMENTATION_LIMITATION	\
	LMIC_IOCTL_RESULT_FAILURE(3)
/* 4:  IN ptr omitted */
#define LMIC_IOCTL_RESULT_IN_PARAM_NULL			\
	LMIC_IOCTL_RESULT_FAILURE(4)
/* 5:  OUT ptr omitted */
#define LMIC_IOCTL_RESULT_OUT_PARAM_NULL		\
	LMIC_IOCTL_RESULT_FAILURE(5)
/* 6:  IOCTL code direction is not valid */
#define LMIC_IOCTL_RESULT_DIRECTION_BAD			\
	LMIC_IOCTL_RESULT_FAILURE(6)
/* 7:  last result in a sequence has been returned */
#define LMIC_IOCTL_RESULT_NO_MORE			\
	LMIC_IOCTL_RESULT_FAILURE(7)
/* 8:  an IN parameter value was invalid */
#define LMIC_IOCTL_RESULT_INVALID_PARAMETER		\
	LMIC_IOCTL_RESULT_FAILURE(8)
/* 9:  function was already open */
#define	LMIC_IOCTL_RESULT_ALREADY_OPEN			\
	LMIC_IOCTL_RESULT_FAILURE(9)
/* 10: function was not opened */
#define	LMIC_IOCTL_RESULT_NOT_OPEN			\
	LMIC_IOCTL_RESULT_FAILURE(10)
/* 11: RFU */
/* 12: RFU */
/* 14: request not implemented */
#define	LMIC_IOCTL_RESULT_NOT_IMPLEMENTED		\
	LMIC_IOCTL_RESULT_FAILURE(14)
/* 15: hardware in use */
#define	LMIC_IOCTL_RESULT_HARDWARE_INUSE		\
	LMIC_IOCTL_RESULT_FAILURE(15)
/* 16: hardware not in use */
#define	LMIC_IOCTL_RESULT_HARDWARE_NOT_INUSE		\
	LMIC_IOCTL_RESULT_FAILURE(16)
/* 17: no hardware */
#define	LMIC_IOCTL_RESULT_NO_HARDWARE			\
	LMIC_IOCTL_RESULT_FAILURE(17)
/* 18: hardware IO-error */
#define	LMIC_IOCTL_RESULT_HARDWARE_IOERR		\
	LMIC_IOCTL_RESULT_FAILURE(18)
/* 19: already started */
#define	LMIC_IOCTL_RESULT_ALREADY_STARTED		\
	LMIC_IOCTL_RESULT_FAILURE(19)
/* 20: already stopped */
#define	LMIC_IOCTL_RESULT_ALREADY_STOPPED		\
	LMIC_IOCTL_RESULT_FAILURE(20)
/* 21: already register */
#define	LMIC_IOCTL_RESULT_ALREADY_REGISTER		\
	LMIC_IOCTL_RESULT_FAILURE(21)
/* 22: hardware not support */
#define	LMIC_IOCTL_RESULT_HARDWARE_NOT_SUPPORT		\
	LMIC_IOCTL_RESULT_FAILURE(22)
/* 23: already suspended */
#define	LMIC_IOCTL_RESULT_ALREADY_SUSPENDED		\
	LMIC_IOCTL_RESULT_FAILURE(23)


/****************************************************************************\
|
|	API functions
|
\****************************************************************************/

LMIC_BEGIN_DECLS

LMIC_SAL_Success(return == 0)
LMIC_SAL_Check_return
LMIC_IOCTL_RESULT
LMIC_Ioctl_CheckParams(
	LMIC_SAL_In 		LMIC_IOCTL_CODE		Ioctl,
	LMIC_SAL_In_opt 	const  void *		pInParam,
	LMIC_SAL_In_opt 	void *			pOutParam
	);

LMIC_END_DECLS

/**** end of lmic_ioctl.h ****/
#endif /* _LMIC_IOCTL_H_ */
