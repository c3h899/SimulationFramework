#ifndef PHYSICS_H_
#define PHYSICS_H_

#include <iostream>
#include <memory>
#include <type_traits>

#include "Array2D.hpp"
#include "ManagedVariable.hpp"

// Needs to be speciallized for the physics being solved for.

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
	 */
	typedef typename ManagedVariable<T>::MVar MVar;
	typedef typename ManagedVariable<uint8_t>::MVar MBound;
	enum boundary_type : uint8_t {
		none	  = 0, // Not a Bounary (Testing for Zero can be quite fast)
		dirichlet = 1, // First-type
		neumann   = 2 // Second-type
	};
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
	ManagedVariable<uint8_t> Bounds; // Boundary Conditions
};

#endif // PHYSICS_H_
