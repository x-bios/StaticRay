#pragma once


// Vector Template Classes ================================


// All vectors descend from this BaseVector.
struct BaseVector {};


// Constant Expression Square Root
// Adapted from a post by Alex Shtof. See https://stackoverflow.com/a/34134071
template <typename Type>
[[nodiscard]] Type constexpr sqrtNewtonRaphson(const Type x, const Type curr, const Type prev) {
	return curr == prev ? curr : sqrtNewtonRaphson(x, Type(0.5) * (curr + x / curr), curr);
}

template <typename Type>
[[nodiscard]] Type constexpr csqrt(const Type x) {
	return x >= 0 && x < Infinity ? sqrtNewtonRaphson(x, x, Type(0)) : Type(Infinity * 0);
}


// Declaration Specifiers
#define Inline					  			inline
#define InlineND	[[nodiscard]]			inline
#define InlineNDC	[[nodiscard]] constexpr inline

// Vector Template Declaration
#define VectorTemplate(VectorType)										\
	template <typename Type>											\
	struct alignas(sizeof(Type) >= 4 ? 16 :								\
		alignof(Type)) VectorType : BaseVector

// XYZW Vector Components
#define VectorComponents												\
	Type	x, y, z, w;

// RGBA Vector Components
#define ColorComponents													\
	Type	z, y, x, w;													\
	InlineND  Type& Red  ()       { return x; }							\
	InlineNDC Type  Red  () const { return x; }							\
	InlineND  Type& Green()       { return y; }							\
	InlineNDC Type  Green() const { return y; }							\
	InlineND  Type& Blue ()       { return z; }							\
	InlineNDC Type  Blue () const { return z; }							\
	InlineND  Type& Alpha()       { return w; }							\
	InlineNDC Type  Alpha() const { return w; }

// XYZW Vector Constructors
#define BasicConstructors(VectorType)									\
	VectorType() = default;												\
																		\
	template <typename XYZWType = Type>									\
	requires is_convertible_v<XYZWType, Type>							\
	constexpr VectorType(const XYZWType XYZW) :							\
		x(Type(XYZW)), y(Type(XYZW)),									\
		z(Type(XYZW)), w(Type(XYZW)) {}									\
																		\
	template <typename OtherType>										\
	requires derived_from<OtherType, BaseVector>						\
	constexpr VectorType(const OtherType& Other) :						\
		x(Type(Other.x)), y(Type(Other.y)), 							\
		z(Type(Other.z)), w(Type(Other.w)) {}							\
																		\
	template <typename OtherType, typename WType = Type>				\
	requires derived_from<OtherType, BaseVector> &&						\
			 is_convertible_v<WType, Type>								\
	constexpr VectorType(const OtherType& Other, const WType W) :		\
		x(Type(Other.x)), y(Type(Other.y)), 							\
		z(Type(Other.z)), w(Type(W      )) {}							\
																		\
	template <typename XType = Type, typename YType = Type,				\
			  typename ZType = Type, typename WType = Type>				\
	requires is_convertible_v<XType, Type> &&							\
			 is_convertible_v<YType, Type> &&							\
			 is_convertible_v<ZType, Type> &&							\
			 is_convertible_v<WType, Type>								\
	constexpr VectorType(const XType X, const YType Y, 					\
		const ZType Z = 0, const WType W = 0) :							\
		x(Type(X)), y(Type(Y)), z(Type(Z)), w(Type(W)) {}

