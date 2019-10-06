#ifndef PHYSICS_H_
#define PHYSICS_H_

#include <iostream>
#include <memory>
#include <type_traits>

#include "Array2D.hpp"
#include "ManagedVariable.hpp"

// Needs to be speciallized for the physics being solved for.


typedef Array2D<uint8_t> BC;
enum boundary_type : uint8_t {
	none	  = 0, // Not a Bounary (Testing for Zero can be quite fast)
	dirichlet = 1, // First-type
	neumann   = 2 // Second-type
};

// Multi-Array Interactions
template <typename T>
static constexpr void iterp_bilinear(const Array2D<T>& source, Array2D<T>& up_left,
	Array2D<T>& up_right, Array2D<T>& down_left, Array2D<T>& down_right,
	const BC& ul_b, const BC& ur_b, const BC& dl_b, const BC& dr_b);
template <typename T>
static constexpr void iterp_bilinear_int(const Array2D<T>& source, Array2D<T>& up_left,
	Array2D<T>& up_right, Array2D<T>& down_left, Array2D<T>& down_right,
	const BC& ul_b, const BC& ur_b, const BC& dl_b, const BC& dr_b);
template <typename T>
static constexpr void downsamp_mean(Array2D<T>& destination, const BC& dest_b,
	const Array2D<T>& up_left, const Array2D<T>& up_right,
	const Array2D<T>& down_left, const Array2D<T>& down_right);
template <typename T>
static constexpr void downsamp_sum(Array2D<T>& destination, const BC& dest_b,
	const Array2D<T>& up_left, const Array2D<T>& up_right,
	const Array2D<T>& down_left, const Array2D<T>& down_right);

template <class T>
class PhysicsData{
public:
	/*
	 * (!) Warning: Unsafe Resource
	 * Impelementation DEPENDS on an external call to instiating Physics.free_vars()
	 * Prior to deletion to de-allocate the memory resources
	 * ---
	 * Provides a minimal degree of separation between the aggregated data and
	 * the individual resource nodes.
	 * ---
	 * Element (0,0) is taken to be the top left
	 */
	typedef typename ManagedVariable<T>::MVar MVar;
	typedef typename ManagedVariable<BC>::MVar MBound;
	class PhysicsNode{
		// MVar(s) are effectively unique pointers.
		// Move semantics are FORBIDDEN
	public:
		MVar   phi;
		MVar   rho;
		MBound bounds;
		// === Constructor ===
		constexpr PhysicsNode(MVar &&Phi, MVar &&Rho, MBound && Bounds) : // <== This one insists on copying
			phi(std::move(Phi)), rho(std::move(Rho)), bounds(std::move(Bounds)) { } 
		// Copy Constructor
		constexpr PhysicsNode(const PhysicsNode&) = delete;
		// Move Constructor
		constexpr PhysicsNode(PhysicsNode&& other) : 
			phi(std::move(other.phi)), rho(std::move(other.rho)),
			bounds(std::move(other.bounds)) { }
		// === Destructor ===
		~PhysicsNode() { }
		// === Operators ===
		// Copy Assignment
		constexpr PhysicsNode& operator=(const PhysicsNode& rhs) = delete;
		// Move Assignment
		constexpr PhysicsNode& operator=(PhysicsNode&& rhs){
			phi    = std::move(rhs.phi);
			rho    = std::move(rhs.rho);
			bounds = std::move(rhs.bounds);
			return *this;
		}
		// === Friends ===
		constexpr friend std::ostream& operator<<(std::ostream &stream,
			const PhysicsNode &node)
		{
			stream << "Phi [V]:\n" << node.phi << "\n";
			stream << "Rho [C/m^3]:\n" << node.rho;
			return stream;
		}
	private:
	};
	typedef PhysicsNode return_t; // Used externally

