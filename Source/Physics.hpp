#ifndef PHYSICS_H_
#define PHYSICS_H_

#include <iostream>
#include "Array2D.hpp"
#include "ManagedVariable.hpp"

// Needs to be speciallized for the physics being solved for.

/* Additional Types */
template <class T>
class Physics;

template <class T>
/*
 * (!) Warning: Unsafe Resource
 * Impelementation DEPENDS on an external call to instiating Physics.free_vars()
 * Prior to deletion to de-allocate the memory resources
 */
struct PhysicsNode{
	Array2D<T> *phi = nullptr;
	Array2D<T> *rho = nullptr; // Revise this to be less memory intensive
	PhysicsNode(Array2D<T> *Phi = nullptr, Array2D<T> *Rho = nullptr) :
		phi(Phi), rho(Rho){};
	~PhysicsNode() {
		if((phi != nullptr) || (rho != nullptr)){
			std::cerr << "Resource Leak. PhysicsNode was not safeley deleted." << std::endl;
		}
	}
	friend std::ostream& operator<<(std::ostream &stream, const PhysicsNode<T> &node){
		stream << "Phi [V]:\n" << *(node.phi) << "\n";
		stream << "Rho [C/m^3]:\n" << *(node.rho);
		return stream;
		}
	private:
};

template <class T>
class Physics{
	public:
		PhysicsNode<T> get_node(){
			return PhysicsNode<T>(Phi.create_element(), Rho_E.create_element());
		}
		void free_vars(PhysicsNode<T> &node){
//			std::cout << "Deleting Records for Node : " << (void*) node << std::endl;
			// Ensure the resource exists before use.
			if(node.phi != nullptr){ node.phi = Phi.remove_element(node.phi); }
			if(node.rho != nullptr){ node.rho = Rho_E.remove_element(node.rho); }
		}
		/* Physics */
	private:
		/*
		 * Resources are expected to be edited concurrently with the creation
		 * and deletion of PhysicsNode(s).
		 */
		ManagedVariable<Array2D<T>> Phi, Rho_E;
			// Electrostatic Potential [V]
			// Volumetric Charge Density [C/m^3]
};

#endif // PHYSICS_H_