// RGBA Vector Constructors
#define ColorConstructors(VectorType)									\
	VectorType() = default;												\
																		\
	template <typename RGBAType = Type>									\
	requires is_convertible_v<RGBAType, Type>							\
	constexpr VectorType(const RGBAType RGBA) :							\
		x(Type(RGBA)), y(Type(RGBA)),									\
		z(Type(RGBA)), w(Type(RGBA)) {}									\
																		\
	template <typename ColorType>										\
	requires derived_from<ColorType, BaseVector>						\
	constexpr VectorType(const ColorType& Color) :						\
		x(Type(Color.x)), y(Type(Color.y)), 							\
		z(Type(Color.z)), w(Type(Color.w)) {}							\
																		\
	template <typename RGBType, typename AlphaType = Type>				\
	requires derived_from<RGBType, BaseVector> &&						\
			 is_convertible_v<AlphaType, Type>							\
	constexpr VectorType(const RGBType& Color, const AlphaType Alpha) :	\
		x(Type(Color.x)), y(Type(Color.y)), 							\
		z(Type(Color.z)), w(Type(Alpha  )) {}							\
																		\
	template <typename RedType  = Type, typename GreenType = Type, 		\
			  typename BlueType = Type, typename AlphaType = Type>		\
	requires is_convertible_v<RedType,   Type> &&						\
			 is_convertible_v<GreenType, Type> &&						\
			 is_convertible_v<BlueType,  Type> &&						\
			 is_convertible_v<AlphaType, Type>							\
	constexpr VectorType(const RedType Red, const GreenType Green,		\
		const BlueType Blue, const AlphaType Alpha = 1) :				\
		x(Type(Red)), y(Type(Green)), z(Type(Blue)), w(Type(Alpha)) {}

// Array-Style Vector Component Access
#define ComponentAccess													\
	InlineND Type operator[] (const size_t Index) {						\
		assert(Index < 4);												\
		return (Type*)(this)[Index];									\
	}																	\
	InlineNDC Type operator[] (const size_t Index) const {				\
		assert(Index < 4);												\
		return (Type*)(this)[Index];									\
	}

// Unary Operator Implementation
#define UnaryOperator(VectorType, Op)									\
	InlineNDC VectorType operator##Op () const {						\
		return {Op x, Op y, Op z, Op w};								\
	}

// Binary Operator Implementations
#define BinaryOperators(VectorType, Op)									\
	InlineNDC VectorType operator##Op (const VectorType& Other) const {	\
		return {x Op Other.x, y Op Other.y,								\
				z Op Other.z, w Op Other.w};							\
	}																	\
																		\
	Inline VectorType& operator##Op##= (const VectorType& Other) {		\
		return (*this = *this Op Other);								\
	}

// Scalar Operator Implementations
#define ScalarOperators(VectorType, Op)									\
	template <typename ScalarType>										\
	requires is_convertible_v<ScalarType, Type>							\
	InlineNDC VectorType operator##Op (const ScalarType Scalar) const {	\
		return {x Op Type(Scalar), y Op Type(Scalar),					\
				z Op Type(Scalar), w Op Type(Scalar)};					\
	}																	\
																		\
	template <typename ScalarType>										\
	requires is_convertible_v<ScalarType, Type>							\
	Inline VectorType& operator##Op##= (const ScalarType Scalar) {		\
		return (*this = *this Op Type(Scalar));							\
	}

// All Basic Operator Implementations
#define BasicOperators(VectorType)										\
	UnaryOperator  (VectorType, -)										\
	BinaryOperators(VectorType, +)										\
	BinaryOperators(VectorType, -)										\
	BinaryOperators(VectorType, *)										\
	BinaryOperators(VectorType, /)										\
	ScalarOperators(VectorType, +)										\
	ScalarOperators(VectorType, -)										\
	ScalarOperators(VectorType, *)										\
	ScalarOperators(VectorType, /)	

// All Bitwise Operator Implementations
#define BitwiseOperators(VectorType)									\
	UnaryOperator  (VectorType,  ~)										\
	BinaryOperators(VectorType,  &)										\
	BinaryOperators(VectorType,  |)										\
	BinaryOperators(VectorType,  ^)										\
	ScalarOperators(VectorType,  &)										\
	ScalarOperators(VectorType,  |)										\
	ScalarOperators(VectorType,  ^)										\
	ScalarOperators(VectorType, >>)										\
	ScalarOperators(VectorType, <<)

// Integer-Only Modulus Operator Implementations
#define IntegerModulus(VectorType)										\
	BinaryOperators(VectorType,  %)										\
	ScalarOperators(VectorType,  %)