	constexpr return_t get(){
		return PhysicsNode(	std::forward<MVar>(Phi.create_element()),
							std::forward<MVar>(Rho_E.create_element()),
							std::forward<MBound>(Bounds.create_element()) );
	}
private:
	/*
	 * Resources are expected to be edited concurrently with the creation
	 * and deletion of PhysicsNode(s).
	 */
	ManagedVariable<T> Phi, Rho_E; // (ES) Potential [V]; Charge Density [C/m^3]
	ManagedVariable<BC> Bounds; // Boundary Conditions
};

template <typename T>
static constexpr void iterp_bilinear(const Array2D<T>& source, Array2D<T>& up_left,
	Array2D<T>& up_right, Array2D<T>& down_left, Array2D<T>& down_right,
	const BC& ul_b, const BC& ur_b, const BC& dl_b, const BC& dr_b)
{ // Interpolated bilinearly over (2x2) blocks of nodes
	constexpr int_fast8_t half_len = ARRAY_ELEMENT_LENGTH/2;
	// ii, jj Form the Basis for Source Coordinates
	// kk, ll Form the Basis for Target Coordinates
	for(int_fast8_t ii = 0; ii < half_len; ii = ii + 2){ // Upper Half
		for(int_fast8_t jj = 0; jj < half_len; jj = jj + 2){ // Left Half
			// Sample Source for Bilinear Interpolation
			T q12 = source.get_element(ii    , jj    ); // UL
			T q22 = source.get_element(ii    , jj + 1); // UR
			T q11 = source.get_element(ii + 1, jj    ); // DL
			T q21 = source.get_element(ii + 1, jj + 1); // DR
			// Calculate Bilinear Interpolation Constants
			T A = q22 - q12 - q21 + q11;
			T B =       q12       - q11;
			T C =             q21 - q11;
			T D =                   q11;
			// Interpolate
			for(int_fast8_t kk = 0; kk < 4; ++kk){
				int_fast8_t kk_p = 2*ii + kk;
				T y = kk/3.0; // Maps kk to 0, 0.33, 0.67, 1 for interpolation
				for(int_fast8_t ll = 0; ll < 4; ++ll){
					int_fast8_t ll_p = 2*jj + ll;
					if( !ul_b.get_element(kk_p, ll_p) ){
						T x = ll/3.0; // Maps ll to 0, 0.33, 0.67, 1 for interpolation
						T interp = D + x*C + y*(x*A + B);
						up_left.set_element(kk_p, ll_p, interp);
					}
				}
			}
		}
		for(int_fast8_t jj = 0; jj < half_len; jj + jj + 2){ // Right Half
			int_fast8_t jj_p = jj + half_len;
			// Sample Source for Bilinear Interpolation
			T q12 = source.get_element(ii    , jj    ); // UL
			T q22 = source.get_element(ii    , jj + 1); // UR
			T q11 = source.get_element(ii + 1, jj    ); // DL
			T q21 = source.get_element(ii + 1, jj + 1); // DR
			// Calculate Bilinear Interpolation Constants
			T A = q22 - q12 - q21 + q11;
			T B =       q12       - q11;
			T C =             q21 - q11;
			T D =                   q11;
			// Interpolate
			for(int_fast8_t kk = 0; kk < 4; ++kk){
				int_fast8_t kk_p = 2*ii + kk;
				T y = kk/3.0; // Maps kk to 0, 0.33, 0.67, 1 for interpolation
				for(int_fast8_t ll = 0; ll < 4; ++ll){
					int_fast8_t ll_p = 2*jj + ll;
					if( !ur_b.get_element(kk_p, ll_p) ){
						T x = ll/3.0; // Maps ll to 0, 0.33, 0.67, 1 for interpolation
						T interp = D + x*C + y*(x*A + B);
						up_right.set_element(kk_p, ll_p, interp);
					}
				}
			}
		}
	}
	for(int_fast8_t ii = 0; ii < half_len; ii = ii + 2){ // Lower Half
		int_fast8_t ii_p = ii + half_len;
		for(int_fast8_t jj = 0; jj < half_len; jj = jj + 2){ // Left Half
			// Sample Source for Bilinear Interpolation
			T q12 = source.get_element(ii    , jj    ); // UL
			T q22 = source.get_element(ii    , jj + 1); // UR
			T q11 = source.get_element(ii + 1, jj    ); // DL
			T q21 = source.get_element(ii + 1, jj + 1); // DR
			// Calculate Bilinear Interpolation Constants
			T A = q22 - q12 - q21 + q11;
			T B =       q12       - q11;
			T C =             q21 - q11;
			T D =                   q11;
			// Interpolate
			for(int_fast8_t kk = 0; kk < 4; ++kk){
				int_fast8_t kk_p = 2*ii + kk;
				T y = kk/3.0; // Maps kk to 0, 0.33, 0.67, 1 for interpolation
				for(int_fast8_t ll = 0; ll < 4; ++ll){
					int_fast8_t ll_p = 2*jj + ll;
					if( !dl_b.get_element(kk_p, ll_p) ){
						T x = ll/3.0; // Maps ll to 0, 0.33, 0.67, 1 for interpolation
						T interp = D + x*C + y*(x*A + B);
						down_left.set_element(kk_p, ll_p, interp);
					}
				}
			}
		}
		for(int_fast8_t jj = 0; jj < half_len; jj = jj + 2){ // Right Half
			int_fast8_t jj_p = jj + half_len;
			// Sample Source for Bilinear Interpolation
			T q12 = source.get_element(ii    , jj    ); // UL
			T q22 = source.get_element(ii    , jj + 1); // UR
			T q11 = source.get_element(ii + 1, jj    ); // DL
			T q21 = source.get_element(ii + 1, jj + 1); // DR
			// Calculate Bilinear Interpolation Constants
			T A = q22 - q12 - q21 + q11;
			T B =       q12       - q11;
			T C =             q21 - q11;
			T D =                   q11;
			// Interpolate
			for(int_fast8_t kk = 0; kk < 4; ++kk){
				int_fast8_t kk_p = 2*ii + kk;
				T y = kk/3.0; // Maps kk to 0, 0.33, 0.67, 1 for interpolation
				for(int_fast8_t ll = 0; ll < 4; ++ll){
					int_fast8_t ll_p = 2*jj + ll;
					if( !dr_b.get_element(kk_p, ll_p) ){
						T x = ll/3.0; // Maps ll to 0, 0.33, 0.67, 1 for interpolation
						T interp = D + x*C + y*(x*A + B);
						down_right.set_element(kk_p, ll_p, interp);
					}
				}
			}
		}
	}
}
template <typename T>
static constexpr void iterp_bilinear_int(const Array2D<T>& source, Array2D<T>& up_left,
	Array2D<T>& up_right, Array2D<T>& down_left, Array2D<T>& down_right,
	const BC& ul_b, const BC& ur_b, const BC& dl_b, const BC& dr_b)
{ // Interpolated by integrated bilinear interpolation over (2x2) blocks
	// The total integrated quantity is conserved
	constexpr int_fast8_t half_len = ARRAY_ELEMENT_LENGTH/2;
	// ii, jj Form the Basis for Source Coordinates
	// kk, ll Form the Basis for Target Coordinates
	for(int_fast8_t ii = 0; ii < half_len; ii = ii + 2){ // Upper Half
		for(int_fast8_t jj = 0; jj < half_len; jj = jj + 2){ // Left Half
			// Sample Source for Bilinear Interpolation
			T q12 = source.get_element(ii    , jj    ); // UL
			T q22 = source.get_element(ii    , jj + 1); // UR
			T q11 = source.get_element(ii + 1, jj    ); // DL
			T q21 = source.get_element(ii + 1, jj + 1); // DR
			// Calculate Bilinear Interpolation Constants
			T A = q22 - q12 - q21 + q11;
			T B =       q12       - q11;
			T C =             q21 - q11;
			T D =                   q11;
			// Interpolate
			for(int_fast8_t kk = 0; kk < 4; ++kk){
				int_fast8_t kk_p = 2*ii + kk;
				T dy2 = (2.0*kk + 1.0)/256.0; // (y[ii+1])^2 - (y[ii])^2
				for(int_fast8_t ll = 0; ll < 4; ++ll){
					int_fast8_t ll_p = 2*jj + ll;
					if( !ul_b.get_element(kk_p, ll_p) ){
						T dx2 = (2.0*ll + 1.0)/256.0; // (x[ii+1])^2 - (x[ii])^2
						T interp = dx2*(A*dy2 + 0.125*C) + 0.125*(B*dy2 + 0.125*D);
						up_left.set_element(kk_p, ll_p, interp);
					}
				}
			}
		}
		for(int_fast8_t jj = 0; jj < half_len; jj + jj + 2){ // Right Half
			int_fast8_t jj_p = jj + half_len;
			// Sample Source for Bilinear Interpolation
			T q12 = source.get_element(ii    , jj    ); // UL
			T q22 = source.get_element(ii    , jj + 1); // UR
			T q11 = source.get_element(ii + 1, jj    ); // DL
			T q21 = source.get_element(ii + 1, jj + 1); // DR
			// Calculate Bilinear Interpolation Constants
			T A = q22 - q12 - q21 + q11;
			T B =       q12       - q11;
			T C =             q21 - q11;
			T D =                   q11;
			// Interpolate
			for(int_fast8_t kk = 0; kk < 4; ++kk){
				int_fast8_t kk_p = 2*ii + kk;
				T dy2 = (2.0*kk + 1.0)/256.0; // (y[ii+1])^2 - (y[ii])^2
				for(int_fast8_t ll = 0; ll < 4; ++ll){
					int_fast8_t ll_p = 2*jj + ll;
					if( !ur_b.get_element(kk_p, ll_p) ){
						T dx2 = (2.0*ll + 1.0)/256.0; // (x[ii+1])^2 - (x[ii])^2
						T interp = dx2*(A*dy2 + 0.125*C) + 0.125*(B*dy2 + 0.125*D);
						up_right.set_element(kk_p, ll_p, interp);
					}
				}
			}
		}
	}
	for(int_fast8_t ii = 0; ii < half_len; ii = ii + 2){ // Lower Half
		int_fast8_t ii_p = ii + half_len;
		for(int_fast8_t jj = 0; jj < half_len; jj = jj + 2){ // Left Half
			// Sample Source for Bilinear Interpolation
			T q12 = source.get_element(ii    , jj    ); // UL
			T q22 = source.get_element(ii    , jj + 1); // UR
			T q11 = source.get_element(ii + 1, jj    ); // DL
			T q21 = source.get_element(ii + 1, jj + 1); // DR
			// Calculate Bilinear Interpolation Constants
			T A = q22 - q12 - q21 + q11;
			T B =       q12       - q11;
			T C =             q21 - q11;
			T D =                   q11;
			// Interpolate
			for(int_fast8_t kk = 0; kk < 4; ++kk){
				int_fast8_t kk_p = 2*ii + kk;
				T dy2 = (2.0*kk + 1.0)/256.0; // (y[ii+1])^2 - (y[ii])^2
				for(int_fast8_t ll = 0; ll < 4; ++ll){
					int_fast8_t ll_p = 2*jj + ll;
					if( !dl_b.get_element(kk_p, ll_p) ){
						T dx2 = (2.0*ll + 1.0)/256.0; // (x[ii+1])^2 - (x[ii])^2
						T interp = dx2*(A*dy2 + 0.125*C) + 0.125*(B*dy2 + 0.125*D);
						down_left.set_element(kk_p, ll_p, interp);
					}
				}
			}
		}
		for(int_fast8_t jj = 0; jj < half_len; jj = jj + 2){ // Right Half
			int_fast8_t jj_p = jj + half_len;
			// Sample Source for Bilinear Interpolation
			T q12 = source.get_element(ii    , jj    ); // UL
			T q22 = source.get_element(ii    , jj + 1); // UR
			T q11 = source.get_element(ii + 1, jj    ); // DL
			T q21 = source.get_element(ii + 1, jj + 1); // DR
			// Calculate Bilinear Interpolation Constants
			T A = q22 - q12 - q21 + q11;
			T B =       q12       - q11;
			T C =             q21 - q11;
			T D =                   q11;
			// Interpolate
			for(int_fast8_t kk = 0; kk < 4; ++kk){
				int_fast8_t kk_p = 2*ii + kk;
				T dy2 = (2.0*kk + 1.0)/256.0; // (y[ii+1])^2 - (y[ii])^2
				for(int_fast8_t ll = 0; ll < 4; ++ll){
					int_fast8_t ll_p = 2*jj + ll;
					if( !dr_b.get_element(kk_p, ll_p) ){
						T dx2 = (2.0*ll + 1.0)/256.0; // (x[ii+1])^2 - (x[ii])^2
						T interp = dx2*(A*dy2 + 0.125*C) + 0.125*(B*dy2 + 0.125*D);
						down_right.set_element(kk_p, ll_p, interp);
					}
				}
			}
		}
	}
}
template <typename T>
static constexpr void downsamp_mean(Array2D<T>& destination, const BC& dest_b,
	const Array2D<T>& up_left, const Array2D<T>& up_right,
	const Array2D<T>& down_left, const Array2D<T>& down_right)
{ // Quantities are Averaged over (2x2) blockes of nodes
	constexpr int_fast8_t full_len = ARRAY_ELEMENT_LENGTH/2;
	constexpr int_fast8_t half_len = ARRAY_ELEMENT_LENGTH/2;
	for(int_fast8_t ii = 0; ii < full_len; ii = ii + 2){ // Upper Half
		int ii_p = ii/2;
		for(int_fast8_t jj = 0; jj < full_len; jj = jj + 2){ // Left Half
			int jj_p = jj/2;
			if( !dest_b.get_element(ii_p, jj_p) ){
				T q12 = up_left.get_element(ii    , jj    ); // UL
				T q22 = up_left.get_element(ii    , jj + 1); // UR
				T q11 = up_left.get_element(ii + 1, jj    ); // DL
				T q21 = up_left.get_element(ii + 1, jj + 1); // DR
				T interp = 0.25*(q12 + q22 + q11 + q21);
				destination.set_element(ii_p, jj_p, interp);
			}
		}
		for(int_fast8_t jj = 0; jj < full_len; jj = jj + 2){ // Right Half
			int_fast8_t jj_p = jj/2 + half_len;
			if( !dest_b.get_element(ii_p, jj_p) ){
				T q12 = up_right.get_element(ii    , jj    ); // UL
				T q22 = up_right.get_element(ii    , jj + 1); // UR
				T q11 = up_right.get_element(ii + 1, jj    ); // DL
				T q21 = up_right.get_element(ii + 1, jj + 1); // DR
				T interp = 0.25*(q12 + q22 + q11 + q21);
				destination.set_element(ii_p, jj_p, interp);
			}
		}
	}
	for(int_fast8_t ii = 0; ii < full_len; ii = ii + 2){ // Lower Half
		int ii_p = ii/2 + half_len;
		for(int_fast8_t jj = 0; jj < full_len; jj = jj + 2){ // Left Half
			int jj_p = jj/2;
			if( !dest_b.get_element(ii_p, jj_p) ){
				T q12 = down_left.get_element(ii    , jj    ); // UL
				T q22 = down_left.get_element(ii    , jj + 1); // UR
				T q11 = down_left.get_element(ii + 1, jj    ); // DL
				T q21 = down_left.get_element(ii + 1, jj + 1); // DR
				T interp = 0.25*(q12 + q22 + q11 + q21);
				destination.set_element(ii_p, jj_p, interp);
			}
		}
		for(int_fast8_t jj = 0; jj < full_len; jj = jj + 2){ // Right Half
			int_fast8_t jj_p = jj/2 + half_len;
			if( !dest_b.get_element(ii_p, jj_p) ){
				T q12 = down_right.get_element(ii    , jj    ); // UL
				T q22 = down_right.get_element(ii    , jj + 1); // UR
				T q11 = down_right.get_element(ii + 1, jj    ); // DL
				T q21 = down_right.get_element(ii + 1, jj + 1); // DR
				T interp = 0.25*(q12 + q22 + q11 + q21);
				destination.set_element(ii_p, jj_p, interp);
			}
		}
	}
}
template <typename T>
static constexpr void downsamp_sum(Array2D<T>& destination, const BC& dest_b,
	const Array2D<T>& up_left, const Array2D<T>& up_right,
	const Array2D<T>& down_left, const Array2D<T>& down_right)
{ // Quantities are summed over (2x2) Blocks of Nodes (Integration over the Space)
	constexpr int_fast8_t full_len = ARRAY_ELEMENT_LENGTH/2;
	constexpr int_fast8_t half_len = ARRAY_ELEMENT_LENGTH/2;
	for(int_fast8_t ii = 0; ii < full_len; ii = ii + 2){ // Upper Half
		int ii_p = ii/2;
		for(int_fast8_t jj = 0; jj < full_len; jj = jj + 2){ // Left Half
			int_fast8_t jj_p = jj/2;
			if( !dest_b.get_element(ii_p, jj_p) ){
				T q12 = up_left.get_element(ii    , jj    ); // UL
				T q22 = up_left.get_element(ii    , jj + 1); // UR
				T q11 = up_left.get_element(ii + 1, jj    ); // DL
				T q21 = up_left.get_element(ii + 1, jj + 1); // DR
				T interp = (q12 + q22 + q11 + q21);
				destination.set_element(ii_p, jj_p, interp);
			}
		}
		for(int_fast8_t jj = 0; jj < full_len; jj = jj + 2){ // Right Half
			int_fast8_t jj_p = jj/2 + half_len;
			if( !dest_b.get_element(ii_p, jj_p) ){
				T q12 = up_right.get_element(ii    , jj    ); // UL
				T q22 = up_right.get_element(ii    , jj + 1); // UR
				T q11 = up_right.get_element(ii + 1, jj    ); // DL
				T q21 = up_right.get_element(ii + 1, jj + 1); // DR
				T interp = (q12 + q22 + q11 + q21);
				destination.set_element(ii_p, jj_p, interp);
			}
		}
	}
	for(int_fast8_t ii = 0; ii < full_len; ii = ii + 2){ // Lower Half
		int ii_p = ii/2 + half_len;
		for(int_fast8_t jj = 0; jj < full_len; jj = jj + 2){ // Left Half
			int_fast8_t jj_p = jj/2;
			if( !dest_b.get_element(ii_p, jj_p) ){
				T q12 = down_left.get_element(ii    , jj    ); // UL
				T q22 = down_left.get_element(ii    , jj + 1); // UR
				T q11 = down_left.get_element(ii + 1, jj    ); // DL
				T q21 = down_left.get_element(ii + 1, jj + 1); // DR
				T interp = (q12 + q22 + q11 + q21);
				destination.set_element(ii_p, jj_p, interp);
			}
		}
		for(int_fast8_t jj = 0; jj < full_len; jj = jj + 2){ // Right Half
			int_fast8_t jj_p = jj/2 + half_len;
			if( !dest_b.get_element(ii_p, jj_p) ){
				T q12 = down_right.get_element(ii    , jj    ); // UL
				T q22 = down_right.get_element(ii    , jj + 1); // UR
				T q11 = down_right.get_element(ii + 1, jj    ); // DL
				T q21 = down_right.get_element(ii + 1, jj + 1); // DR
				T interp = (q12 + q22 + q11 + q21);
				destination.set_element(ii_p, jj_p, interp);
			}
		}
	}
}

#endif // PHYSICS_H_
