#ifndef PHYSICS_H_
#define PHYSICS_H_

#include <iostream>
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
		typedef typename ManagedVariable<T>::iterator_t IT;

		enum boundary_type : uint8_t {
			none	  = 0, // Not a Bounary (Testing for Zero can be quite fast)
			dirichlet = 1, // First-type
			neumann   = 2 // Second-type
		};

		struct PhysicsNode{
			/* 
			 * (!) Warning (!) Node Requires External Removal & Cleanup
			 * Throught the Physics::erase() interface.
			 */
			IT phi;
			IT rho;
			constexpr PhysicsNode(IT &&Phi, IT &&Rho) : phi(std::move(Phi)), rho(std::move(Rho)) {}
			constexpr PhysicsNode(const IT&) = delete; // Copy Constructors are Disallowed
			~PhysicsNode() { }
			constexpr friend std::ostream& operator<<(std::ostream &stream, const PhysicsNode &node) {
				stream << "Phi [V]:\n" << *(node.phi) << "\n";
				stream << "Rho [C/m^3]:\n" << *(node.rho);
				return stream;
			}
		};
		typedef PhysicsNode return_t; // Used externally
		constexpr return_t get(){
			return PhysicsNode(Phi.create_element(), Rho_E.create_element());
		}
		constexpr void erase(PhysicsNode &node){
			Phi.erase(std::forward<IT>(node.phi));
			Rho_E.erase(std::forward<IT>(node.rho));
		}
	private:
		/*
		 * Resources are expected to be edited concurrently with the creation
		 * and deletion of PhysicsNode(s).
		 */
		ManagedVariable<T> Phi, Rho_E; // (ES) Potential [V]; Charge Density [C/m^3]
};

#endif // PHYSICS_H_