// Float-Only Modulus Operator Implementations
#define FloatModulus(VectorType)										\
	InlineNDC VectorType operator% (const VectorType& Other) const {	\
		return {modf(x, Other.x), modf(y, Other.y), 					\
				modf(z, Other.z), modf(w, Other.w)};					\
	}																	\
																		\
	template <typename ScalarType>										\
	requires is_convertible_v<ScalarType, Type>							\
	InlineNDC VectorType operator% (const ScalarType Scalar) const {	\
		return {modf(x, Type(Scalar)), modf(y, Type(Scalar)), 			\
				modf(z, Type(Scalar)), modf(w, Type(Scalar))};			\
	}																	\
																		\
	Inline VectorType& operator%= (const VectorType& Other) {			\
		return (*this = *this % Other);									\
	}																	\
																		\
	template <typename ScalarType>										\
	requires is_convertible_v<ScalarType, Type>							\
	Inline VectorType& operator%= (const ScalarType Scalar) {			\
		return (*this = *this % Type(Scalar));							\
	}

// Relational Operator Implementations
#define RelationalOperators(VectorType)									\
	InlineNDC bool operator== (const VectorType& Other) const {		\
		return x == Other.x && y == Other.y && z == Other.z;			\
	}																	\
																		\
	InlineNDC bool operator!= (const VectorType& Other) const {		\
		return !(*this == Other);										\
	}																	\
																		\
	InlineNDC bool operator<  (const VectorType& Other) const {		\
		return (*this - Other).Max() < 0;								\
	}																	\
																		\
	InlineNDC bool operator<= (const VectorType& Other) const {		\
		return (*this - Other).Max() <= 0;								\
	}																	\
																		\
	InlineNDC bool operator>  (const VectorType& Other) const {		\
		return (*this - Other).Min() > 0;								\
	}																	\
																		\
	InlineNDC bool operator>= (const VectorType& Other) const {		\
		return (*this - Other).Min() >= 0;								\
	}

// 3D and 4D Metrics Implementations
#define VectorMetrics(VectorType)										\
	InlineNDC Type Sum() const {										\
		return x + y + z;												\
	}																	\
																		\
	InlineNDC Type Sum4() const {										\
		return x + y + z + w;											\
	}																	\
																		\
	InlineNDC Type Min() const {										\
		return min(x, min(y, z));										\
	}																	\
																		\
	InlineNDC Type Min4() const {										\
		return min(x, min(y, min(z, w)));								\
	}																	\
																		\
	InlineNDC Type Max() const {										\
		return max(x, max(y, z));										\
	}																	\
																		\
	InlineNDC Type Max4() const {										\
		return max(x, max(y, max(z, w)));								\
	}																	\
																		\
	InlineNDC VectorType Abs() const {									\
		return VectorType(Abs4(), 0);									\
	}																	\
																		\
	InlineNDC VectorType Abs4() const {									\
		return {abs(x), abs(y), abs(z), abs(w)};						\
	}																	\
																		\
	InlineNDC VectorType Min(VectorType&& Other) const {				\
		return VectorType(Min4(Other), 0);								\
	}																	\
																		\
	InlineNDC VectorType Min4(VectorType&& Other) const {				\
		return {min(x, Other.x), min(y, Other.y),						\
				min(z, Other.z), min(w, Other.w)};						\
	}																	\
																		\
	InlineNDC VectorType Max(VectorType&& Other) const {				\
		return VectorType(Max4(Other), 0);								\
	}																	\
																		\
	InlineNDC VectorType Max4(VectorType&& Other) const {				\
		return {max(x, Other.x), max(y, Other.y),						\
				max(z, Other.z), max(w, Other.w)};						\
	}																	\
																		\
	InlineNDC VectorType Clamp(const Type Minimum = 0,					\
							   const Type Maximum = 1) const {			\
		return VectorType(Clamp4(Minimum, Maximum), 0);					\
	}																	\
																		\
	InlineNDC VectorType Clamp4(const Type Minimum = 0,					\
								const Type Maximum = 1) const {			\
		return Max4(Minimum).Min4(Maximum);								\
	}

// 3D Linear Algebra Implementations
#define LinearAlgebra(VectorType)										\
	InlineNDC VectorType Cross(const VectorType& Other) const {			\
		return {y * Other.z - z * Other.y, 								\
				z * Other.x - x * Other.z, 								\
				x * Other.y - y * Other.x};								\
	}																	\
																		\
	InlineNDC Type Dot(const VectorType& Other) const {					\
		return (*this * Other).Sum();									\
	}																	\
																		\
	InlineNDC Type LengthSq() const {									\
		return Dot(*this);												\
	}																	\
																		\
	InlineND Type Length() const {										\
		return sqrt(LengthSq());										\
	}																	\
																		\
	InlineND VectorType Normalized() const {							\
		return *this / Length();										\
	}																	\
																		\
	Inline VectorType& Normalize() {									\
		return (*this = Normalized());									\
	}																	\
																		\
	InlineNDC Type ConstLength() const {								\
		return csqrt(LengthSq());										\
	}																	\
																		\
	InlineNDC VectorType ConstNormalized() const {						\
		return *this / ConstLength();									\
	}
	
// Basic Linear Algebra-enabled Vector
VectorTemplate(LinearVector3D) {
	VectorComponents;
	ComponentAccess;

	BasicConstructors	(LinearVector3D);
	BasicOperators		(LinearVector3D);
	RelationalOperators	(LinearVector3D);
	FloatModulus		(LinearVector3D);
	VectorMetrics		(LinearVector3D);
	LinearAlgebra		(LinearVector3D);
};

// Basic Integer Vector
VectorTemplate(IntegerVector4) {
	VectorComponents;
	ComponentAccess;

	BasicConstructors	(IntegerVector4);
	BasicOperators		(IntegerVector4);
	BitwiseOperators	(IntegerVector4);
	RelationalOperators	(IntegerVector4);
	IntegerModulus		(IntegerVector4);
	VectorMetrics		(IntegerVector4);
};

// Basic Integer Color Vector
VectorTemplate(IntegerColor4) {
	ColorComponents;
	ComponentAccess;

	ColorConstructors	(IntegerColor4);
	BasicOperators		(IntegerColor4);
	BitwiseOperators	(IntegerColor4);
	RelationalOperators	(IntegerColor4);
	IntegerModulus		(IntegerColor4);
	VectorMetrics		(IntegerColor4);

	// Cast this color to a 32bit ARGB Integer
	InlineND uint32 Color() const {
		const IntegerColor4<uint8> color(Clamp4(0, 255));
		return *(uint32*)&color;
	}
};

// Basic Floating-Point Color Vector
VectorTemplate(FloatColor4) {
	ColorComponents;
	ComponentAccess;

	ColorConstructors	(FloatColor4);
	BasicOperators		(FloatColor4);
	RelationalOperators	(FloatColor4);
	FloatModulus		(FloatColor4);
	VectorMetrics		(FloatColor4);

	// Cast this color to a 32bit ARGB Integer
	InlineND uint32 Color() const {
		return IntegerColor4<uint8>(Clamp4() * 255).Color();
	}
};


// Remove Macros
#undef Inline
#undef InlineND
#undef InlineNDC
#undef VectorTemplate
#undef VectorComponents
#undef ColorComponents
#undef BasicConstructors
#undef ColorConstructors
#undef ComponentAccess
#undef UnaryOperator
#undef BinaryOperators
#undef ScalarOperators
#undef BasicOperators
#undef BitwiseOperators
#undef IntegerModulus
#undef FloatModulus
#undef RelationalOperators
#undef VectorMetrics
#undef LinearAlgebra


// 4x32bit Floating-Point Vector
using FVector = LinearVector3D<float32>;

// 4xReal Floating-Point Vector
using RVector = LinearVector3D<Real>;

// 4x32bit Integer Vector
using IVector = IntegerVector4<Integer>;

// 4x8bit Integer Vector
using BVector = IntegerVector4<uint8>;

// 4xReal Floating-Point Color
using RColor  = FloatColor4<Real>;

// 4xInteger Color
using IColor  = IntegerColor4<Integer>;

// 4x32bit Integer Color
using BColor  = IntegerColor4<uint8>;